/**
 * MIT License
 * 
 * Copyright (c) 2017 Ebbe Strandell
 * https://github.com/estrandell/arduino
 * 
 * 
 * See Readme.txt for setup information and how to flash the ESP8266
 * 
 */
 
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>


/**
 * Interrupts caused by SoftwareSerial disturbs the WS2812 update cycle on the Arduino.
 * As such we send data over the I2C bus, and may skip the serial com for performance.
 */
 
#define USE_SERIAL
#ifdef USE_SERIAL
  #define SerialAvailable()         Serial.available()
  #define SerialBegin(x)            Serial.begin(x)
  #define SerialPrint(x)            Serial.print(x)
  #define SerialPrintln(x)          Serial.println(x)
  #define SerialReadStringUntil(x)  Serial.readStringUntil(x)
#else
  #define SerialAvailable()         false
  #define SerialBegin(x)
  #define SerialPrint(x)
  #define SerialPrintln(x)
  #define SerialReadStringUntil(x)  ""
#endif


/**
 * SSID and password to be stored on EEPROM
 */
#define SSID_LEN 32
#define PASS_LEN 32
#define EEPROM_LEN 512
char ssid[SSID_LEN];
char password[PASS_LEN];


/**
 * ESP <> Arduino communication over I2C
 */
#define I2C_ADDRESS 8
#define I2C_BUF_SIZE 4

 enum I2C_MSG_TYPE{
  I2C_UNKNOWN = 0,
  I2C_WIFI_STATUS,
  I2C_USER_DATA
 };


/**  
 *  Network communication over HTTP and/or UDP.
 */

//#define USE_HTTP
#ifdef USE_HTTP
  #define HTTP_PORT 80
  WiFiServer server(HTTP_PORT);
#endif

#define USE_UDP
#ifdef USE_UDP
  #define UDP_PORT 7777
  #define UDP_PACKET_SIZE 4
  WiFiUDP udpServer;
  IPAddress broadCastIp(192, 168, 1, 255);
#endif


void setup() {

  /** Setup serial port if in use */
  SerialBegin(9600);
  SerialPrintln("");
  SerialPrintln("*** Program start ***");
  SerialPrintln("");
  delay(5);

  /** Setup I2C communication */
  Wire.begin(0, 2);
  sendI2cWifiStatus(0);

  /** Load credentials from persistent storage */
  if (loadCredentials()){
    SerialPrintln("Loaded credentials from EEPROM");

    if (connectWifi()){
      sendI2cWifiStatus(1);
    }
  }
  else {
    SerialPrintln("FAILED to load credentials. Setup needed.");
  }

  delay(50);
}

void loop() {

  static const int wait = 100;
  static const int wifiStatusFreq = 1000 / wait;

  /** Check for serial communication */
  if(SerialAvailable()){
    String msg = SerialReadStringUntil('\r');
    handleMessage(msg);
  }
  delay(1);

  /** Report wifi status */
  static int wifiStatus = 0;
  if (wifiStatus++ % wifiStatusFreq == 0){ // 1 Hz
    sendI2cWifiStatus(int(WiFi.status() == WL_CONNECTED ? 1 : 0));
  }

  /** Need wifi or do nothing */
  if (WiFi.status() != WL_CONNECTED){
    if (!connectWifi()){
      delay(200);
      return;
    }
  }
  delay(1);

  /** Check for I2C messages */
  static uint8_t dataBuffer[UDP_PACKET_SIZE];
  if (receiveI2c(dataBuffer)){
    sendUdp(dataBuffer);
  }
  delay(1);

  /** Check for UDP data */
  if (receiveUdp(dataBuffer)){
    sendI2cUserData(dataBuffer);
  }
  delay(1);

  /** Check for HTTP data */
  String request;
  if (receiveHttp(request)){
    if (request.startsWith("h=")){
      // send to arduino
      int val = request.substring(2).toInt();
      if (val >= 0 && val <= 360){
      //  sendI2c(I2C_HSV, val);
      }
    }
    else{
      // handle internally
      handleMessage(request);
    }
  }

  delay(6);
}

