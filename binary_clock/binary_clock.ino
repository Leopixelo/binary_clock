#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <ESP32Time.h>

#define LED_BUILTIN 15

// const int NUM_PIXELS = 104;
const int NUM_PIXELS = 77;
const int LED_PIN = 16;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

ESP32Time rtc(3600);  // offset in seconds GMT+1

void setup() {  //
    pixels.begin();

    Serial.begin(115200);

    rtc.setTime(30, 24, 15, 17, 1, 2021);
}

void loop() {
    Serial.println(rtc.getTime("%A, %B %d %Y %H:%M:%S"));
    delay(1000);
}
