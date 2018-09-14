/* Arduino library for ST M24SR Dynamic NFC/RFID tag IC with EEPROM, NFC Forum Type 4 Tag and I2C interface
 (c) 2014 by ReNa http://regnerischernachmittag.wordpress.com/
 */
//==============================================================================
#include <M24SR.h>
//==============================================================================
M24SR::M24SR(uint8_t gpo)
{
    verbose = false;
    cmds = false;
    gpoPin = gpo;
}
//==============================================================================
M24SR::~M24SR()
{
    if (responseLength)
    {
        free(response);
    }
}
//==============================================================================
void M24SR::setup()
{
    if (verbose)
    {
        Serial.println(F("setup"));
    }
    lastGPO = 1;
    sendGetI2cSession = true;
    deviceAddress = 0x56;
    blockNo = 0;
    responseLength = 0x15;
    response = (byte*)malloc(responseLength);
    
    Wire.begin(); // join i2c bus (address optional for master)
    pinMode(gpoPin, INPUT);
    writeGPO(0x61);
}
// //==============================================================================
void M24SR::writeGPO(uint8_t value)
{
    if (verbose)
    {
        Serial.println(F("\r\nwriteGPO"));
    }
    
    if (!verifyI2cPassword()) {
        Serial.println(F("\r\nwrong password!!!"));
        return;
    }
    
    sendApdu_P(0x00, INS_SELECT_FILE, 0x00, 0x0C, 0x02, FILE_SYSTEM);
    receiveResponse(2 + 3);
    
    sendApdu(0x00, INS_UPDATE_BINARY, 0x00, 0x04, 0x01, &value); //write system file at offset 0x0004 GPO
    receiveResponse(2 + 3);
    sendDESELECT();
}

//==============================================================================
int M24SR::receiveResponse(unsigned int len)
{
    unsigned int index = 0;
    boolean WTX = false;
    boolean loop = false;
    if (verbose)
    {
        Serial.print(F("\r\nreceiveResponse, len="));
        Serial.print(len, DEC);
        Serial.println();
    }
    if (responseLength < len)
    {
        free(response);
        if (verbose)
        {
            Serial.print(F("\r\nresponseLength="));
            Serial.print(len, DEC);
        }
        responseLength = len;
        response = (byte*)malloc(responseLength);
    }
    else
    {
        delay(1);
    }
    do
    {
        WTX = false;
        loop = false;
        Wire.requestFrom(deviceAddress, len);
        if (cmds)
        {
            Serial.print(F("<= "));
        }
        else
        {
            delay(1);
        }
        while ((Wire.available() &&
                index < len &&
                !WTX) ||
               (WTX && index < len-1))
        {
            int c  = (Wire.read() & 0xff);
            if (cmds)
            {
                if (c < 0x10)
                {
                    Serial.print(F("0"));
                }
                Serial.print(c, HEX);
                Serial.print(F(" "));
            }
            else
            {
                delay(1);
            }
            if (c == 0xF2 && index == 0)
            {
                WTX = true;
            }
            if (index >= 1)
            {
                response[index-1] = c;
            }
            index++;
        }
        if (WTX)
        {
            Serial.print(F("\r\nWTX"));
            delay(200 * response[0]);
            //send WTX response
            //sendSBLOCK(0xF2);
            data[0] = 0xF2; //WTX
            data[1] = response[0];
            sendCommand(/*data,*/ 2, false);
            loop = true;
            index = 0;
        }
    }
    while(loop);
    return index;
}
//==============================================================================
/*
 end of a I2c Session:
 5.4 S-Block format
 0xC2: for S(DES) when the DID field is not present
 */
void M24SR::sendDESELECT()
{
    if (verbose)
    {
        Serial.print(F("\r\nsend DESELECT"));
    }
    sendSBLOCK(0xC2);//PCB field
}

void M24SR::sendSBLOCK(byte sblock)
{
    data[0] = sblock;
    sendCommand(/*data,*/ 1, false);
    receiveResponse(0 + 3) ;
}

