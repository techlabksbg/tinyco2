/*
 * To program: Examples -> Arduino ISP, write to UNO
 * Set ATTINY85, to 8MHz, Arduino as ISP programmer
 * 
 * Install ATTiny Boards from  https://raw.githubusercontent.com/damellis/attiny/ide-1.6.x-boards-manager/package_damellis_attiny_index.json
 * 
 * Burn Bootloader (for 8MHz, otherwise its just 1MHz)
 * Upload Sketch...
 */


#include <Adafruit_NeoPixel.h>
#define PIN            4
#define NUMPIXELS      4
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// https://github.com/avishorp/TM1637
#include <TM1637Display.h>
#define CLK 3
#define DIO 2
TM1637Display display(CLK, DIO);

#include <SoftwareSerial.h> 
#define RXpin 0
#define TXpin 1
SoftwareSerial myserial(RXpin, TXpin);  //rx, tx

int demoCounter = 400;

void clearSerialBuffer() {
  while (myserial.available()>0) {
    myserial.read();
    delay(1);
  }
}

unsigned char getCheckSum(unsigned char *packet) {
  unsigned char checksum = 0;
  for (unsigned char i = 1; i < 8; i++) {
    checksum += packet[i];
  }
  return (255-checksum)+1;
}

unsigned int readCO2UART(){
  if (demoCounter<2000) {
    demoCounter+=3+demoCounter/100;
    return demoCounter;
  }
  unsigned char cmd[9] = {0xFF,0x01,0x86,0x00,0x00,0x00,0x00,0x00,0x79};
  unsigned char response[9]; // for answer
  unsigned int  ppm=0;
  clearSerialBuffer();
  myserial.write(cmd, 9); //request measure
  
  memset(response, 0, 9); // wait a second...
  for (int i=0; myserial.available() == 0 && i<50; i++) { 
    delay(20);
  }
  if (myserial.available() > 0) {
    myserial.readBytes(response, 9);
    if (response[8] == getCheckSum(response) && response[5]==0) { // Checksum and status
      ppm = ((unsigned int)response[2])<<8 | response[3];
//      temp = response[4] - 40;
    }
  }
  clearSerialBuffer();
  return ppm;
}

// Enables auto-calibration (every 24h the lowest co2-value is calibrated to 410 (or something like that))
void enableABC() {
  unsigned char cmd[9] = {0xFF,0x01,0x79,0x00,0x00,0x00,0x00,0x00,0x86};
  clearSerialBuffer();
  myserial.write(cmd, 9); //send command
}


void setup() {
  myserial.begin(9600);
  enableABC();
  display.setBrightness(0x07); // 0-7
  pixels.begin();
}
int r=0,g=20,b=0;

void loop() {
  unsigned int ppm = readCO2UART();
  display.showNumberDec(ppm, false);
  if (ppm>400) {
    r = ppm<655 ? 0 : (ppm<1675 ? (ppm-655)/4 : 255);  // 1675 max
    g = ppm>655 ? (ppm<1675 ? 255-(ppm-655)/4 : 0) : ppm-400;
    b = ppm>655 ? 0 : 655-ppm;
    if (ppm>2000) {
      demoCounter++;
      if (demoCounter%2==0) {
        r = 0;
        g = 0;
        b = 255;
      }
    }
    for (int i=0; i<4; i++) {
      pixels.setPixelColor(i, pixels.Color(r,g,b));
    }
    pixels.show();
  }
  if (demoCounter<2000) {
    delay(50);
  } else {
    delay(2000);
  }

}
