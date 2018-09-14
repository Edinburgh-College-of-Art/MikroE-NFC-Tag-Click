M24SR
============

Arduino library for ST M24SR NFC dynamic tag. This is specifically aimed at the MikroElektronika Tag Click which uses the M24SR with I2C and 3.3v power.

## Installation

Copy these folders from the repository into your Arduino library folder. Open the Arduino IDE and you should see under Examples the M24SR library.

- M24SR
- crc16
- PN532
- NDEF

## Libraries

The M24SR Library is dependant on the a few libraries. These libraries have been included and should be copied into your Arduino Library. If these libraries are already present then there may be a clash of versions. Try using the latest version of the library and, failing that, simply copy these over. You will probably need to do a little library management if that is the case.

- M24SR
- crc16
- PN532
- NDEF

# Resources

- [AN4433 Storing data into the NDEF memory of M24SR](http://www.st.com/web/en/resource/technical/document/application_note/DM00105043.pdf])
- [M24SR Datasheet](http://www.st.com/st-web-ui/static/active/en/resource/technical/document/datasheet/DM00097458.pdf)
- Use of printing via F(), which will use up more program memory. [F Macro](https://www.baldengineer.com/arduino-f-macro.html)
- [Tag Click User Manual](https://download.mikroe.com/documents/add-on-boards/click/nfc-tag/nfc-tag-click-manual-v100.pdf)
