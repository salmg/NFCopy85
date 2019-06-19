#define PN532_PREAMBLE                      (0x00)
#define PN532_STARTCODE2                    (0xFF)
#define PN532_POSTAMBLE                     (0x00)

#define PN532_HOSTTOPN532                   (0xD4)
//#define PN532_PN532TOHOST                   (0xD5)

// PN532 Commands
#define PN532_COMMAND_GETFIRMWAREVERSION    (0x02)
#define PN532_COMMAND_SAMCONFIGURATION      (0x14)
#define PN532_COMMAND_RFCONFIGURATION       (0x32)
#define PN532_COMMAND_INLISTPASSIVETARGET   (0x4A)
#define PN532_COMMAND_TGGETDATA             (0x86)
#define PN532_COMMAND_TGSETDATA             (0x8E)
#define PN532_COMMAND_TGINITASTARGET        (0x8C)

#define PN532_SPI_STATREAD                  (0x02)
#define PN532_SPI_DATAWRITE                 (0x01)
#define PN532_SPI_DATAREAD                  (0x03)
#define PN532_SPI_READY                     (0x01)

#define PN532_MIFARE_ISO14443A              (0x00)

class Adafruit_PN532{
 public:
  Adafruit_PN532(uint8_t clk, uint8_t miso, uint8_t mosi, uint8_t ss);  // Software SPI
  Adafruit_PN532(uint8_t irq, uint8_t reset);  // Hardware I2C
  Adafruit_PN532(uint8_t ss);  // Hardware SPI
  void begin(void);
  
  // Generic PN532 functions
  bool     SAMConfig(void);
  uint32_t getFirmwareVersion(void);
  bool     sendCommandCheckAck(uint8_t *cmd, uint8_t cmdlen, uint16_t timeout = 1000);  
  bool     setPassiveActivationRetries(uint8_t maxRetries);
  
  // ISO14443A functions
  bool readPassiveTargetID(uint8_t cardbaudrate, uint8_t * uid, uint8_t * uidLength, uint16_t timeout = 0); //timeout 0 means no timeout - will block forever.
  uint8_t AsTarget();
  uint8_t getDataTarget();
  uint8_t setDataTarget(uint8_t cmd);
  
 private:
  uint8_t _ss, _clk, _mosi, _miso;
  uint8_t _irq, _reset;
  uint8_t _uid[7];       // ISO14443A uid
  uint8_t _uidLen;       // uid len
  uint8_t _key[6];       // Mifare Classic key
  uint8_t _inListedTag;  // Tg number of inlisted tag.
  bool    _usingSPI;     // True if using SPI, false if using I2C.

  // Low level communication functions that handle both SPI and I2C.
  void readdata(uint8_t* buff, uint8_t n);
  void writecommand(uint8_t* cmd, uint8_t cmdlen);
  bool isready();
  bool waitready(uint16_t timeout);
  bool readack();

  // SPI-specific functions.
  void    spi_write(uint8_t c);
  uint8_t spi_read(void);
};
Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

byte pn532ack[] = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};
byte pn532response_firmwarevers[] = {0x00, 0xFF, 0x06, 0xFA, 0xD5, 0x03};
// Hardware SPI-specific configuration:
#ifdef SPI_HAS_TRANSACTION
    #define PN532_SPI_SETTING SPISettings(1000000, LSBFIRST, SPI_MODE0)
#else
    #define PN532_SPI_CLOCKDIV SPI_CLOCK_DIV16
#endif

#define PN532_PACKBUFFSIZ 64
byte pn532_packetbuffer[PN532_PACKBUFFSIZ];

#ifndef _BV
    #define _BV(bit) (1<<(bit))
