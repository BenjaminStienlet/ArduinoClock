#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#include <Wire.h>
#include "RTClib.h"

// Pins
#define DIGIT1_PIN          4
#define DIGIT2_PIN          5
#define HOURS_PIN           6
#define LDR_PIN             0   // analog pin
#define HOURS_BUTTON_PIN    9   // pin change interrupt PCINT0 (D8-D13)
#define MIN_BUTTON_PIN      10  // pin change interrupt PCINT0 (D8-D13)
#define TOGGLE_PIN          3

// Constants
#define LDR_THRESHOLD_LOW   300
#define LDR_THRESHOLD_HIGH  600
#define NUMPIXELS_DIGITS    21
#define NUMPIXELS_HOURS     12
#define NUMLEDS_PER_SEGMENT 3
#define NUMSTRIPS           3
#define BRIGHTNESS          10

// Real-time clock
RTC_DS3231 rtc;
DateTime currTime;
int currHour;
int prevHour = -1;
int currMinute;
int prevMinute = -1;
bool summerTime;
bool colorChanged = false;
volatile bool increase_hours_flag = false;
volatile unsigned long last_increase_hours = 0;
volatile bool increase_minutes_flag = false;
volatile unsigned long last_increase_minutes = 0;
unsigned long debounce_delay = 200;

int photocellReading;

// Setup NeoPixel library: create the different LED strips
Adafruit_NeoPixel digit1Strip = Adafruit_NeoPixel(NUMPIXELS_DIGITS, DIGIT1_PIN,
                                                    NEO_RGB + NEO_KHZ400);
Adafruit_NeoPixel digit2Strip = Adafruit_NeoPixel(NUMPIXELS_DIGITS, DIGIT2_PIN, 
                                                    NEO_RGB + NEO_KHZ400);
Adafruit_NeoPixel hoursStrip = Adafruit_NeoPixel(NUMPIXELS_HOURS, HOURS_PIN, 
                                                    NEO_RGB + NEO_KHZ400);                                             
Adafruit_NeoPixel *strips [NUMSTRIPS] = { &digit1Strip, &digit2Strip, &hoursStrip };

//uint32_t colorLight = hoursStrip.Color(30, 200, 255); // blue
uint32_t colorLight = hoursStrip.Color(255, 128, 0); // orange
uint32_t colorDark = hoursStrip.Color(24, 0, 0);
uint32_t colorOff = hoursStrip.Color(0, 0, 0);
uint32_t currColor = colorLight;

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
    { true, false, true, true, false, false, true },   // 4: 0, 2, 3, 6
    { true, true, false, true, false, true, true },    // 5: 0, 1, 3, 5, 6
    { true, true, false, true, true, true, true },     // 6: 0, 1, 3, 4, 5, 6
    { false, true, true, false, false, false, true },  // 7: 1, 2, 6
    { true, true, true, true, true, true, true },      // 8: 0, 1, 2, 3, 4, 5, 6
    { true, true, true, true, false, true, true }      // 9: 0, 1, 2, 3, 5, 6
};

void setup() {
    // Serial.begin(9600);
  
    // This is for Trinket 5V 16MHz, 
    // you can remove these three lines if you are not using a Trinket
    #if defined (__AVR_ATtiny85__)
    if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
    #endif

    for (int i = 0; i < NUMSTRIPS; i++) {
        strips[i]->begin(); // This initializes the NeoPixel library.
        strips[i]->setBrightness(BRIGHTNESS);
        strips[i]->setPixelColor(0, colorDark);
        strips[i]->show();
    }

    pinMode(TOGGLE_PIN, INPUT);

    // Wait for RTC
    while (! rtc.begin()) {
        // Serial.println("Couldn't find RTC");
        delay(100);
    }

    // Set the time to the date & time this sketch was compiled
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

    // Interrupts (arduino uno)
    // attachInterrupt(digitalPinToInterrupt(HOURS_BUTTON_PIN), 
    //                 increase_hours_interrupt, RISING);
    // attachInterrupt(digitalPinToInterrupt(MIN_BUTTON_PIN), 
    //                 increase_minutes_interrupt, RISING);

    // Pin change interrupts (arduino trinket pro)
    pinMode(HOURS_BUTTON_PIN, INPUT);
    pinMode(MIN_BUTTON_PIN, INPUT);
    digitalWrite(HOURS_BUTTON_PIN, HIGH);
    digitalWrite(MIN_BUTTON_PIN, HIGH);
    pciSetup(HOURS_BUTTON_PIN);
    pciSetup(MIN_BUTTON_PIN);
}

