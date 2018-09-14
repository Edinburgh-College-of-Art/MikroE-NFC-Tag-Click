/*  Example: NDefWrite
 *
 *  Write a message to an M24SR NFC Tag
 *
 * Pinout:
 *  -------------------------------------------------------------------------------
 *  M24SR             -> Arduino / resistor / antenna
 *  -------------------------------------------------------------------------------
 *  1 RF disable      -> not used
 *  2 AC0 (antenna)   -> Antenna
 *  3 AC1 (antenna)   -> Antenna
 *  4 VSS (GND)       -> Arduino Gnd
 *  5 SDA (I2C data)  -> Arduino A4 (SDA Pin)
 *  6 SCK (I2C clock) -> Arduino A5 (SCL Pin)
 *  7 GPO             -> Arduino D7 + Pull-Up resistor (>4.7kOhm) to VCC
 *  8 VCC (2...5V)    -> Arduino 3.3V
 *  -------------------------------------------------------------------------------
 */
//==============================================================================
#include <M24SR.h>
//==============================================================================
#define gpo_pin 7
//==============================================================================
M24SR m24sr(gpo_pin);
//==============================================================================
const char URI[] PROGMEM = "https://github.com/Edinburgh-College-of-Art/";
//==============================================================================
void setup()
{
   Serial.begin(9600);
   m24sr.setup();
   displayFreeRAM();
   NdefMessage message = NdefMessage();
   message.addUriRecord(&URI[0]);
   displayFreeRAM();
   m24sr.writeNdefMessage(&message);
   Serial.print(F("\r\nUse NFC phone/reader to read out NFC tag content!"));
}

void loop()
{
}

//==============================================================================
// http://playground.arduino.cc/Code/AvailableMemory
int freeRam ()
{
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void displayFreeRAM()
{
  Serial.print(F("\r\nfree RAM: "));
  Serial.println(freeRam(), DEC);
}
