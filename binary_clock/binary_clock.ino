#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

#define LED_BUILTIN 15

const int NUM_PIXELS = 10;
const int LED_PIN = 16;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {  //
    // pinMode(LED_BUILTIN, OUTPUT);
    pixels.begin();
}

void loop() {
    // digitalWrite(LED_BUILTIN, HIGH);
    // delay(1000);
    // digitalWrite(LED_BUILTIN, LOW);
    // delay(1000);

    for (int i = 0; i < NUM_PIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(64, 16, 32));
        pixels.show();
        delay(100);
    }
}