void handleMessage(String msg){

  int index = msg.indexOf("ssid=");
  if (index != -1){
    String newSsid = msg.substring(index+5);
    newSsid.toCharArray(ssid, newSsid.length()+1);
    SerialPrint("New SSID: ");
    SerialPrintln(ssid);
  }
  
  index = msg.indexOf("pass=");
  if (index != -1){
    String newPass = msg.substring(index+5);
    newPass.toCharArray(password, newPass.length()+1);
    SerialPrintln("New password set: ********** ");
  }

  index = msg.indexOf("loadcfg");
  if (index != -1){
    if (loadCredentials()){
      SerialPrintln("Loaded credentials from EEPROM");
    }
    else{
      SerialPrintln("FAILED to load credentials");
    }
  }

  index = msg.indexOf("savecfg");
  if (index != -1){
    saveCredentials();
    SerialPrintln("Saved credentials to EEPROM");
  }

  index = msg.indexOf("clearcfg");
  if (index != -1){
    clearCredentials();
    SerialPrintln("Cleared credentials in EEPROM");
  }

  index = msg.indexOf("reconnect");
  if (index != -1){
    disconnectWifi();
    sendI2cWifiStatus(0);

    if (connectWifi()){
      sendI2cWifiStatus(1);
    }
  }

  index = msg.indexOf("print");
  if (index != -1){
    SerialPrint("SSID: ");
    SerialPrintln(ssid);
    SerialPrint("Password: ");
    SerialPrintln(strlen(password)>0 ? "********" : "<no password>");
  }
}

/** UDP: receive UDP value */
bool receiveUdp(byte packetBuffer[]) {
#ifdef USE_UDP
  int cb = udpServer.parsePacket();
  if (cb) {
    udpServer.read(packetBuffer, UDP_PACKET_SIZE);
    return true;
  }
  return false;
#else
  return false;
#endif
}

/** UDP: Send UDP value */
void sendUdp(byte packetBuffer[]){
#ifdef USE_UDP
  udpServer.beginPacket(broadCastIp, UDP_PORT);
  udpServer.write(packetBuffer, UDP_PACKET_SIZE);
  udpServer.endPacket();
#endif
}

/** UDP: Setup UDP server */
void setupUdp(){
#ifdef USE_UDP
  SerialPrintln("Connecting to UDP");
  if (udpServer.begin(UDP_PORT) == 1){
    SerialPrint("Connection successful. Port: ");
    SerialPrintln(udpServer.localPort());
  }
  else{
  SerialPrintln("Connection failed");
  }
#else  
  SerialPrintln("UDP disabled");
#endif
}

/** I2C Master: Send data to slave node */
int sendI2cWifiStatus(int connectionStatus){

  static uint8_t buffer[I2C_BUF_SIZE];
  buffer[0] = I2C_WIFI_STATUS;          // 1 byte - message type
  buffer[1] = 0;                        // 1 byte - switch
  buffer[2] = connectionStatus >> 8;    // 2 byte - data
  buffer[3] = connectionStatus & 0xFF;

  Wire.beginTransmission(I2C_ADDRESS);
  Wire.write(buffer, I2C_BUF_SIZE);
  return Wire.endTransmission();
}


/** I2C Master: Send data to slave node */
int sendI2cUserData(uint8_t dataBuffer[]){

  dataBuffer[0] = I2C_USER_DATA;
  Wire.beginTransmission(I2C_ADDRESS);
  Wire.write(dataBuffer, I2C_BUF_SIZE);
  return Wire.endTransmission();
}

