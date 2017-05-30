#include <SoftwareSerial.h>
SoftwareSerial mySerial(11,10); // RX, TX

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(50);
  mySerial.begin(9600);
  mySerial.setTimeout(50);
  Serial.println("Starting program");
  delay(200);
}
 
void loop() {

  /** Read ESP8266 output */
  while (mySerial.available()) {
    String msg = mySerial.readStringUntil('\r');
    if (msg.length() > 0){
      Serial.print(msg);

      // TODO: Add logic
      
    }
  }

  /** Send command from monitor to ESP8266 */
  if (Serial.available()){
    String msg = Serial.readStringUntil('\r') + '\r';
    Serial.readStringUntil('\n'); //read of everything else 
    mySerial.print(msg);
  }
}
