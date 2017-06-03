/**
 * MIT License
 * 
 * Copyright (c) 2017 Ebbe Strandell
 * https://github.com/estrandell/arduino
 */

#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <Time.h>
#include "colorLib.h"

enum WifiState{
  WIFI_INIT = 0,
  WIFI_CONNECTING,    // connecting to wifi
  WIFI_WAITING,       // waiting for instructions
  WIFI_CONNECTED      // connected and instructed
};


#define SERIAL_TX 10            // To RX  on ESP8266
#define SERIAL_RX 11            // To TX  on ESP8266
#define SERIAL_RESET 6          // To Res on ESP8266
#define I2C_ADDRESS 8           // Address on i2c bus
#define I2C_BUF_SIZE 32

/*
 * Communication over Software Serial for ESP8266 setup and debugging 
 * BE ADVISED: Enabling this messes up led strip stuff and I2C communication
 */
//#define USE_SOFTWARE_SERIAL
#ifdef USE_SOFTWARE_SERIAL
  #include <SoftwareSerial.h>
  SoftwareSerial mySerial(SERIAL_RX, SERIAL_TX);
  
  #define SoftSerialAvailable()           mySerial.available()          
  #define SoftSerialBegin(x)              mySerial.begin(x)
  #define SoftSerialPrint(x)              mySerial.print(x)
  #define SoftSerialReadStringUntil(x)    mySerial.readStringUntil(x)
  #define SoftSerialSetTimeout(x)         mySerial.setTimeout(x);
#else
  #define SoftSerialAvailable()           false          
  #define SoftSerialBegin(x)              
  #define SoftSerialPrint(x)
  #define SoftSerialReadStringUntil(x)    ""
  #define SoftSerialSetTimeout(x)         
#endif



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
HSV           pixels[NUM_PIXELS];
HSV           targetHSV;
int16_t       targetPixel = 0;
WifiState     wifiState = WIFI_INIT;
unsigned long wifiCmdTime = 0; 
unsigned long wifiTimeout = 30000;
unsigned long lastRequest = 0;

void onI2cReceived(int howMany) {
  String str = "";
  while (Wire.available()) {
    str += (char)Wire.read();
  }

  if (str.length() > 0){
    wifiCmdTime = millis();

    if (str.startsWith("wifi=")){
      int state = str.substring(5).toInt();
      wifiState = (state == 0 ? WIFI_CONNECTING : WIFI_WAITING);
    }
    else if (str.startsWith("h=")){
      int h       = str.substring(2).toInt();
      targetHSV   = HSV(h/360.0f, 1.0f, 1.0f, 1.0f);
      targetPixel = random(strip.numPixels());
      wifiState   = WIFI_CONNECTED;
    }
  }
}

void onI2cRequest() {
  static char buf[I2C_BUF_SIZE];

  if (lastRequest + 3000 > millis()){
    String s = "http://192.168.1.120/h=";
    s+= (int)(targetHSV.h*360);
    s.toCharArray(buf, s.length()+1);
    Wire.write(buf,I2C_BUF_SIZE);
    Serial.println(s);
    lastRequest = millis();
  }
}

void setup() {
  
  /** Setup serial communication */
  Serial.begin(9600);
  Serial.setTimeout(50);
  Serial.println("Starting program");
  randomSeed(analogRead(0));

  /** Setup software serial communication, if allowed */
  pinMode(SERIAL_TX, OUTPUT);
  pinMode(SERIAL_RX, INPUT_PULLUP);
  SoftSerialBegin(9600);
  SoftSerialSetTimeout(50);

  /** Join i2c bus on address #8 */
  Wire.begin(I2C_ADDRESS);
  Wire.onReceive(onI2cReceived);
  Wire.onRequest(onI2cRequest);

  /** Init led strip */
  strip.begin();
  strip.setBrightness(BRIGTHNESS);
  strip.show(); // Initialize all pixels to 'off'

  targetHSV   = HSVClockRandom();
  targetPixel = random(strip.numPixels());

  /** Reset ESP8266 */
  pinMode(SERIAL_RESET, OUTPUT);
  digitalWrite(SERIAL_RESET, HIGH);
  delay(100);
  resetWifi();
}

void loop() {

  /** If using SoftwareSerial */
  handleSerialCommunication();

  /** Is connection active */
  if (wifiState == WIFI_CONNECTED && wifiCmdTime + wifiTimeout < millis()){ //5sec
    wifiState = WIFI_WAITING;
  }

  /** Update pixels */
  /*
  switch(wifiState){
    case WIFI_INIT:       pulse(HSV_RED, 25);     break;
    case WIFI_CONNECTING: pulse(HSV_ORANGE,25);   break;
    case WIFI_WAITING:    pulse(HSV_YELLOW,25);   break;
    case WIFI_CONNECTED:  pixelChase(25);         break;
  }
  */
}

/** If using SoftwareSerial */
void handleSerialCommunication(){

  /** Read ESP8266 output */
  while (SoftSerialAvailable()) {
    String msg = SoftSerialReadStringUntil('\r');
    if (msg.length() > 0){
      Serial.print(msg);
    }
  }

  /** Send command from monitor to ESP8266 */
  if (Serial.available()){
    String msg = Serial.readStringUntil('\r') + '\r';
    Serial.readStringUntil('\n'); //read of everything else 
    SoftSerialPrint(msg);
  }
}

/** Reset ESP8266 module */
void resetWifi(){
  wifiState = WIFI_INIT;
  digitalWrite(SERIAL_RESET, LOW);
  delay(100);
  digitalWrite(SERIAL_RESET, HIGH);
}

/** Apply color magic */
void pixelChase(uint8_t wait){

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

/** Blue-ish pulse for connection to wifi */
void pulse(const HSV &hsv, uint8_t wait){

    static float rate = 0.05;
    static float value = 0.5f;

    value += rate;
    if (value <= 0.3f || value >= 1.0f){
      rate = -rate;
    }
    
    for (int i=0; i<NUM_PIXELS;i++){
      pixels[i]   = hsv;
      pixels[i].v = Clamp01(value + rate);
      strip.setPixelColor(i, pixels[i].ToRgb().ToUInt32());
    }

    strip.show();
    delay(wait);
}