//==============================================================================
void M24SR::writeNdefMessage(NdefMessage* pNDefMsg)
{
    if (pNDefMsg != NULL)
    {
        pNDefMsg->print();
        NdefRecord rec = pNDefMsg->getRecord(0);
        Serial.print(F("NDefRecord: "));
        rec.print();
        selectFileNdefApp();
        selectFileNdefFile();
        updateBinaryNdefMsgLen0();
        uint8_t len = pNDefMsg->getEncodedSize();
        uint8_t* mem = (uint8_t*)malloc(len);
        
        //TODO memcpy(&data[0], &SAMPLE_NDEF_message1[0], len);
        //uint8_t encoded[pNDefMsg->getEncodedSize()];
        pNDefMsg->encode((uint8_t*)mem);
        
        updateBinary((char*)mem, len);
        receiveResponse(2 + 3);
        free(mem);
        
        updateBinaryLen(len);
        receiveResponse(2 + 3);
        sendDESELECT();
    }
}
// //==============================================================================
void M24SR::selectFileNdefApp()
{
    if (verbose)
    {
        Serial.println(F("\r\nselectFile_NDEF_App"));
    }
    sendGetI2cSession = true;
    sendApdu_P(0x00, INS_SELECT_FILE, 0x04, 0x00, 0x07, AID_NDEF_TAG_APPLICATION2);
    receiveResponse(2 + 3);
}

void M24SR::selectFileNdefFile()
{
    if (verbose)
    {
        Serial.print(F("\r\nselectFile_NDEF_file"));
    }
    uint8_t ndef_file[] = "\x00\x01";
    sendApdu(0x00, INS_SELECT_FILE, 0x00, 0x0C, 0x02, ndef_file);
    receiveResponse(2 + 3);
}
// //==============================================================================
boolean M24SR::verifyI2cPassword()
{
    if (verbose)
    {
        Serial.println(F("\r\nverifyI2cPassword"));
    }
    selectFileNdefApp();
    sendApdu_P(0x00, INS_VERIFY, 0x00, 0x03, 0x10, DEFAULT_PASSWORD);
    receiveResponse(2 + 3);
    return ((response[0] == 0x90) && (response[1] == 0));
}
//==============================================================================
boolean M24SR::checkGPOTrigger()
{
    uint8_t newval = digitalRead(gpoPin);
    if (newval == 1 && lastGPO == 0) {
        lastGPO = newval;
        return true;
    }
    lastGPO = newval;
    return false;
}
//==============================================================================
void M24SR::updateBinaryLen(int len)
{
    if (verbose)
    {
        Serial.println(F("\r\nupdateBinaryLen"));
    }
    uint8_t len_bytes[] = "\x00\x00";
    len_bytes[0] = (len >> 8) & 0xff;
    len_bytes[1] = (len & 0xff);
    sendApdu(0x00, INS_UPDATE_BINARY, 0x00, 0x00, 0x02, len_bytes);
}
//==============================================================================
void M24SR::updateBinary(char* Data, uint8_t len)
{
    if (verbose)
    {
        Serial.println(F("\r\nupdateBinary"));
    }
    dumpHex((uint8_t*)Data, len);
    uint8_t pos = 0;
    while(pos < len)
    {
        uint8_t chunk_len = (len - pos);
        if (chunk_len > (BUFFER_LENGTH - 8))
        {
            chunk_len = BUFFER_LENGTH - 8;
        }
        Serial.print(F("\r\nchunk_len:"));
        Serial.print(chunk_len, DEC);
        Serial.print(F(", pos:"));
        Serial.print(pos, DEC);
        sendApdu(0x00, INS_UPDATE_BINARY, ((pos+2) >> 8) & 0xff, (pos+2) & 0xff, chunk_len, (uint8_t*)&Data[pos]);
        pos += chunk_len;
        if (pos < len)
        {
            receiveResponse(2 + 3);
        }
    }
}
//==============================================================================
void M24SR::updateBinary(unsigned int offset, char* data, uint8_t len)
{
    if (verbose)
    {
        Serial.println(F("\r\nupdateBinary offset"));
    }
    sendApdu(0x00, INS_UPDATE_BINARY, (offset >> 8) & 0xff, (offset & 0xff), len, (uint8_t*)data);
}
//==============================================================================
void M24SR::displaySystemFile()
{
    selectFileNdefApp();
    sendApdu_P(0x00, INS_SELECT_FILE, 0x00, 0x0C, 0x02, FILE_SYSTEM);
    receiveResponse(2 + 3);
    
    sendApdu(0x00, INS_READ_BINARY, 0x00, 0x00, 0x02);
    receiveResponse(2 + 2 + 3);
    
    if (verbose)
    {
        Serial.print(F("\r\nresponse[1]: "));
        Serial.print(response[1] & 0xff, HEX);
    }
    
    sendApdu(0x00, INS_READ_BINARY, 0x00, 0x00, response[1]);
    receiveResponse((response[1] & 0xff) + 2 + 3);
    
    //display settings
    Serial.print(F("\r\nUID: "));
    dumpHex(&response[8], 7);
    Serial.print(F("\r\nMemory Size: 0x"));
    if ((response[0xf] & 0xff) < 0x10)
        Serial.print("0");
    Serial.print((response[0xf] & 0xff), HEX);
    if ((response[0x10] & 0xff) < 0x10)
        Serial.print("0");
    Serial.print((response[0x10] & 0xff), HEX);
    Serial.print(F("\r\nProduct Code: 0x"));
    if ((response[0x11] & 0xff) < 0x10)
        Serial.print("0");
    Serial.print((response[0x11] & 0xff), HEX);
    
    sendDESELECT();
}


