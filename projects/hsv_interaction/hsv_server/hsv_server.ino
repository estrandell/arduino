/**
 * MIT License
 * 
 * Copyright (c) 2017 Ebbe Strandell
 * https://github.com/estrandell/arduino
 */

#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>
#include <Wire.h>
#include <Time.h>
#include "colorLib.h"


#define NES_DATA_PIN 4          // NES controller
#define NES_LATCH_PIN 2         // NES controller
#define NES_CLOCK_PIN 3         // NES controller
#define DATA_PIN_0 2            // Pixel strip 0 data pin
#define DATA_PIN_1 3            // Pixel strip 1 data pin
#define BRIGTHNESS 40           // Pixel brightness
#define SERIAL_TX 10            // To RX  on ESP8266
#define SERIAL_RX 11            // To TX  on ESP8266
#define SERIAL_RESET 6          // To Res on ESP8266
#define I2C_ADDRESS 8           // Address on i2c bus
#define I2C_BUF_SIZE 4          // Buffer size for i2c messages

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


enum WifiState{
  WIFI_INIT = 0,
  WIFI_DISCONNECTED,
  WIFI_CONNECTED,
};

enum HsvMode{
  HSV_MODE_MONOCHROMATIC = 0,
  HSV_MODE_BOUNCE,
  HSV_MODE_CHASE
};


/** Run as app client */
Adafruit_NeoPixel * strip0 = NULL;
HSV               * pixels = NULL;

HSV               nextHSV;
bool              getNextHsv = false;
unsigned long     getNextHsvTime = 0;
float             nextHValue;


WifiState         wifiState     = WIFI_INIT;
unsigned long     i2cHeartbeat  = 0;

HsvMode           hsvMode       = HSV_MODE_MONOCHROMATIC;

bool              isHost = false;
int               numPixels = 0;

void setup() {
  
  /** Setup serial communication */
  Serial.begin(9600);
  Serial.setTimeout(50);
  Serial.println("*** Starting program ***");
  randomSeed(analogRead(0));

  /** Setup software serial communication, if allowed */
  pinMode(SERIAL_TX, OUTPUT);
  pinMode(SERIAL_RX, INPUT_PULLUP);
  SoftSerialBegin(9600);
  SoftSerialSetTimeout(50);

  /** Read / write EEPROM */
  //clearEEPROM();
  setupEEPROM();
  Serial.print(" isHost: ");
  Serial.println(isHost);
  Serial.print(" numPixels: ");
  Serial.println(numPixels);

  /** Join i2c bus on address #8 */
  I2C_ClearBus();
  Wire.begin(I2C_ADDRESS);
  Wire.onReceive(onI2cReceived);
  Wire.onRequest(onI2cRequest);

  if (isHost){
    /** Init NES Controler */
    pinMode(NES_LATCH_PIN, OUTPUT);  // Latch connection is an output
    pinMode(NES_CLOCK_PIN, OUTPUT);  // Clock connection is an output
    pinMode(NES_DATA_PIN, INPUT);    // Data connection is an Input (because we need to receive the data from the control pad)
  } else {
    /** Init led strip */
    strip0 = new Adafruit_NeoPixel(numPixels, DATA_PIN_0, NEO_GRB + NEO_KHZ800);
    strip0->begin();
    strip0->setBrightness(BRIGTHNESS);
    strip0->show(); // Initialize all pixels to 'off'

    pixels = new HSV[numPixels];
  }

  /** Reset ESP8266 */
  pinMode(SERIAL_RESET, OUTPUT);
  digitalWrite(SERIAL_RESET, HIGH);
  delay(100);
  resetWifi();
}

void loop() {

  /** If using SoftwareSerial */
  handleSerialCommunication();

  /** If I2C conncetion is dead, try to clear bus */
  if (i2cHeartbeat + 5000 < millis()){ //30sec
    Serial.println("resetting ");
    I2C_ClearBus();
    Wire.begin();
  }

  /** Main logic */
  if(isHost){
    /** Host: Read input device */
    static unsigned long lastChange = 0;
    if (handleNESControllerData()){
      lastChange = millis();
    }

    switch(hsvMode){
      case HSV_MODE_CHASE : 
        if (millis() > lastChange + 30000){
          nextHSV     = HSVClockRandom();
          getNextHsv  = true;
          lastChange  = millis();
          //Serial.print("new target: ");
          //Serial.println(nextHSV.h);
        }
        break;
    }
    
  }else{
    /** Update pixels */
    switch (wifiState){
      case WIFI_INIT:         pulse(HSV_RED, 25);     break;
      case WIFI_DISCONNECTED: pulse(HSV_ORANGE,25);    break;
      case WIFI_CONNECTED:    
      {
        switch (hsvMode){
          case HSV_MODE_MONOCHROMATIC:    setMonoChromatic(25);   break;
          case HSV_MODE_BOUNCE:           pixelBounce(25);        break;
          case HSV_MODE_CHASE:            pixelChase(25);         break;
        }
        break;
      }
    }
  }
}