void loop() {
  mainLoop();
}

void mainLoop() {
    // Set color based on LDR reading
    photocellReading = analogRead(LDR_PIN);

    colorChanged = false;
    if (currColor == colorLight && photocellReading < LDR_THRESHOLD_LOW) {
        currColor = colorDark;
        colorChanged = true;
    }
    if (currColor == colorDark && photocellReading > LDR_THRESHOLD_HIGH) {
        currColor = colorLight;
        colorChanged = true;
    }

    // Read toggle switch for summer time
    summerTime = (digitalRead(TOGGLE_PIN) == HIGH);

    // Change time
    if (increase_hours_flag) {
      rtc.adjust(rtc.now() + TimeSpan(3600));
      increase_hours_flag = false;
    }
    if (increase_minutes_flag) {
      rtc.adjust(rtc.now() + TimeSpan(60));
      increase_minutes_flag = false;
    }

    // Update shown time
    currTime = rtc.now();
    currHour = currTime.hour();
    currMinute = currTime.minute();
    
    if (currHour != prevHour || colorChanged) {
        prevHour = currHour;
        if (summerTime) {
            showHour(currHour, currColor);
        } else {
            if (currHour == 0) {
                showHour(23, currColor);
            } else {
                showHour(currHour - 1, currColor);
            }
        }
    }
    if (currMinute != prevMinute || colorChanged) {
        prevMinute = currMinute;
        showMinute(currMinute, currColor);
    }
}

void testLoop() {
  uint32_t color = colorLight;
  for (int i = 0; i < 100; i++) {
    showMinute(i, color);

    for (int j = 1; j <= 12; j++) {
      showHour(j, color);
      delay(83);
    }
  }
}

void testLoop2() {
  for (int i = 0; i < NUMPIXELS_DIGITS; i++) {
    digit1Strip.setPixelColor(i, colorLight);
    digit1Strip.show();
    delay(100);
  }
  for (int i = 0; i < NUMPIXELS_DIGITS; i++) {
    digit1Strip.setPixelColor(i, colorOff);
    digit1Strip.show();
    delay(100);
  }
}

void testColor() {
  showDigit(&digit1Strip, 8, hoursStrip.Color(255, 128, 0));
  showDigit(&digit2Strip, 8, hoursStrip.Color(30, 200, 255));
//  showDigit(&digit2Strip, 8, hoursStrip.Color(50, 0, 0));
}

void showHour(int currHour, uint32_t color) {
    // Hour: 0 - 23
    // Connection: first LED in the hours strip is at hour 1
    if (currHour > 12) {
      currHour -= 12;
    }
    for (int i = 0; i < NUMPIXELS_HOURS; i++) {
        if (i < currHour) {
            hoursStrip.setPixelColor(i, color);
        } else {
            hoursStrip.setPixelColor(i, colorOff);
        }
    }
    hoursStrip.show();
}

void showMinute(int currMinute, uint32_t color) {
    showDigit(&digit1Strip, currMinute / 10, color);
    showDigit(&digit2Strip, currMinute % 10, color);
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
void showDigit(Adafruit_NeoPixel *strip, int digit, uint32_t color) {
    for (int i = 0; i < NUMPIXELS_DIGITS; i++) {
        if (segments[digit][i / NUMLEDS_PER_SEGMENT]) {
            strip->setPixelColor(i, color);
        } else {
            strip->setPixelColor(i, colorOff);
        }
    }
    strip->show();
}

void pciSetup(byte pin) {
    *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));  // enable pin
    PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
    PCICR  |= bit (digitalPinToPCICRbit(pin)); // enable interrupt for the group
}

ISR(PCINT0_vect) {
    if (digitalRead(HOURS_BUTTON_PIN) == HIGH) {
        if (millis() - last_increase_hours > debounce_delay) {
            increase_hours_interrupt();
            last_increase_hours = millis();
        }
    }
    if (digitalRead(MIN_BUTTON_PIN) == HIGH) {
        if (millis() - last_increase_minutes > debounce_delay) {
            increase_minutes_interrupt();
            last_increase_minutes = millis();
        }
    }
}

void increase_hours_interrupt() {
    increase_hours_flag = true;
}

void increase_minutes_interrupt() {
    increase_minutes_flag = true;
}

