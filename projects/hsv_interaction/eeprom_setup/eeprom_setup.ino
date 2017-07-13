/**
 * MIT License
 * 
 * Copyright (c) 2017 Ebbe Strandell
 * https://github.com/estrandell/arduino
 */

#include <EEPROM.h>

bool              isHost = false;
int               numPixels = 0;
byte              check = 231;

void setup() {
  
  /** Setup serial communication */
  Serial.begin(9600);
  Serial.setTimeout(50);
  Serial.println("***** Start program *****");

  /** Read / write EEPROM */
  while (true)
  {
    Serial.println("Trying to read ...");
    bool bRead = readEEPROM();
    if (bRead){
      Serial.println("EEPROM read success");
      Serial.print(" isHost: ");
      Serial.println(isHost);
      Serial.print(" numPixels: ");
      Serial.println(numPixels);
    } else{
      Serial.println("EEPROM read fail.");
    }

    bool bCleared = false;
    if (bRead){
      Serial.print("Clear EEPROM? (0/1): ");
      while (Serial.available() == 0){}
      bCleared = (bool)Serial.readStringUntil('\r').toInt();
      if (bCleared){
        clearEEPROM();
        Serial.println("\nEEPROM cleared");
      } else{
        Serial.println("\nEEPROM not cleared");
      }
    }
  
    if (!bRead || bCleared){
      Serial.println("isHost (0/1): ");
      while (Serial.available() == 0){}
      isHost    = (bool)Serial.readStringUntil('\r').toInt();
      Serial.println("isHost: ");
      Serial.println(isHost);

      while (Serial.available())
        Serial.read();
    
      Serial.println("numPixels :" );
      while (Serial.available() == 0){}
      numPixels = Serial.readStringUntil('\r').toInt();
      Serial.println("numPixels: ");
      Serial.println(numPixels);

      while (Serial.available())
        Serial.read();

      Serial.print("save? (0/1): ");
      while (Serial.available() == 0){}
      if (1 == Serial.readStringUntil('\r').toInt()){
        Serial.println("EEPROM write");
        writeEEPROM();
      }
    }
  }
}

void loop() {
}

/** EEPROM: read */
bool readEEPROM(){
  isHost     = EEPROM.read(0);
  numPixels  = (EEPROM.read(1) << 8) | EEPROM.read(2);
  byte ok    = EEPROM.read(3);
  if (ok == check) {
    return true;
  }
  else{
    return false;
  }
 }
 
/** EEPROM: Store credentials */
void writeEEPROM() {
  EEPROM.write(0, isHost);
  EEPROM.write(1, numPixels >> 8);
  EEPROM.write(2, numPixels & 0xFF);
  EEPROM.write(3, check);
}

/** EEPROM: Clear credentials */
void clearEEPROM(){
  EEPROM.write(0, 0);
  EEPROM.write(1, 0);
  EEPROM.write(2, 0);
  EEPROM.write(3, 0);
}