/** I2C Master: Request data from slave node */
bool receiveI2c(uint8_t dataBuffer[]){

  memset(dataBuffer, 0, I2C_BUF_SIZE);

  Wire.requestFrom(I2C_ADDRESS, I2C_BUF_SIZE);
  int i = 0;
  while (Wire.available()) {
    dataBuffer[i++] = (uint8_t)Wire.read();
  }

  if (i == I2C_BUF_SIZE && dataBuffer[0] == 1){
    return true;
  }

  return false;
}
 
/** HTTP: connect to remote HTTP server */
void sendHttp(int address, int value){
#ifdef USE_HTTP
  String url = "http://192.168.1.";
  url += address;
  url += "/h=";
  url += value;

  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET(); // zero is OK!
  http.end(); 
#endif
}

/** HTTP: Check for HTTP requests */
bool receiveHttp(String &request){
#ifdef USE_HTTP
  WiFiClient client = server.available();
  if (!client) {
    return false;
  }
   
  // Wait until the client sends some data
  int tries = 0;
  while(!client.available()){
    if (tries++ == 10)
      return false;
    delay(1);
  }

  // Read the first line of the request
  request = client.readStringUntil('\r');
  client.readStringUntil('\n');
  client.flush();  

  // Write response
  client.println("HTTP/1.1 200 OK");

  // Clean request
  if (request.indexOf("favicon.ico") == -1){
    int i = request.indexOf("/");
    int j = request.indexOf("HTTP/");
    if (i != -1 && j != -1){
      request = request.substring(i+1,j-1);
      if (request.length() >0){
        return true;
      }
    }
  }
  
  request = "";
  return false;
#else
  request = "";
  return false;
#endif
}

/** HTTP: Setup HTTP server */
void setupHttp(){
#ifdef USE_HTTP
  server.begin();
  SerialPrintln("Server started at http://");
  SerialPrint(WiFi.localIP());
  SerialPrintln("/");
#else
  SerialPrintln("HTTP disabled");
#endif
}

/** WIFI: Connect to network */
bool connectWifi(){

  /** Disconnect if connected */
  if (WiFi.status() == WL_CONNECTED){
    disconnectWifi();
  }

  /** Connect to WiFi network */
  SerialPrint("Connecting to ssid: ");
  SerialPrintln(ssid);
  WiFi.begin(ssid, password);

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    SerialPrint(".");

    if (i++ == 10){
      SerialPrintln("");
      SerialPrintln("Failed to connect");
      return false;
    }
  }

  SerialPrintln("");
  SerialPrintln("Wifi connected");
   
  /** Start HTTP / UDP services */
  setupHttp();
  setupUdp();

  return true;
}

/** WIFI: Disconnect from current network */
void disconnectWifi(){
#ifdef USE_HTTP
  server.stop();
#endif

  WiFi.disconnect();
}

/** EEPROM: Load credentials */
bool loadCredentials() {
  char ok[2+1];
  EEPROM.begin(EEPROM_LEN);
  EEPROM.get(0, ssid);
  EEPROM.get(0+sizeof(ssid), password);
  EEPROM.get(0+sizeof(ssid)+sizeof(password), ok);
  EEPROM.end();
  if (String(ok) == String("OK")) {
    return true;
  }
  else{
    ssid[0] = 0;
    password[0] = 0;
    return false;
  }
}

/** EEPROM: Store credentials */
void saveCredentials() {
  char ok[2+1] = "OK";
  EEPROM.begin(EEPROM_LEN);
  EEPROM.put(0, ssid);
  EEPROM.put(0+sizeof(ssid), password);
  EEPROM.put(0+sizeof(ssid)+sizeof(password), ok);
  EEPROM.commit();
  EEPROM.end();
}

/** EEPROM: Clear credentials */
void clearCredentials(){
  EEPROM.begin(EEPROM_LEN);
  for (int i = 0 ; i < EEPROM_LEN ; i++) {
    EEPROM.put(i, 0);
  }
  EEPROM.commit();
  EEPROM.end();
}


