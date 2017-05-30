#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#include "colorLib.h"

#define PIN 2

Adafruit_NeoPixel strip = Adafruit_NeoPixel(300, PIN, NEO_GRB + NEO_KHZ800);
uint16_t brightness = 40; 

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

const uint16_t iNumDrops = 6;
int16_t dropPixel[iNumDrops];
int16_t dropState[iNumDrops]; // 0 = uninitiated, 1 = fade in, 2 = fade out


void setup() {

  strip.begin();
  strip.setBrightness(brightness);
  strip.show(); // Initialize all pixels to 'off'

  Serial.begin(9600);

  randomSeed(analogRead(0));

  for(int i =0; i<iNumDrops; i++){
    dropPixel[i] = -1;
    dropState[i] = 0;
  }
}

void loop() {

  colorDropHSV(25);
}


void colorDropHSV(uint8_t wait){

    for(uint16_t i=0; i<iNumDrops; i++) {
      uint16_t pixel = dropPixel[i];
      uint16_t state = dropState[i];

      if (state == 0){
        if (random(20) < 1) {
          int newPixel = random(strip.numPixels());
          for(uint16_t j=0; j<iNumDrops; j++) {
            if (newPixel > dropPixel[j]-3 && newPixel < dropPixel[j]+3)
              return; 
          }
          
          dropPixel[i] = newPixel;
          dropState[i] = 1; // fade in
          strip.setPixelColor(newPixel, Wheel(random(255)));
        }
      }
      else
      {
        float step = (state == 1 ? 0.05 : -0.02);
        step += random(10) / 1000.0;

        uint32_t current = strip.getPixelColor(pixel);
        RGBA rgb(current);
        HSV  hsv = Rgb2Hsv(rgb);

        float v = Clamp01(hsv.v + step);

        uint32_t c = HSV(hsv.h, hsv.s, v, hsv.a).ToRgb().ToUInt32();
        strip.setPixelColor(pixel,   c);

        c     = HSV(hsv.h, hsv.s, Clamp01(v-0.15), hsv.a).ToRgb().ToUInt32();
        strip.setPixelColor(pixel-1, c);
        strip.setPixelColor(pixel+1, c);

        c     = HSV(hsv.h, hsv.s, Clamp01(v-0.30), hsv.a).ToRgb().ToUInt32();
        strip.setPixelColor(pixel-2, c);
        strip.setPixelColor(pixel+2, c);

        if (v >= 1.0){
            dropState[i] = 2;
        }
        else if (v <= 0.0){
            dropState[i] = 0;
            dropPixel[i] = -1;
        }
      }
    }

    strip.show();
    delay(wait);
}




// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels()/2; i++) {
    strip.setPixelColor(i,                     c);
    strip.setPixelColor(i+strip.numPixels()/2, c);
    strip.show();
    delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels()/2; i++) {
      strip.setPixelColor(i,                         Wheel((i+j) & 255));
      strip.setPixelColor(strip.numPixels() - 1 - i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait, int itr) {
  uint16_t i, j;

  for(j=0; j<256*itr; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels()/2; i++) {
      strip.setPixelColor(i,                     Wheel(((i * 256 / strip.numPixels()) + j) & 255));
      strip.setPixelColor(i+strip.numPixels()/2, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
      for (int i=0; i < strip.numPixels()/2; i=i+3) {
        strip.setPixelColor(i+q,                           Wheel( (i+j) % 255));    //turn every third pixel on
        strip.setPixelColor(strip.numPixels() - 1 - (i+q), Wheel( (i+j) % 255));    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (int i=0; i < strip.numPixels()/2; i=i+3) {
        strip.setPixelColor(i+q,                     0);        //turn every third pixel off
        strip.setPixelColor(strip.numPixels() - (i+q), 0);        //turn every third pixel off
      }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
