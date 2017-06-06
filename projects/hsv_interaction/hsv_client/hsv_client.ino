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
#define I2C_BUF_SIZE 4
char i2cBuffer[I2C_BUF_SIZE];

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
#define DATA_PIN 2
#define NUM_PIXELS 16
#define BRIGTHNESS 40 

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, DATA_PIN, NEO_GRB + NEO_KHZ800);
HSV           pixels[NUM_PIXELS];
HSV           targetHSV;
int16_t       targetPixel = 0;
WifiState     wifiState = WIFI_INIT;
unsigned long wifiCmdTime    = 0; 
unsigned long wifiTimeout = 5000;
unsigned long lastRequest = 0;

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
  I2C_ClearBus();
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

  static int kkk = 0;
  if (kkk++ % 1000)
    Serial.println(wifiCmdTime);

  if (wifiCmdTime + 3000 < millis()){ //5sec
    Serial.println("resetting ");
    I2C_ClearBus();
    Wire.begin();
    Wire.onReceive(onI2cReceived);
    Wire.onRequest(onI2cRequest);
  }

  /** Is connection active */
  if (wifiState == WIFI_CONNECTED && wifiCmdTime + wifiTimeout < millis()){ //5sec
    wifiState = WIFI_WAITING;
  }

  /** Update pixels */
  switch(wifiState){
    case WIFI_INIT:       pulse(HSV_RED, 25);     break;
    case WIFI_CONNECTING: pulse(HSV_ORANGE,25);   break;
    case WIFI_WAITING:    pulse(HSV_GREEN,25);    break;
    case WIFI_CONNECTED:  pixelChase(25);         break;
  }
}

/** I2C Slave: Handle data send by i2c master */
void onI2cReceived(int howMany) {

  String str = "";
  while (Wire.available()) {
    str += (char)Wire.read();
  }

   if (str.length() > 0){
    wifiCmdTime = millis();

    if (str.startsWith("w=")){
      int state = str.substring(2).toInt();
      wifiState = (state == 0 ? WIFI_CONNECTING : WIFI_WAITING);
    }
    else {
      int h       = str.toInt();
      targetHSV   = HSV(h/360.0f, 1.0f, 1.0f, 1.0f);
      targetPixel = random(strip.numPixels());
      wifiState   = WIFI_CONNECTED;
    }
  }
}

/** I2C Slave: On request, send data to master (esp8266) */
void onI2cRequest() {
  
  if (lastRequest + 500 < millis()){
    memset(i2cBuffer, 0, I2C_BUF_SIZE);
    Wire.write(i2cBuffer, I2C_BUF_SIZE);

    lastRequest = millis();
    wifiCmdTime = millis();
  }
}

/** SoftwareSerial: Handle SoftwareSerial <> Serial communication */
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

/** ESP8266: Hard reset circut */
void resetWifi(){
  wifiState = WIFI_INIT;
  digitalWrite(SERIAL_RESET, LOW);
  delay(100);
  digitalWrite(SERIAL_RESET, HIGH);
}

/** WS2812: Apply color magic */
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

/** WS2812: Create a pulse effect */
void pulse(const HSV &hsv, uint8_t wait){

    static float rate = 0.01;
    static float value = 0.5f;

    value += rate;
    if (value <= 0.3f || value >= 0.7f){
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

/**
 * SOURCE: http://www.forward.com.au/pfod/ArduinoProgramming/I2C_ClearBus/index.html
 * This routine turns off the I2C bus and clears it
 * on return SCA and SCL pins are tri-state inputs.
 * You need to call Wire.begin() after this to re-enable I2C
 * This routine does NOT use the Wire library at all.
 *
 * returns 0 if bus cleared
 *         1 if SCL held low.
 *         2 if SDA held low by slave clock stretch for > 2sec
 *         3 if SDA held low after 20 clocks.
 */
 
int I2C_ClearBus() {
#if defined(TWCR) && defined(TWEN)
  TWCR &= ~(_BV(TWEN)); //Disable the Atmel 2-Wire interface so we can control the SDA and SCL pins directly
#endif

  pinMode(SDA, INPUT_PULLUP); // Make SDA (data) and SCL (clock) pins Inputs with pullup.
  pinMode(SCL, INPUT_PULLUP);

  //delay(2500);  // Wait 2.5 secs. This is strictly only necessary on the first power
  // up of the DS3231 module to allow it to initialize properly,
  // but is also assists in reliable programming of FioV3 boards as it gives the
  // IDE a chance to start uploaded the program
  // before existing sketch confuses the IDE by sending Serial data.

  boolean SCL_LOW = (digitalRead(SCL) == LOW); // Check is SCL is Low.
  if (SCL_LOW) { //If it is held low Arduno cannot become the I2C master. 
    return 1; //I2C bus error. Could not clear SCL clock line held low
  }

  boolean SDA_LOW = (digitalRead(SDA) == LOW);  // vi. Check SDA input.
  int clockCount = 20; // > 2x9 clock

  while (SDA_LOW && (clockCount > 0)) { //  vii. If SDA is Low,
    clockCount--;
  // Note: I2C bus is open collector so do NOT drive SCL or SDA high.
    pinMode(SCL, INPUT); // release SCL pullup so that when made output it will be LOW
    pinMode(SCL, OUTPUT); // then clock SCL Low
    delayMicroseconds(10); //  for >5uS
    pinMode(SCL, INPUT); // release SCL LOW
    pinMode(SCL, INPUT_PULLUP); // turn on pullup resistors again
    // do not force high as slave may be holding it low for clock stretching.
    delayMicroseconds(10); //  for >5uS
    // The >5uS is so that even the slowest I2C devices are handled.
    SCL_LOW = (digitalRead(SCL) == LOW); // Check if SCL is Low.
    int counter = 20;
    while (SCL_LOW && (counter > 0)) {  //  loop waiting for SCL to become High only wait 2sec.
      counter--;
      delay(100);
      SCL_LOW = (digitalRead(SCL) == LOW);
    }
    if (SCL_LOW) { // still low after 2 sec error
      return 2; // I2C bus error. Could not clear. SCL clock line held low by slave clock stretch for >2sec
    }
    SDA_LOW = (digitalRead(SDA) == LOW); //   and check SDA input again and loop
  }
  if (SDA_LOW) { // still low
    return 3; // I2C bus error. Could not clear. SDA data line held low
  }

  // else pull SDA line low for Start or Repeated Start
  pinMode(SDA, INPUT); // remove pullup.
  pinMode(SDA, OUTPUT);  // and then make it LOW i.e. send an I2C Start or Repeated start control.
  // When there is only one I2C master a Start or Repeat Start has the same function as a Stop and clears the bus.
  /// A Repeat Start is a Start occurring after a Start with no intervening Stop.
  delayMicroseconds(10); // wait >5uS
  pinMode(SDA, INPUT); // remove output low
  pinMode(SDA, INPUT_PULLUP); // and make SDA high i.e. send I2C STOP control.
  delayMicroseconds(10); // x. wait >5uS
  pinMode(SDA, INPUT); // and reset pins as tri-state inputs which is the default state on reset
  pinMode(SCL, INPUT);
  return 0; // all ok
}


