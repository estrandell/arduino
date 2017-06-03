/**
 * MIT License
 * 
 * Copyright (c) 2017 Ebbe Strandell
 * https://github.com/estrandell/arduino
 */
 
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
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

/*
 * --- Baud rate on the ESP8266 ---
 * How to change the baud rate on the ESP8266 will depend on the firmware version. 
 * I've had success with AT+IPR=9600. You only need to run this command once (it's a persistent setting).
 * 
 * 
 * --- How to connect everything ---
 * This is far from the whole truth, ill post diagram in the future
 * https://diyhacking.com/esp8266-tutorial/
 * 
 * 
 * --- Commands ---
 * AT+RST—Restarts the Module
 * AT+GMR—Checks Version Information
 * AT+RESTORE—Restores the Factory Default Settings
 * 
 * AT+CWLAP - Lists available APs.
 * AT+CWQAP - Disconnects from an AP  
*/

#define SSID_LEN 32
#define PASS_LEN 32
#define EEPROM_LEN 512
char ssid[SSID_LEN];
char password[PASS_LEN];

#define I2C_ADDRESS 8
#define I2C_BUF_SIZE 32

#define SERVER_PORT 80
WiFiServer server(SERVER_PORT);



void setup() {

  /** Setup serial port if in use */
  SerialBegin(9600);
  SerialPrintln("");
  SerialPrintln("*** Program start ***");
  SerialPrintln("");
  delay(100);

  /** Setup I2C communication */
  Wire.begin(0,2);  //Change to Wire.begin() for non ESP.
  sendI2cMessage("wifi=0");

  /** Load credentials from persistent storage */
  if (loadCredentials()){
    SerialPrintln("Loaded credentials from EEPROM");

    if (connectWifi()){
      sendI2cMessage("wifi=1");
    }
  }
  else {
    SerialPrintln("FAILED to load credentials. Setup needed.");
  }
}

void sendI2cMessage(String message){
  static char buf[I2C_BUF_SIZE];
  message.toCharArray(buf, message.length()+1);
  
  Wire.beginTransmission(I2C_ADDRESS);
  Wire.write(buf);
  Wire.endTransmission();
}

String requestI2cMessage(){

  String s = "";  
  Wire.requestFrom(I2C_ADDRESS, I2C_BUF_SIZE);
  while (Wire.available()){
    s += (char)Wire.read();
  }
  return s;
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
    sendI2cMessage("wifi=0");

    if (connectWifi()){
      sendI2cMessage("wifi=1");
    }
  }

  index = msg.indexOf("ping");
  if (index != -1){
    delay(100);
    SerialPrintln("pong");
    sendI2cMessage("pong");
  }

  index = msg.indexOf("print");
  if (index != -1){
    SerialPrint("SSID: ");
    SerialPrintln(ssid);
    SerialPrint("Password: ");
    SerialPrintln(strlen(password)>0 ? "********" : "<no password>");
  }
}


void loop() {
  
  /** Check for serial communication */
  if(SerialAvailable()){
    String msg = SerialReadStringUntil('\r');
    handleMessage(msg);
  }

  /** Check for I2C messages */
  String msg = requestI2cMessage();
  if (msg.length() > 0){
    sendI2cMessage("s: " +msg.substring(0,10));
  }
   
  /** Read request from wifi */
  String request;
  if (checkWifiRequest(request)){
    if (request.indexOf("favicon.ico") == -1){

      int i = request.indexOf("/");
      int j = request.indexOf("HTTP/");
      if (i != -1 && j != -1){
        request = request.substring(i+1,j-1);
        if (request.startsWith("h=")){
          //sendI2cMessage(request);

        
          String s    = "http://192.168.1.120/h=";
          s          += request.substring(2).toInt();        

          HTTPClient http;
          http.begin(s);
          int httpCode = http.GET();
          if (httpCode > 0) {
            String payload = http.getString();
            Serial.println(payload);
          }
          else {
            Serial.println("payload size is zero");
          }
          http.end();   //Close connection        
        }
        else{
          handleMessage(request);
        }
      }
    }
  }

  delay(50);
}

/** Check if Wifi is up and running
 *  Check if a client has connected
 *  Wait for client to sends some data 
 *  
 *  returns true if all is good and request is populated
 */
bool checkWifiRequest(String &request){

  if (WiFi.status() != WL_CONNECTED){
    if (!connectWifi()){
      return false;
    }
  }

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

  if (request.length() >0)
    return true;
  return false;
}

/** Connect to wifi */
bool connectWifi(){
  if (WiFi.status() == WL_CONNECTED){
    disconnectWifi();
  }

  // Connect to WiFi network
  SerialPrintln("");
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
   
  // Start the server
  server.begin();
  SerialPrintln("Server started at http://");
  SerialPrint(WiFi.localIP());
  SerialPrintln("/");

  return true;
}

/** Disconnect from current wifi */
void disconnectWifi(){
  server.stop();
  WiFi.disconnect();
}

/** Load WLAN credentials from EEPROM */
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

/** Store WLAN credentials to EEPROM */
void saveCredentials() {
  char ok[2+1] = "OK";
  EEPROM.begin(EEPROM_LEN);
  EEPROM.put(0, ssid);
  EEPROM.put(0+sizeof(ssid), password);
  EEPROM.put(0+sizeof(ssid)+sizeof(password), ok);
  EEPROM.commit();
  EEPROM.end();
}

/** Clear WLAN credentials on EEPROM */
void clearCredentials(){
  EEPROM.begin(EEPROM_LEN);
  for (int i = 0 ; i < EEPROM_LEN ; i++) {
    EEPROM.put(i, 0);
  }
  EEPROM.commit();
  EEPROM.end();
}

