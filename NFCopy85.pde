/* 
 *  Author: Salvador Mendoza (salmg.net)
 *  Project: NFCopy85
 *  More info: https://salmg.net/2019/06/16/nfcopy85/ 
*/
#define PN532_SCK  (0)
#define PN532_MOSI (2)
#define PN532_SS   (3)
#define PN532_MISO (1)
#define LED   (4)

#include "tinyPN532.h" //shortened version of PN532 Adafruit library

const uint8_t ppse[] = {0x8E,0x6F,0x23,0x84,0x0E,0x32,0x50,0x41,0x59,0x2E,0x53,0x59,0x53,0x2E,0x44,0x44,0x46,0x30,0x31,0xA5,0x11,0xBF,0x0C,0x0E,0x61,0x0C,0x4F,0x07,0xA0,0x00,0x00,0x00,0x03,0x10,0x10,0x87,0x01,0x01,0x90,0x00};
const uint8_t visa[] = {0x8E,0x6F,0x1E,0x84,0x07,0xA0,0x00,0x00,0x00,0x03,0x10,0x10,0xA5,0x13,0x50,0x0B,0x56,0x49,0x53,0x41,0x20,0x43,0x52,0x45,0x44,0x49,0x54,0x9F,0x38,0x03,0x9F,0x66,0x02,0x90,0x00};
const uint8_t processing[] = {0x8E,0x80,0x06,0x00,0x80,0x08,0x01,0x01,0x00,0x90,0x00};
const uint8_t card [] = {
  0x8E,0x70,0x15,0x57,0x13,
  0x46,0x50,0x98,0x29,0x81,0x62,0x29,0x58,0xd2,0x40,0x32,0x01,0x14,0x69,0x00,0x00,0x13,0x83,
  0x44,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xd0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, //Card/token number
  0x8f,0x90,0x00
};
const uint8_t finish[] = {0x8E,0x6f,0x00};

uint8_t Adafruit_PN532::setDataTarget(uint8_t cmd){//communication control - from code to PN532
  if (cmd == 1){
    if (!sendCommandCheckAck(ppse, sizeof(ppse)))
      return false;
  }else if(cmd == 2){
    if (!sendCommandCheckAck(visa, sizeof(visa)))
      return false;
  }else if(cmd == 3){
    if (!sendCommandCheckAck(processing, sizeof(processing)))
      return false;
  }else if(cmd == 4){
    if (!sendCommandCheckAck(card, sizeof(card)))
      return false;
  }else {
    if (!sendCommandCheckAck(finish, sizeof(finish)))
      return false;
  }
  // read data packet
  readdata(pn532_packetbuffer, 8);                 //unnecessary but could open the door for new processing projects
}

void setup(){
  pinMode(LED, OUTPUT);
  delay(300);
  blink(LED, 100, 1);
  nfc.begin();                                     //initialize the PN532
  uint32_t versiondata = nfc.getFirmwareVersion(); //check PN532 version(ping!)
  if (!versiondata) {
    while(1){
      blink(LED, 50, 6);
      delay(200);
    }
  }
  nfc.setPassiveActivationRetries(0xFF);           //infinite num of retries
  nfc.SAMConfig();                                 //type of cards
}

void setData(int value){                           //communication function!
   nfc.setDataTarget(value);                       //send data to PN532
   nfc.getDataTarget();                            //request data from PN532
}

void loop(){
  blink(LED, 150, 3);
  nfc.AsTarget();                                  //in every loop, run as-target
  nfc.getDataTarget();                             //clear the PN532 buffer 
  for (int a=1; a<6; a++){
    setData(a);
    if (a == 5)                                    //bacause we already sent the needed data, 
      setData(a);                                  //just sending couple errors to force the end of transaction loop
  }
  blink(LED, 50, 2);
  delay(1000);
}
