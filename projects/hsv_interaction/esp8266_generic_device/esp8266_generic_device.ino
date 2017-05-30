#include <ESP8266WiFi.h>
#include <EEPROM.h>

/*
  How to change the baud rate on the ESP8266 will depend on the firmware version. 
  I've had success with AT+IPR=9600. 
  You only need to run this command once (it's a persistent setting).

  https://diyhacking.com/esp8266-tutorial/

  AT+RST—Restarts the Module
  AT+GMR—Checks Version Information
  AT+RESTORE—Restores the Factory Default Settings

  AT+CWLAP - Lists available APs.
  AT+CWQAP - Disconnects from an AP  
*/

#define SSID_LEN 32
#define PASS_LEN 32
#define EEPROM_LEN 512
char ssid[SSID_LEN];
char password[PASS_LEN];

int ledPin = 2; // GPIO2 of ESP8266
WiFiServer server(80);//Service Port



 
void setup() {
  Serial.begin(9600);
  Serial.println("");
  Serial.println("*** Program start ***");
  Serial.println("");
  delay(100);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  if (loadCredentials()){
    Serial.println("Loaded credentials from EEPROM");
    if (connectWifi()){
      digitalWrite(ledPin, HIGH);
    }
  }
  else {
    Serial.println("FAILED to load credentials. Setup needed.");
  }
}

void handleMessage(String msg){

  int index = msg.indexOf("ssid=");
  if (index != -1){
    String newSsid = msg.substring(index+5);
    newSsid.toCharArray(ssid, newSsid.length()+1);
    Serial.print("New SSID: ");
    Serial.println(ssid);
  }
  
  index = msg.indexOf("pass=");
  if (index != -1){
    String newPass = msg.substring(index+5);
    newPass.toCharArray(password, newPass.length()+1);
    Serial.print("New password set: ********** ");
  }

  index = msg.indexOf("loadcfg");
  if (index != -1){
    if (loadCredentials()){
      Serial.println("Loaded credentials from EEPROM");
    }
    else{
      Serial.println("FAILED to load credentials");
    }
  }

  index = msg.indexOf("savecfg");
  if (index != -1){
    saveCredentials();
    Serial.println("Saved credentials to EEPROM");
  }

  index = msg.indexOf("clearcfg");
  if (index != -1){
    clearCredentials();
    Serial.println("Cleared credentials in EEPROM");
  }

  index = msg.indexOf("reconnect");
  if (index != -1){
    disconnectWifi();
    digitalWrite(ledPin, LOW);
    if (connectWifi()){
      digitalWrite(ledPin, HIGH);
    }
  }

  index = msg.indexOf("print");
  if (index != -1){
    Serial.print("SSID: ");
    Serial.println(ssid);
    Serial.print("Password: ");
    Serial.println(strlen(password)>0 ? "********" : "<no password>");
  }
}


void loop() {
  
  /** Read Arduino communication */
  if(Serial.available()){
    String msg = Serial.readStringUntil('\r');
    handleMessage(msg);
  }


  /** Check if Wifi is up and running */
  if (WiFi.status() != WL_CONNECTED){
    digitalWrite(ledPin, LOW);
    if (connectWifi()){
      digitalWrite(ledPin, HIGH);
    }
  }
  
  /** Check if a client has connected */
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
   
  /** Wait until the client sends some data */
  int tries = 0;
  while(!client.available()){
    if (tries++ == 10)
      return;
    delay(1);
  }
   
  /** Read the first line of the request */
  String request = client.readStringUntil('\r');
  client.flush();

  /** Interpret message */
  int i = request.indexOf("/");
  int j = request.indexOf("HTTP/");
  int h;
  if (i != -1 && j != -1){
    if (request.indexOf("favicon.ico") == -1){
      request = request.substring(i+1,j-1);
      h = request.indexOf("h=");
      if (h != -1){
        Serial.println(request.substring(h+2));
      }
      else{
        handleMessage(request);
      }
    }
  }

  /** Write response */
  client.println("HTTP/1.1 200 OK");
  delay(1);
}



bool connectWifi(){
  if (WiFi.status() == WL_CONNECTED){
    disconnectWifi();
  }

  Serial.println("");
  Serial.print("Connecting to ssid: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (i++ == 10){
      Serial.println("");
      Serial.println("Failed to connect");
      return false;
    }
  }

  Serial.println("");
  Serial.println("Wifi connected");
   
  // Start the server
  server.begin();
  Serial.println("Server started at http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
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

