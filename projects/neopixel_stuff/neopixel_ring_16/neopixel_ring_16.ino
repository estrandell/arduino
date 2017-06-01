#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#include "colorLib.h"

#define PIN 6
#define NUM_PIXELS 16
uint16_t brightness = 40; 

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, PIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

HSV pixels[NUM_PIXELS];
HSV lerpTarget;
int16_t dropPixel = 0;

void setup() {
  strip.begin();
  strip.setBrightness(brightness);
  strip.show(); // Initialize all pixels to 'off'

  Serial.begin(9600);
  randomSeed(analogRead(0));

  dropPixel  = random(strip.numPixels());
  lerpTarget = HSVClockRandom();
}

void loop() {
  colorDropHSV(25);
}

void colorDropHSV(uint8_t wait){

    // set main color
    pixels[dropPixel] = LerpHSV(pixels[dropPixel], lerpTarget, 0.05f);

    for (int i=0; i<strip.numPixels();i++){
      if (i != dropPixel){

        int a = (i-dropPixel)%strip.numPixels();
        int b = (dropPixel-i)%strip.numPixels();
        if ( a < b)
          pixels[i] = LerpHSV(pixels[i], pixels[(i-1+NUM_PIXELS)%NUM_PIXELS], 0.3f);
        else
          pixels[i] = LerpHSV(pixels[i], pixels[(i+1+NUM_PIXELS)%NUM_PIXELS], 0.3f);
        
        pixels[i].v = Clamp01(pixels[i].v - 0.01);
      }

      strip.setPixelColor(i, pixels[i].ToRgb().ToUInt32());
      
      if (abs(pixels[dropPixel].h - lerpTarget.h) < 0.01f){
        dropPixel           = random(strip.numPixels());
        lerpTarget          = HSVClockRandom();
      }
    }

    strip.show();
    delay(wait);
}

