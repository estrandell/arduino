******************************
CONNECT EVERYTHING
******************************

This is far from the whole truth, ill post diagram in the future
https://diyhacking.com/esp8266-tutorial/



******************************
FLASH ESP8266
******************************

*** Flash ESP
run: esp8266_flasher.exe
bin: v1.3.0.2 AT Firmware.bin

*** Set baud rate to ESP8266
How to change the baud rate on the ESP8266 will depend on the firmware version. 
I've had success with AT+IPR=9600. You only need to run this command once (it's a persistent setting).
> AT -> OK
> AT+IPR=9600 

>AT+CWMODE?
>AT+CWMODE=3
>AT+CWLAP
		
>AT+RST		- Reset
>AT+GMR		- Check firmware
>AT+RESTORE	- Restores the Factory Default Settings



******
UPLOAD SKETCH
******


1. Need to install ESP8266 in board manager.

File->Preferences add the following URL tp Additional Board Manager
http://arduino.esp8266.com/stable/package_esp8266com_index.json

Tools->Board: XXX ->Board Manager
Install ESP8266 by ESP8266 Community


2. When compiling, set Board type to Generic ESP8266 Module in list.

3. When uploading be sure to set Flash pin to GRD when resetting the module to enter Flash mode.

Compile, upload, and have fun.




