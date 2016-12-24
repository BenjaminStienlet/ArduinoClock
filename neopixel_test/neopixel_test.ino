// NeoPixel Ring simple sketch (c) 2013 Shae Erisson
// released under the GPLv3 license to match the rest of the AdaFruit NeoPixel library

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#include <Wire.h>
#include "RTClib.h"

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1
#define PIN            5
#define LDR_PIN        0
#define TOGGLE_PIN     4

RTC_DS3231 rtc;

#define LDR_THRESHOLD_LOW   300
#define LDR_THRESHOLD_HIGH  600

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS      21

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_RGB + NEO_KHZ400);

int delayval = 500; // delay for half a second

void setup() {
//  Serial.begin(9600);
  
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
#if defined (__AVR_ATtiny85__)
  if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
#endif
  // End of trinket special code

  strip.begin(); // This initializes the NeoPixel library.
  strip.setBrightness(30);

  pinMode(TOGGLE_PIN, INPUT);

  if (! rtc.begin()) {
//    Serial.println("Couldn't find RTC");
    while (1);
  }

  // Set the time to the date & time this sketch was compiled
//  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

void loop() {
  DateTime now = rtc.now();
//  Serial.print(now.hour(), DEC);
//  Serial.print(':');
//  Serial.print(now.minute(), DEC);
//  Serial.print(':');
//  Serial.print(now.second(), DEC);
//  Serial.println();
    
//  simple();
  if (digitalRead(TOGGLE_PIN) == HIGH) {
    int photocellReading = analogRead(LDR_PIN);  
 
//    Serial.print("Analog reading = ");
//    Serial.println(photocellReading);

    if (photocellReading > LDR_THRESHOLD_LOW) {
      colorWipe(strip.Color(220, 220, 255), 1000);
    } else {
      colorWipe(strip.Color(50, 0, 0), 1000);
    }
  } else {
    rainbowComplex(100);
  }
//  rainbow(50);
//  rainbowCycle(50);


}

void simple() {
  
  for(int i=0; i<256; i+=10) {
    Serial.print(i);
    Serial.print("\n");
    for(int j=0;j<NUMPIXELS;j++){
  
      // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
      strip.setPixelColor(j, strip.Color(i,i,i));
  
      strip.show(); // This sends the updated pixel color to the hardware.
  
      delay(100); // Delay for a period of time (in milliseconds).
  
    }
    delay(100);
  }
}



// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

void rainbowComplex(uint8_t wait) {
  uint16_t i, j;
  bool up;
  
  i = 0;
  up = true;
  
  for(j=0; j<256; j+=4) {
    strip.setPixelColor(i, Wheel(j));
    strip.show();
    delay(wait);
    i = (i + 1) % strip.numPixels();
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
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
