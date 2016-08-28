
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#include <Wire.h>
#include "RTClib.h"

// Pins
#define DIGIT1_PIN          5
#define DIGIT2_PIN          6
#define HOURS_PIN           7
#define LDR_PIN             0
#define TOGGLE_PIN          9
#define HOURS_BUTTON_PIN    10
#define MIN_BUTTON_PIN      11

// Constants
#define LDR_THRESHOLD_LOW   300
#define LDR_THRESHOLD_HIGH  600
#define NUMPIXELS_DIGITS    21
#define NUMPIXELS_HOURS     12

// Real-time clock
RTC_DS3231 rtc;
DateTime currTime;
int currHour, prevHour;
int currMinute, prevMinute;


int photocellReading;
uint32_t colorLight = strip.Color(220, 220, 255);
uint32_t colorDark = strip.Color(50, 0, 0);
uint32_t currColor = colorDark;

// Setup NeoPixel library: create the different LED strips
Adafruit_NeoPixel digit1Strip = Adafruit_NeoPixel(NUMPIXELS_DIGITS, DIGIT1_PIN,
                                                    NEO_RGB + NEO_KHZ400);
Adafruit_NeoPixel digit2Strip = Adafruit_NeoPixel(NUMPIXELS_DIGITS, DIGIT2_PIN, 
                                                    NEO_RGB + NEO_KHZ400);
Adafruit_NeoPixel hoursStrip = Adafruit_NeoPixel(NUMPIXELS_HOURS, HOURS_PIN, 
                                                    NEO_RGB + NEO_KHZ400);
// strips = ...

void setup() {
    Serial.begin(9600);
  
// This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
#if defined (__AVR_ATtiny85__)
    if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
#endif

    // for strip in strips
    strip.begin(); // This initializes the NeoPixel library.
    strip.setBrightness(30);
    // end-for

    pinMode(TOGGLE_PIN, INPUT);

    // Wait for RTC
    while (! rtc.begin()) {
        Serial.println("Couldn't find RTC");
        delay(100);
    }

    // Set the time to the date & time this sketch was compiled
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

void loop() {

    // Set color based on LDR reading
    photocellReading = analogRead(LDR_PIN);
    if (currColor == colorLight && photocellReading < LDR_THRESHOLD_LOW) {
        currColor = colorDark;
    }
    if (currColor == colorDark && photocellReading > LDR_THRESHOLD_HIGH) {
        currColor = colorLight;
    }


    // Update shown time
    currTime = rtc.now();
    currHour = currTime.hour();
    currMinute = currTime.minute();

    if (currHour != prevHour) {
        prevHour = currHour;
        showHour(currHour, currColor);
    }
    if (currMinute != prevMinute) {
        prevMinute = currMinute;
        showMinute(currMinute, currColor);
    }
    

    // switch reading pins to interrupt based
    if (digitalRead(TOGGLE_PIN) == HIGH) {
        colorWipe(strip.Color(220, 220, 255), 1000);
    } else {
        rainbowComplex(100);
    }

}

void showHour(int hour, uint32_t color) {
    // Hour: 0 - 23
    // First LED in the hours strip is at hour 1
    if (hour > 12) {
        hoursStrip.setPixelColor(11, color);
        hour -= 12;
    }
    for (int i = 0; i < hour; i++) {
        hoursStrip.setPixelColor(i, color);
    }
}

void showMinute(int minute, uint32_t color) {
    showDigit(digit1Strip, minute / 10, color);
    showDigit(digit2Strip, minute % 10, color);
}

void showDigit(Adafruit_NeoPixel strip, int digit, uint32_t color) {
    // Connection:
    //   03 04 05 
    // 02        06
    // 01        07
    // 00        08
    //   11 10 09
    // 12        20
    // 13        19
    // 14        18
    //   15 16 17 

    // Segments:
    //   1
    // 0   2
    //   3
    // 4   6
    //   5
    // Segments per digit:
    // 0: 0, 1, 2, 4, 5, 6
    // 1: 2, 6
    // 2: 1, 2, 3, 4, 5
    // 3: 1, 2, 3, 5, 6
    // 4: 0, 1, 3, 6
    // 5: 0, 1, 3, 5, 6
    // 6: 0, 1, 3, 4, 5, 6
    // 7: 1, 2, 6
    // 8: 0, 1, 2, 3, 4, 5, 6
    // 9: 0, 1, 2, 3, 5, 6
}
