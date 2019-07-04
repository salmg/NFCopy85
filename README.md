# NFCopy85
NFCopy85 is a 10 dollars device to make replay attacks against NFC payment systems.

By default, the Adafruit PN532 library(https://github.com/adafruit/Adafruit-PN532) is too big for the ATtiny85 and some functions had to be edited for a better communication between the ATtiny85 and PN532 board. To establish the communication, I decided to use SPI. So I deleted all the parts in the library that were not needed for the SPI communication.

More details about this project: https://salmg.net/2019/06/16/nfcopy85/