#endif
/**************************************************************************/
/*!
    @brief  Instantiates a new PN532 class using software SPI.

    @param  clk       SPI clock pin (SCK)
    @param  miso      SPI MISO pin
    @param  mosi      SPI MOSI pin
    @param  ss        SPI chip select pin (CS/SSEL)
*/
/**************************************************************************/
Adafruit_PN532::Adafruit_PN532(uint8_t clk, uint8_t miso, uint8_t mosi, uint8_t ss):
  _clk(clk),
  _miso(miso),
  _mosi(mosi),
  _ss(ss),
  _irq(0),
  _reset(0),
  _usingSPI(true)
{
  pinMode(_ss, OUTPUT);
  digitalWrite(_ss, HIGH); 
  pinMode(_clk, OUTPUT);
  pinMode(_mosi, OUTPUT);
  pinMode(_miso, INPUT);
}
bool Adafruit_PN532::readPassiveTargetID(uint8_t cardbaudrate, uint8_t * uid, uint8_t * uidLength, uint16_t timeout) {
  pn532_packetbuffer[0] = PN532_COMMAND_INLISTPASSIVETARGET;
  pn532_packetbuffer[1] = 1;  // max 1 cards at once (we can set this to 2 later)
  pn532_packetbuffer[2] = cardbaudrate;

  if (!sendCommandCheckAck(pn532_packetbuffer, 3, timeout))
    return 0x0;  // no cards read
  

  readdata(pn532_packetbuffer, 20);

  if (pn532_packetbuffer[7] != 1)
    return 0;

  uint16_t sens_res = pn532_packetbuffer[9];
  sens_res <<= 8;
  sens_res |= pn532_packetbuffer[10];

  /* Card appears to be Mifare Classic */
  *uidLength = pn532_packetbuffer[12];

  for (uint8_t i=0; i < pn532_packetbuffer[12]; i++)
    uid[i] = pn532_packetbuffer[13+i];
  
  return 1;
}
bool Adafruit_PN532::waitready(uint16_t timeout) {
  uint16_t timer = 0;
  while(!isready()) {
    if (timeout != 0) {
      timer += 10;
      if (timer > timeout) 
        return false;
    }
    delay(10);
  }
  return true;
}
void Adafruit_PN532::readdata(uint8_t* buff, uint8_t n) {
  // SPI write.
  digitalWrite(_ss, LOW);
  delay(2);
  spi_write(PN532_SPI_DATAREAD);

  for (uint8_t i=0; i<n; i++) {
    delay(1);
    buff[i] = spi_read();
  }

  digitalWrite(_ss, HIGH);
}

bool Adafruit_PN532::readack() {
  uint8_t ackbuff[6];
  readdata(ackbuff, 6);
  return (0 == strncmp((char *)ackbuff, (char *)pn532ack, 6));
}

uint8_t Adafruit_PN532::spi_read(void) {
  int8_t i, x = 0;
  //x = 0;
  // Software SPI read.
  digitalWrite(_clk, HIGH);

  for (i=0; i<8; i++) {
    if (digitalRead(_miso)) 
      x |= _BV(i);
    digitalWrite(_clk, LOW);
    digitalWrite(_clk, HIGH);
  }

  return x;
}
bool Adafruit_PN532::isready() {
  digitalWrite(_ss, LOW);
  delay(2);
  spi_write(PN532_SPI_STATREAD);
  // read byte
  uint8_t x = spi_read();

  digitalWrite(_ss, HIGH);
  #ifdef SPI_HAS_TRANSACTION
    if (_hardwareSPI) SPI.endTransaction();
  #endif

  // Check if status is ready.
  return x == PN532_SPI_READY;
}
void Adafruit_PN532::spi_write(uint8_t c) {
  int8_t i;
  digitalWrite(_clk, HIGH);

  for (i=0; i<8; i++) {
    digitalWrite(_clk, LOW);
    if (c & _BV(i)) {
      digitalWrite(_mosi, HIGH);
    } else {
      digitalWrite(_mosi, LOW);
    }
    digitalWrite(_clk, HIGH);
  }
}
void Adafruit_PN532::writecommand(uint8_t* cmd, uint8_t cmdlen) {
  uint8_t checksum;
  cmdlen++;
  digitalWrite(_ss, LOW);
  delay(2);     // or whatever the delay is for waking up the board
  spi_write(PN532_SPI_DATAWRITE);

  checksum = PN532_PREAMBLE + PN532_PREAMBLE + PN532_STARTCODE2;
  spi_write(PN532_PREAMBLE);
  spi_write(PN532_PREAMBLE);
  spi_write(PN532_STARTCODE2);

  spi_write(cmdlen);
  spi_write(~cmdlen + 1);

  spi_write(PN532_HOSTTOPN532);
  checksum += PN532_HOSTTOPN532;

  for (uint8_t i=0; i<cmdlen-1; i++) {
    spi_write(cmd[i]);
    checksum += cmd[i];
  }

  spi_write(~checksum);
  spi_write(PN532_POSTAMBLE);
  digitalWrite(_ss, HIGH);
}
bool Adafruit_PN532::sendCommandCheckAck(uint8_t *cmd, uint8_t cmdlen, uint16_t timeout) {
  uint16_t timer = 0;
  // write the command
  writecommand(cmd, cmdlen);

  // Wait for chip to say its ready!
  if (!waitready(timeout)) 
    return false;

  // read acknowledgement
  if (!readack()) 
    return false;

  if (!waitready(timeout)) 
    return false;

  return true; // ack'd command
}
uint32_t Adafruit_PN532::getFirmwareVersion(void) {
  uint32_t response;
  pn532_packetbuffer[0] = PN532_COMMAND_GETFIRMWAREVERSION;

  if (! sendCommandCheckAck(pn532_packetbuffer, 1))
    return 0;

  // read data packet
  readdata(pn532_packetbuffer, 12);

  // check some basic stuff
  if (0 != strncmp((char *)pn532_packetbuffer, (char *)pn532response_firmwarevers, 6)) 
    return 0;
  
  int offset = _usingSPI ? 6 : 7;  // Skip a response byte when using I2C to ignore extra data.
  response = pn532_packetbuffer[offset++];
  response <<= 8;
  response |= pn532_packetbuffer[offset++];
  response <<= 8;
  response |= pn532_packetbuffer[offset++];
  response <<= 8;
  response |= pn532_packetbuffer[offset++];

  return response;
}
void Adafruit_PN532::begin() {
  // SPI initialization
  digitalWrite(_ss, LOW);
  delay(1000);

  // not exactly sure why but we have to send a dummy command to get synced up
  pn532_packetbuffer[0] = PN532_COMMAND_GETFIRMWAREVERSION;
  sendCommandCheckAck(pn532_packetbuffer, 1);
  // ignore response!

  digitalWrite(_ss, HIGH);
}

