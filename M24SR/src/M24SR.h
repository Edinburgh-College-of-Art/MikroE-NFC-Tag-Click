/* Arduino library for ST M24SR Dynamic NFC/RFID tag IC with EEPROM, NFC Forum Type 4 Tag and I2C interface
 (c) 2014 by ReNa http://regnerischernachmittag.wordpress.com/

  Edited by mhamilt on 14/10/2018

  mhamilt: most edits involve formatting of code. I am quite particular with the look and layout
           and, ideally, code should serve to be informative as well as functional.

           Naming conventions have been fixed, there was previously a mix of camel case, underscores
           and unhealthy combination of the two.

           - NDEF is stylised as Ndef throughout a la the NDEF library. So, if your code is not working, watch out for typos
           - Functions that are only used internally have been made private.
           -

           I am still uncertain what the real function of some methods are with respect to the low level
           interfacing with the M24SR. The answers obviously lie in the ST documentation linked below.
           If you have any suggestions for wording of class method documentation, fire it over.

           mhamilt.github.io
 */

/*

 TODO
 ----
 - clean-up code and add TODOs
 - test: > 1 NDef record in NDef message
 - read/write data (without NDef classes)
 - what to do with writeSampleMsg?
 - ndef_len > 255
 - password handling
 - dynamic data buffer
 - if (len > BUFFER_LENGTH - 8) update for-loop

 INFO
 ----

 NOTE: The Arduino Wire library only has a 32 character buffer, so that is the maximun we can send using Arduino. This buffer includes the two address bytes which limits our data payload to 30 bytes

 AN4433 Storing data into the NDEF memory of M24SR http://www.st.com/web/en/resource/technical/document/application_note/DM00105043.pdf
 Datasheet http://www.st.com/st-web-ui/static/active/en/resource/technical/document/datasheet/DM00097458.pdf

 Use of printing via F(), which will use up more program memory.
 https://www.baldengineer.com/arduino-f-macro.html
 */
//==============================================================================
#ifndef M24SR_h
#define M24SR_h
//==============================================================================
// Standard Libraries

#include <Arduino.h>
#include <Wire.h>
// #include <avr/pgmspace.h> //prog_char
//==============================================================================
// Additional Libraries

#include <NfcAdapter.h> // from NDEF library (include NDefMessage)
#include <crc16.h>
// #include <PN532.h> //
//==============================================================================
// Program Memory constants
const char FILE_SYSTEM[] PROGMEM = "\xE1\x01";
const char AID_NDEF_TAG_APPLICATION2[] PROGMEM = "\xD2\x76\x00\x00\x85\x01\x01";
const char DEFAULT_PASSWORD[] PROGMEM = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";

//==============================================================================
// UNUSED CONSTANTS

// const char FILE_CC[] PROGMEM = "\xE1\x03";
// const char SAMPLE_NDEF_message1[] PROGMEM = "\x91\x01\x10\x55\x01\x73\x74\x2E\x63\x6F\x6D\x2F\x6E\x66\x63\x2D\x72\x66\x69\x64\x54\x18\x16\x73\x74\x2E\x63\x6F\x6D\x3A\x6D\x32\x34\x73\x72\x5F\x70\x72\x6F\x70\x72\x69\x65\x74\x61\x72\x73\x4D\x32\x34\x53\x52\x20\x70\x72\x6F\x70\x72\x69\x65\x74\x61\x72\x79\x20\x64\x61\x74\x61";
//
// // 0x45
// //const char SAMPLE_NDEF_message2[] PROGMEM = "\xd1\x01\x0a\x55\x03\x6e\x6f\x6b\x69\x61\x2e\x63\x6f\x6d";
//
// // 0x0F www.st.com
// //const char SAMPLE_NDEF_message2[] PROGMEM = "\xD1\x01\x0b\x55\x01\x77\x77\x77\x2E\x73\x74\x2E\x63\x6F\x6D";
//
// // 0x0c www.nfc.com
// //const char SAMPLE_NDEF_message2[] PROGMEM = "\xD1\x01\x08\x55\x01\x6E\x66\x63\x2E\x63\x6F\x6D";
//
// // 0x11 phone: +358 9 1234567
// const char SAMPLE_NDEF_message2[] PROGMEM = "\xD1\x01\x0D\x55\x05\x2B\x33\x35\x38\x39\x31\x32\x33\x34\x35\x36\x37";
//
// // 0x13 txt: "hello, world"
// const char SAMPLE_NDEF_message3[] PROGMEM = "\xD1\x01\x0F\x54\x02\x65\x6e\x68\x65\x6c\x6c\x6f\x2c\x20\x77\x6f\x72\x6c\x64";
//
// // EMPTY Tag
// // 00 03    D0 00 00
//==============================================================================
// //const char SELECT_NDEF_Tag_Application2[] PROGMEM = "\x00\xA4\x04\x00\x07\xD2\x76\x00\x00\x85\x01\x01";
// //const char SELECT_CC[] PROGMEM = "\x00\xA4\x00\x0C\x02\xE1\x03";
// //const char SELECT_NDEF_file[] PROGMEM = "\x00\xA4\x00\x0C\x02\x00\x01";
// //const char SELECT_system_file[] PROGMEM = "\x00\xA4\x00\x0C\x02\xE1\x01";
// //const char UPDATE_BINARY_NDEF_MSG_LEN0[] PROGMEM = "\x00\xD6\x00\x00\x02\x00\x00";
// //const char READ_BINARY_LENGTH[] PROGMEM = "\x02\x00\xB0\x00\x00\x02";
// //const char READ_BINARY[] PROGMEM = "\x02\x00\xB0\x00\x00";
// //const char READ_BINARY_DEF_MSG[] PROGMEM = "\x02\x00\xB0\x00\x02";
//==============================================================================