/** I2C Slave: Handle data send by i2c master */
void onI2cReceived(int howMany) {

  if (!isHost){
    static uint8_t byteBuffer[I2C_BUF_SIZE];
    memset(byteBuffer, 0, I2C_BUF_SIZE);
  
    int i = 0;
    while (Wire.available() && i < I2C_BUF_SIZE) {
      byteBuffer[i++] = (byte)Wire.read();
    }
  
    if (i == I2C_BUF_SIZE){
      int data = (byteBuffer[2] << 8) | byteBuffer[3];
      if (byteBuffer[0] == 1){
        /** This is ESP Wifi status */
        wifiState = (data == 0 ? WIFI_DISCONNECTED : WIFI_CONNECTED);
      }
      if (byteBuffer[0] == 2){
        /** This is arduino <> arduino data */
        hsvMode = ((byteBuffer[1] % 3) + 3) % 3;
        nextHSV = HSV(data/360.0f, 1.0f, 1.0f, 1.0f);
        getNextHsv = true;
        getNextHsvTime = millis();
      }  
    }
  }

  i2cHeartbeat = millis();
}

/** I2C Slave: On request, send data to master (esp8266) */
void onI2cRequest() {

  static uint8_t byteBuffer[I2C_BUF_SIZE];
  memset(byteBuffer, 0, I2C_BUF_SIZE);

  if (isHost){
    int hval = (int)nextHValue;
    byteBuffer[0] = 1; // data available
    byteBuffer[1] = (byte)hsvMode;
    byteBuffer[2] = hval >> 8;
    byteBuffer[3] = hval & 0xFF;
  }

  i2cHeartbeat = millis();
  Wire.write(byteBuffer, I2C_BUF_SIZE);
}

/** NES: Get controller data */
byte getNESControllerData(){
  byte data = 0;
  digitalWrite(NES_LATCH_PIN, HIGH);       
  digitalWrite(NES_LATCH_PIN, LOW);
  for(int i=0; i<=7; i++){       
    bitWrite(data, i, digitalRead(NES_DATA_PIN)); 
    digitalWrite(NES_CLOCK_PIN, HIGH);
    digitalWrite(NES_CLOCK_PIN, LOW);
  }
  return data;
}

/** NES: HandleGet controller data */
bool handleNESControllerData(){
  static const unsigned char option1 = 0x01; // hex for 0000 0001, A
  static const unsigned char option2 = 0x02; // hex for 0000 0010, B
  static const unsigned char option3 = 0x04; // hex for 0000 0100, Select
  static const unsigned char option4 = 0x08; // hex for 0000 1000, Start
  static const unsigned char option5 = 0x10; // hex for 0001 0000, UP
  static const unsigned char option6 = 0x20; // hex for 0010 0000, DOWN
  static const unsigned char option7 = 0x40; // hex for 0100 0000, LEFT
  static const unsigned char option8 = 0x80; // hex for 1000 0000, RIGHT

  static unsigned long nextNesRead = 0;
  byte data = getNESControllerData();
  bool dirty = false;

  if (nextNesRead < millis()){ 
    if ((data & 0x01) == 0){
      nextHValue = fmod(nextHValue - 30.f + 360, 360.0f);
      nextNesRead = 300 + millis();
      dirty = true;
    }else if((data & 0x02) == 0){
      nextHValue = fmod(nextHValue + 30.f, 360.0f);
      nextNesRead = 300 + millis();
      dirty = true;
    }else if((data & 0x04) == 0){
      hsvMode = hsvMode + 1; 
      nextNesRead = 300 + millis();
      Serial.println(hsvMode);
      dirty = true;
    }else if((data & 0x08) == 0){
      hsvMode = hsvMode -1; 
      nextNesRead = 300 + millis();
      Serial.println(hsvMode);
      dirty = true;
    }else if((data & 0x40) == 0){
      nextHValue = fmod(nextHValue - 0.01f + 360, 360.0f);
      dirty = true;
    }else if((data & 0x80) == 0){
      nextHValue = fmod(nextHValue + 0.01f, 360.0f);
      dirty = true;
    }
    
  }

  return dirty;
}

/** SoftwareSerial: Handle SoftwareSerial <> Serial communication */
void handleSerialCommunication(){
#ifdef USE_SOFTWARE_SERIAL
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
#endif
}

/** ESP8266: Hard reset circut */
void resetWifi(){
  wifiState = WIFI_INIT;
  digitalWrite(SERIAL_RESET, LOW);
  delay(100);
  digitalWrite(SERIAL_RESET, HIGH);
}

