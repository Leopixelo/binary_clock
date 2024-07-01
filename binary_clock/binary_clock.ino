#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

#define LED_BUILTIN 15

// const int NUM_PIXELS = 104;
const int NUM_PIXELS = 77;
const int LED_PIN = 16;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {  //
    pinMode(LED_BUILTIN, OUTPUT);
    pixels.begin();

    Serial.begin(115200);
}

void loop() {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);

    Serial.write("testing");

    for (int i = 0; i < NUM_PIXELS; i++) {
        for (int j = 0; j < NUM_PIXELS; j++) {
            if (j == i) {
                pixels.setPixelColor(j, pixels.Color(16, 16, 16));
            } else {
                pixels.setPixelColor(j, pixels.Color(0, 0, 0));
            }
        }
        pixels.show();
        delay(100);
    }
}
