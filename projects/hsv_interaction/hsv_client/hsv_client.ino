/**
 * MIT License
 * 
 * Copyright (c) 2017 Ebbe Strandell
 * https://github.com/estrandell/arduino
 */

#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include "colorLib.h"


/*
 * Communication over Software Serial for setup and debugging 
 * This will mess up led stip stuff due to interupts
 */
//#define USE_SOFTWARE_SERIAL 
#ifdef USE_SOFTWARE_SERIAL
  #include <SoftwareSerial.h>
  #define SERIAL_TX 10      // To RX  on ESP8266
  #define SERIAL_RX 11      // To TX  on ESP8266

  SoftwareSerial mySerial(SERIAL_RX, SERIAL_TX);
#endif


#define SERIAL_RESET 9          // To Res on ESP8266
#define I2C_ADDRESS_ESP8266 8   // Address


/* 
 * IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
 * pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
 * and minimize distance between Arduino and first pixel.  Avoid connecting
 * on a live circuit...if you must, connect GND first.
 */
#define DATA_PIN 12
#define NUM_PIXELS 16
#define BRIGTHNESS 40 

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, DATA_PIN, NEO_GRB + NEO_KHZ800);
HSV      pixels[NUM_PIXELS];
HSV      targetHSV;
int16_t  targetPixel = 0;


void receivedEsp8266Event(int howMany) {
  String s = "";
  while (Wire.available()) {
    //char c = Wire.read();    // receive byte as a character
    //int x = Wire.read();    // receive byte as an integer
    s += (char)Wire.read();
  }
  int h = s.toInt();

  targetHSV   = HSV(h/360.0f, 1.0f, 1.0f, 1.0f);
  targetPixel = random(strip.numPixels());
}

void setup() {
  
  /** Setup serial communication */
  Serial.begin(9600);
  Serial.setTimeout(50);
  Serial.println("Starting program");
  randomSeed(analogRead(0));
  #ifdef USE_SOFTWARE_SERIAL
    mySerial.begin(9600);
    mySerial.setTimeout(50);
  #endif

  /** Join i2c bus on address #8 */
  Wire.begin(I2C_ADDRESS_ESP8266);
  Wire.onReceive(receivedEsp8266Event);

  /** Init led strip */
  strip.begin();
  strip.setBrightness(BRIGTHNESS);
  strip.show(); // Initialize all pixels to 'off'
  
  targetHSV   = HSVClockRandom();
  targetPixel = random(strip.numPixels());
}


void loop() {

  /** If using SoftwareSerial */
  handleSerialCommunication();

  /** Update pixels */
  colorDropHSV(25);
}

/** If using SoftwareSerial */
void handleSerialCommunication(){

#ifdef USE_SOFTWARE_SERIAL
  /** Read ESP8266 output */
  while (mySerial.available()) {
    String msg = mySerial.readStringUntil('\r');
    if (msg.length() > 0){
      Serial.print(msg);
    }
  }

  /** Send command from monitor to ESP8266 */
  if (Serial.available()){
    String msg = Serial.readStringUntil('\r') + '\r';
    Serial.readStringUntil('\n'); //read of everything else 
    mySerial.print(msg);
  }
#endif
}

/** Apply color magic */
void colorDropHSV(uint8_t wait){

    // set main color
    pixels[targetPixel] = LerpHSV(pixels[targetPixel], targetHSV, 0.05f);

    static const unsigned int numPixels = NUM_PIXELS;
    for (int i=0; i<numPixels;i++){
      if (i != targetPixel){

        int a = (i-targetPixel)%numPixels;
        int b = (targetPixel-i)%numPixels;
        if ( a < b)
          pixels[i] = LerpHSV(pixels[i], pixels[(i-1+numPixels)%numPixels], 0.3f);
        else
          pixels[i] = LerpHSV(pixels[i], pixels[(i+1+numPixels)%numPixels], 0.3f);
        
        pixels[i].v = Clamp01(pixels[i].v - 0.01);
      }

      strip.setPixelColor(i, pixels[i].ToRgb().ToUInt32());

      if (abs(pixels[targetPixel].h - targetHSV.h) < 0.01f){
        targetHSV   = HSVClockRandom();
        targetPixel = random(numPixels);
      }
    }

    strip.show();
    delay(wait);
}