//==============================================================================
void M24SR::sendApdu(uint8_t CLA, uint8_t INS, uint8_t P1, uint8_t P2, uint8_t Lc, uint8_t* Data)
{
    data[1] = CLA;
    data[2] = INS;
    data[3] = P1;
    data[4] = P2;
    data[5] = Lc;
    memcpy(&data[6], Data, Lc);
    sendCommand(/*data,*/ 1+5+Lc, true);
}

void M24SR::sendApdu(uint8_t CLA, uint8_t INS, uint8_t P1, uint8_t P2, uint8_t Le)
{
    data[1] = CLA;
    data[2] = INS;
    data[3] = P1;
    data[4] = P2;
    data[5] = Le;
    sendCommand(/*data, */1+5, true);
}

void M24SR::sendApdu_P(uint8_t CLA, uint8_t INS, uint8_t P1, uint8_t P2, uint8_t Lc, const char* Data)
{
    data[1] = CLA;
    data[2] = INS;
    data[3] = P1;
    data[4] = P2;
    data[5] = Lc;
    memcpy_P(&data[6], Data, Lc);
    sendCommand(/*data,*/ 1+5+Lc, true);
}

//==============================================================================

void M24SR::sendCommand(/*char* data,*/ int len)
{
    sendCommand(/*data,*/ len, true);
}

void M24SR::sendCommand(/*char* data, */int len, boolean setPCB)
{
    uint8_t v;
    if (setPCB)
    {
        if (blockNo == 0)
        {
            data[0] = 0x02;
            blockNo = 1;
        } else
        {
            data[0] = 0x03;
            blockNo = 0;
        }
    }
    if (sendGetI2cSession)
    {
        Wire.beginTransmission(deviceAddress); // transmit to device 0x2D
        Wire.write(byte(CMD_GETI2CSESSION)); // GetI2Csession
        err = Wire.endTransmission();     // stop transmitting
        if (verbose)
        {
            Serial.print(F("\r\nGetI2Csession: "));
            Serial.print(err, HEX);
        }
        else
            delay(1);
    }
    
    if (cmds)
        Serial.print(F("\r\n=> "));
    else
        delay(1);
    
    Wire.beginTransmission(deviceAddress);
    
    for(int i = 0; i < len; ++i)
    {
        v = (data[i] & 0xff);
        if (cmds)
        {
            if (v < 0x10)
            {
                Serial.print(F("0"));
            }
            Serial.print(v, HEX);
        }
        else
        {
            delay(5);
        }
        Wire.write(byte(v & 0xff));
        if (cmds)
        {
            Serial.print(F(" "));
        }
        else
        {
            delay(1);
        }
    }
    
    //5.5 CRC of the I2C and RF frame ISO/IEC 13239. The initial register content shall be 0x6363
    int chksum =  crcsum((unsigned char*) data, len, 0x6363 );
    
    v = chksum & 0xff;
    if (cmds)
    {
        if (v < 0x10)
        {
            Serial.print(F("0"));
        }
        Serial.print(v, HEX);
    }
    else
    {
        delay(1);
    }
    
    Wire.write(byte(v & 0xff));
    
    //EOD field
    v = (chksum >> 8) & 0xff;
    if (cmds)
    {
        if (v < 0x10)
        {
            Serial.print(F("0"));
        }
        Serial.print(v, HEX);
    }
    else
    {
        delay(1);
    }
    Wire.write(byte(v & 0xff));
    
    err = Wire.endTransmission();
    if (cmds)
    {
        Serial.print(F("\r\n"));
    }
    else
    {
        delay(1);
    }
    //TODO does this really work?
    if (err != 0)
    {
        Serial.print(F("write err: "));
        Serial.print(err, HEX);
    }
}