bool Adafruit_PN532::setPassiveActivationRetries(uint8_t maxRetries) {
  pn532_packetbuffer[0] = PN532_COMMAND_RFCONFIGURATION;
  pn532_packetbuffer[1] = 5;    // Config item 5 (MaxRetries)
  pn532_packetbuffer[2] = 0xFF; // MxRtyATR (default = 0xFF)
  pn532_packetbuffer[3] = 0x01; // MxRtyPSL (default = 0x01)
  pn532_packetbuffer[4] = maxRetries;
  if (! sendCommandCheckAck(pn532_packetbuffer, 5))
    return 0x0;  // no ACK

  return 1;
}

bool Adafruit_PN532::SAMConfig(void) {
  pn532_packetbuffer[0] = PN532_COMMAND_SAMCONFIGURATION;
  pn532_packetbuffer[1] = 0x01; // normal mode;
  pn532_packetbuffer[2] = 0x14; // timeout 50ms * 20 = 1 second
  pn532_packetbuffer[3] = 0x01; // use IRQ pin!

  if (!sendCommandCheckAck(pn532_packetbuffer, 4))
    return false;
  // read data packet
  readdata(pn532_packetbuffer, 8);

  int offset = _usingSPI ? 5 : 6;
  return (pn532_packetbuffer[offset] == 0x15);
}


uint8_t Adafruit_PN532::AsTarget() {
   pn532_packetbuffer[0] = 0x8C;
    uint8_t target[] = {
    0x8C, // INIT AS TARGET
    0x00, // MODE -> BITFIELD
    0x08, 0x00, //SENS_RES - MIFARE PARAMS
    0xdc, 0x44, 0x20, //NFCID1T
    0x60, //SEL_RES
    0x01,0xfe, //NFCID2T MUST START WITH 01fe - FELICA PARAMS - POL_RES
    0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,
    0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,//PAD
    0xff,0xff, //SYSTEM CODE
    0xaa,0x99,0x88,0x77,0x66,0x55,0x44,0x33,0x22,0x11,0x01,0x00, //NFCID3t MAX 47 BYTES ATR_RES
    0x0d,0x52,0x46,0x49,0x44,0x49,0x4f,0x74,0x20,0x50,0x4e,0x35,0x33,0x32 //HISTORICAL BYTES
  };
  if (!sendCommandCheckAck(target, sizeof(target)))
    return false;

  // read data packet
  readdata(pn532_packetbuffer, 8);
}
/**************************************************************************/
/*!
    @brief  retrieve response from the emulation mode

    @param  cmd    = data
    @param  cmdlen = data length
*/
/**************************************************************************/
uint8_t Adafruit_PN532::getDataTarget() {
  pn532_packetbuffer[0] = 0x86;
  if (!sendCommandCheckAck(pn532_packetbuffer, 1, 1000)){
    return false;
  }
  // read data packet
  readdata(pn532_packetbuffer, 64); //64
  return true;
}
void blink(int pin, int msdelay, int times){
  for (int i = 0; i < times; i++){
    digitalWrite(pin, HIGH);
    delay(msdelay);
    digitalWrite(pin, LOW);
    delay(msdelay);
  }
}
