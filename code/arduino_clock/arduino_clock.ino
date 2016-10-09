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
#define LDR_PIN             0   // analog pin
#define HOURS_BUTTON_PIN    2   // interrupt pin
#define MIN_BUTTON_PIN      3   // interrupt pin
#define TOGGLE_PIN          4

// Constants
#define LDR_THRESHOLD_LOW   300
#define LDR_THRESHOLD_HIGH  600
#define NUMPIXELS_DIGITS    21
#define NUMPIXELS_HOURS     12
#define NUMSEGMENTS         7
#define NUMSTRIPS           3

// Real-time clock
RTC_DS3231 rtc;
DateTime currTime;
int currHour, prevHour;
int currMinute, prevMinute;
bool summerTime;

int photocellReading;

// Setup NeoPixel library: create the different LED strips
Adafruit_NeoPixel digit1Strip = Adafruit_NeoPixel(NUMPIXELS_DIGITS, DIGIT1_PIN,
                                                    NEO_RGB + NEO_KHZ400);
Adafruit_NeoPixel digit2Strip = Adafruit_NeoPixel(NUMPIXELS_DIGITS, DIGIT2_PIN, 
                                                    NEO_RGB + NEO_KHZ400);
Adafruit_NeoPixel hoursStrip = Adafruit_NeoPixel(NUMPIXELS_HOURS, HOURS_PIN, 
                                                    NEO_RGB + NEO_KHZ400);                                             
Adafruit_NeoPixel strips [NUMSTRIPS] = { digit1Strip, digit2Strip, hoursStrip };

uint32_t colorLight = hoursStrip.Color(220, 220, 255);
uint32_t colorDark = hoursStrip.Color(50, 0, 0);
uint32_t colorOff = hoursStrip.Color(0, 0, 0);
uint32_t currColor = colorDark;

// Segments:
//   1
// 0   2
//   3
// 4   6
//   5
// Activated segments per digit
int segments [10][7] = {
    { true, true, true, false, true, true, true },     // 0: 0, 1, 2, 4, 5, 6
    { false, false, true, false, false, false, true }, // 1: 2, 6
    { false, true, true, true, true, true, false },    // 2: 1, 2, 3, 4, 5
    { false, true, true, true, false, true, true },    // 3: 1, 2, 3, 5, 6
    { true, true, false, true, false, false, true },   // 4: 0, 1, 3, 6
    { true, true, false, true, false, true, true },    // 5: 0, 1, 3, 5, 6
    { true, true, false, true, true, true, true },     // 6: 0, 1, 3, 4, 5, 6
    { false, true, true, false, false, false, true },  // 7: 1, 2, 6
    { true, true, true, true, true, true, true },      // 8: 0, 1, 2, 3, 4, 5, 6
    { true, true, true, true, false, true, true }      // 9: 0, 1, 2, 3, 5, 6
};

void setup() {
    Serial.begin(9600);
  
// This is for Trinket 5V 16MHz, 
// you can remove these three lines if you are not using a Trinket
#if defined (__AVR_ATtiny85__)
    if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
#endif

    for (int i = 0; i < NUMSTRIPS; i++) {
        strips[i].begin(); // This initializes the NeoPixel library.
        strips[i].setBrightness(30);
    }

    pinMode(TOGGLE_PIN, INPUT);

    // Wait for RTC
    while (! rtc.begin()) {
        Serial.println("Couldn't find RTC");
        delay(100);
    }

    // Set the time to the date & time this sketch was compiled
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

    // Interrupts
    attachInterrupt(digitalPinToInterrupt(HOURS_BUTTON_PIN), 
                    increase_hours, RISING);
    attachInterrupt(digitalPinToInterrupt(MIN_BUTTON_PIN), 
                    increase_minutes, RISING);
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


    // Read toggle switch for summer time
    summerTime = (digitalRead(TOGGLE_PIN) == HIGH);


    // Update shown time
    currTime = rtc.now();
    currHour = currTime.hour();
    currMinute = currTime.minute();

    if (currHour != prevHour) {
        prevHour = currHour;
        if (summerTime) {
            showHour(currHour, currColor);
        } else {
            showHour(currHour - 1, currColor);
        }
    }
    if (currMinute != prevMinute) {
        prevMinute = currMinute;
        showMinute(currMinute, currColor);
    }

}

void showHour(int hour, uint32_t color) {
    // Hour: 0 - 23
    // Connection: first LED in the hours strip is at hour 1
    if (hour > 12) {
        hoursStrip.setPixelColor(11, color);
        hour -= 12;
    }
    for (int i = 0; i < NUMPIXELS_HOURS; i++) {
        if (i < hour) {
            hoursStrip.setPixelColor(i, color);
        } else {
            hoursStrip.setPixelColor(i, colorOff);
        }
    }
}

void showMinute(int minute, uint32_t color) {
    showDigit(digit1Strip, minute / 10, color);
    showDigit(digit2Strip, minute % 10, color);
}

// Digit connection scheme:
//   03 04 05 
// 02        06
// 01        07
// 00        08
//   11 10 09
// 12        20
// 13        19
// 14        18
//   15 16 17 
void showDigit(Adafruit_NeoPixel strip, int digit, uint32_t color) {
    for (int i = 0; i < NUMPIXELS_DIGITS; i++) {
        if (segments[digit][i / NUMSEGMENTS]) {
            strip.setPixelColor(i, color);
        } else {
            strip.setPixelColor(i, colorOff);
        }
    }
}

void increase_hours() {
    rtc.adjust(rtc.now() + TimeSpan(3600));
}

void increase_minutes() {
    rtc.adjust(rtc.now() + TimeSpan(60));
}