/** WS2812: Set monochromatic colors */
void setMonoChromatic(uint8_t wait){
  for (int i=0; i<numPixels; i++){
    strip0->setPixelColor(i, nextHSV.ToRgb().ToUInt32());
  }
  
  strip0->show();
  delay(wait);
}

/** WS2812: Apply color magic */
void pixelBounce(uint8_t wait){
  static const float limit    = 30.0f / 360.0f; 
  static const float rate     = 0.003;
  static bool  rising         = true;
  static float offset         = 0;

  offset += (rising ? rate : -rate);
  if ( rising && offset >=  limit) rising = false;
  if (!rising && offset <= -limit) rising = true;

  HSV hsv(nextHSV.h + offset, 1.0f, 1.0f, 1.0f);
  for (int i=0; i<numPixels; i++){
    strip0->setPixelColor(i, hsv.ToRgb().ToUInt32());
  }
     
  strip0->show();
  delay(wait);
}

/** WS2812: Apply color magic */
void pixelChase(uint8_t wait){
  static const float rise     = 0.05f; 
  static const float growth   = 0.3f; 
  static const float decay    = 0.01f;
  static HSV targetHSV        = HSVClockRandom();
  static int16_t targetPixel  = random(strip0->numPixels());

  
  /** set main color */
  pixels[targetPixel]         = LerpHSV(pixels[targetPixel], targetHSV, rise);

  /** grow */
  for (int i=targetPixel-1; i>=0; i--){
    pixels[i]   = LerpHSV(pixels[i], pixels[i+1], growth);
    pixels[i].v = Clamp01(pixels[i].v - decay);
  }
  for (int i=targetPixel+1; i<numPixels; i++){
    pixels[i]   = LerpHSV(pixels[i], pixels[i-1], growth);
    pixels[i].v = Clamp01(pixels[i].v - decay);
  }

  /** set strip */
  for (int i=0; i<numPixels; i++){
    strip0->setPixelColor(i, pixels[i].ToRgb().ToUInt32());
  }

  /** reset ? */
  if (abs(pixels[targetPixel].h - targetHSV.h) < 0.01f){
    targetPixel = random(numPixels);
    if(getNextHsv){
      targetHSV   = (getNextHsv ? nextHSV : HSVClockRandom());
      targetPixel = random(numPixels);
      getNextHsv = false;
    }
  }

  strip0->show();
  delay(wait);
}

/** WS2812: Create a pulse effect */
void pulse(const HSV &hsv, uint8_t wait){
  static float rate = 0.05;
  static float value = 0.5f;

  value += rate;
  if (value <= 0.3f || value >= 1.0f){
    rate = -rate;
  }
  
  for (int i=0; i<numPixels;i++){
    pixels[i]   = hsv;
    pixels[i].v = Clamp01(value + rate);
    strip0->setPixelColor(i, pixels[i].ToRgb().ToUInt32());
  }

  strip0->show();
  delay(wait);
}

/** setup EEPROM stuff */
void setupEEPROM(){
  if (!readEEPROM()){
    unsigned long t0 = millis();
    Serial.println("Failed to read EEPROM. Press any key for setup or wait 10s to skip");
    while (Serial.available() == 0){
      if (millis() > t0 + 10000){
        Serial.println("Skipped");
        break;
      }
    }

    if (Serial.available() != 0){
      Serial.readStringUntil('\r');
      Serial.print("isHost (0/1): ");
      while (Serial.available() == 0){}
      isHost    = (bool)Serial.readStringUntil('\r').toInt();

      Serial.print("\nnumPixels :" );
      while (Serial.available() == 0){}
      numPixels = Serial.readStringUntil('\r').toInt();

      Serial.print("isHost: ");
      Serial.print(isHost);
      Serial.print(" ,numPixels: ");
      Serial.println(numPixels);

      Serial.print("save? (0/1): ");
      while (Serial.available() == 0){}
      if (1 == Serial.readStringUntil('\r').toInt()){
        Serial.println("Write settings to EEPROM");
        writeEEPROM();
      }
    }
  }
}

/** EEPROM: read */
bool readEEPROM(){
  isHost     = EEPROM.read(0);
  numPixels  = (EEPROM.read(1) << 8) | EEPROM.read(2);
  byte ok    = EEPROM.read(3);
  if (ok == 231) {
    return true;
  }
  else{
    return false;
  }
 }
 
/** EEPROM: Store credentials */
void writeEEPROM() {
  static const char ok[2+1] = "OK";
  EEPROM.write(0, isHost);
  EEPROM.write(1, numPixels >> 8);
  EEPROM.write(2, numPixels & 0xFF);
  EEPROM.write(3, (byte)231);
}

/** EEPROM: Clear credentials */
void clearEEPROM(){
  static const byte zero = 0;
  for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, zero);
  }
}

/** I2C: Clear bus
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