//==============================================================================
void M24SR::updateBinaryNdefMsgLen0()
{
    if (verbose)
    {
        Serial.print(F("\r\nupdateBinary_NdefMsgLen0"));
    }
    uint8_t len0[] = "\x00\x00";
    sendApdu(0x00, INS_UPDATE_BINARY, 0x00, 0x00, 0x02, len0);
    receiveResponse(2 + 3);
}
// //==============================================================================
// void M24SR::writeSampleMsg(uint8_t msgNo) {
//   if (verbose) {
//         Serial.print(F("\r\nwriteSampleMsg "));
//         Serial.print(msgNo, DEC);
//         Serial.println("");
//    }
//    byte len = 0;
//    selectFile_NDEF_App();
//    selectFile_NDEF_file();
//    updateBinary_NdefMsgLen0();
//
//    switch(msgNo) {
//    case 0:
//      len = 0x45;
//      memcpy_P(&data[0], &SAMPLE_NDEF_message1[0], len);
//      break;
//    case 1:
//      len = 0x11;
//      memcpy_P(&data[0], &SAMPLE_NDEF_message2[0], len);
//      break;
//    case 2:
//      len = 0x13;
//      memcpy_P(&data[0], &SAMPLE_NDEF_message3[0], len);
//      break;
//    default:
//      break;
//    }
//    updateBinary(data, len);
//    receiveResponse(2 + 3);
//
//    updateBinaryLen(len);
//    receiveResponse(2 + 3);
//    sendDESELECT();
// }
// //==============================================================================
NdefMessage* M24SR::getNdefMessage()
{
    unsigned int len = 0;
    
    selectFileNdefApp();
    selectFileNdefFile();
    //Read NDEF message length 00 B0 00 00 02
    uint16_t ndef_len = getNdefMessageLength();
    if (verbose)
    {
        Serial.print(F("\r\nndef_len: "));
        Serial.println(ndef_len, DEC);
    }
    
    if (ndef_len < 255)
        sendApdu(0x00, INS_READ_BINARY, 0x00, 0x02, ndef_len & 0xff);
    else
    {
        Serial.println(F("TODO: ndef_len > 255"));
        return (NdefMessage*)NULL;
    }
    //uint8_t ndeflen[2];
    //ndeflen[0] = ndef_len >> 8;
    //ndeflen[1] = ndef_len & 0xff;
    //sendApdu(0x00, INS_READ_BINARY, 0x00, 0x00, ndeflen);
    receiveResponse((response[1] & 0xff) + 2 + 3);
    NdefMessage* pNdefMsg = new NdefMessage((byte *)&response[0], ndef_len);
    
    /* TODO
     NdefRecord rec = pNdefMsg->getRecord(0);
     
     String txt = rec.toString();
     rec.print();
     tft.fillScreen(ST7735_BLACK);
     tft.setCursor(0,0);
     
     tft.setTextColor(ST7735_WHITE);
     char szBuf[txt.length()+1];
     txt.getBytes((unsigned char*)szBuf, txt.length()+1);
     tft.print(szBuf);
     */
    //delete pNdefMsg;
    sendDESELECT();
    return pNdefMsg;
}

unsigned int M24SR::getNdefMessageLength()
{
    sendApdu(0x00, INS_READ_BINARY, 0x00, 0x00, 0x02);
    receiveResponse(2 + 2 + 3);
    return ((response[0] & 0xff) << 8) | (response[1] & 0xff);
}



//==============================================================================
void M24SR::print()
{
    Serial.print(F("\nM24SR GPO:"));
    Serial.println(gpoPin);
}
//==============================================================================
void M24SR::dumpHex(uint8_t* buffer, uint8_t len)
{
    char text[4];
    for(byte i = 0; i < len; ++i)
    {
        sprintf(text, "%02X \x00", (uint8_t)(*(buffer + i)));
        Serial.print(text);
        if ((i % 16) == 15)
        {
            Serial.println("");
        }
    }
}

void M24SR::displayNDefRecord()
{
    NdefMessage* pNDefMsg = this->getNdefMessage(); //read NDef message from memory
    if (pNDefMsg != NULL)
    {
        pNDefMsg->print();
        NdefRecord rec = pNDefMsg->getRecord(0);
        Serial.print(F("NDefRecord: "));
        rec.print();
        delete pNDefMsg;
    }
}

//==============================================================================
//EOF