/** Class to interface with the ST M24SR chip used in NFC Tags. */
class M24SR
{
public:
    //==========================================================================
    M24SR(uint8_t gpoArduinoPin);
    ~M24SR();
    //==========================================================================
    /**
     print the GPO pin on the Arduino
     */
    void print();
    /**
     Print out to Serial the NDef record of NFC Tag
     */
    void displayNDefRecord();
    //==========================================================================
    /** Initialise the class, run in Setup() after Serial has been initialised.
        This is likely not the best way to achieve this. */
    void setup();
    //==========================================================================
    boolean checkGPOTrigger();
    unsigned int getNdefMessageLength();
    boolean verifyI2cPassword();
    void checkCRC(char* data, int len);
    void selfTest();
    void writeSampleMsg(uint8_t msgNo);
    void displaySystemFile();
    void dumpHex(uint8_t* buffer, uint8_t len);
    int receiveResponse(unsigned int len);
    //==========================================================================
    void getUID();
    NdefMessage* getNdefMessage();
    void writeNdefMessage(NdefMessage* message);

    //TODO boolean verifyI2cPassword(uint8_t* pwd);
    //TODO boolean setI2cPassword(uint8_t* old_password, uint8_t* new_password);
private:
  //==========================================================================
  // Private methods

  /** Write to the General Purpose Output */
  void writeGPO(uint8_t data);

  void sendDESELECT();
  void sendSBLOCK(uint8_t sblock);
  void updateBinary(char* data, uint8_t len);
  void updateBinary(unsigned int offset, char* data, uint8_t len);
  void updateBinaryLen(int len);
  void updateBinaryNdefMsgLen0();
  void selectFileNdefFile();
  void selectFileNdefApp();
  void sendCommand(/*char* data,*/ int len);
  void sendCommand(/*char* data,*/ int len, boolean setPCB);
  /** Application Protocol Data Unit */
  void sendApdu(uint8_t CLA, uint8_t INS, uint8_t P1, uint8_t P2, uint8_t Lc, uint8_t* Data);
  void sendApdu(uint8_t CLA, uint8_t INS, uint8_t P1, uint8_t P2, uint8_t Le);
  void sendApdu_P(uint8_t CLA, uint8_t INS, uint8_t P1, uint8_t P2, uint8_t Lc, const char* Data);
private:
    //==========================================================================
    uint8_t gpoPin;
    uint8_t lastGPO;
    uint8_t deviceAddress;
    boolean sendGetI2cSession;
    uint8_t err;
    uint8_t blockNo;
    uint8_t responseLength;
    uint8_t* response;
    //==========================================================================
    // Class constants
    const char CMD_GETI2CSESSION = 0x26;
    const char CMD_KILLRFSESSION = 0x52;
    const char INS_SELECT_FILE = 0xA4;
    const char INS_UPDATE_BINARY = 0xD6;
    const char INS_READ_BINARY = 0xB0;
    const char INS_VERIFY = 0x20;

public:
    //==========================================================================
    boolean verbose;
    boolean cmds;
    char data[100]; //TODO dynamic buffer
};

#endif
