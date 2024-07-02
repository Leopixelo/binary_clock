#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
// #include <ESP32Time.h> // internal RTC
#include <RTClib.h>  // external RTC (DS3231)
#include <WiFi.h>
#include <time.h>

#include "config.cpp"

#define LED_BUILTIN 15

// const int NUM_PIXELS = 104;
const int NUM_PIXELS = 16;
const int LED_PIN = 16;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// ESP32Time rtc(3600);  // offset in seconds GMT+1
RTC_DS3231 rtc;

void setup() {  //
    pixels.begin();
    Serial.begin(115200);

    delay(1000);  // to give computer time to connect to serial port

    // initialize external RTC
    if (!rtc.begin()) {
        Serial.println("RTC module is NOT found");
        while (1);
    }

    Serial.println();
    Serial.print("[WiFi] Connecting to ");
    Serial.println(wifi_ssid);

    WiFi.begin(wifi_ssid, wifi_passwd);
    // auto reconnect is set true as default
    // to set auto connect off, use the following function
    // WiFi.setAutoReconnect(false);

    // will try for about 10 seconds (20x 500ms)
    const int wifi_retry_delay = 500;
    int wifi_retry_count = 20;

    bool wifi_connected = false;

    // wait for the WiFi event
    while (wifi_retry_count > 0 && wifi_connected == false) {
        wifi_retry_count--;

        switch (WiFi.status()) {
            case WL_NO_SSID_AVAIL:
                Serial.println("[WiFi] SSID not found");
                break;
            case WL_CONNECT_FAILED:
                Serial.print("[WiFi] Failed - WiFi not connected!");
                wifi_retry_count = 0;
                break;
            case WL_CONNECTION_LOST:
                Serial.println("[WiFi] Connection was lost");
                break;
            case WL_SCAN_COMPLETED:
                Serial.println("[WiFi] Scan is completed");
                break;
            case WL_DISCONNECTED:
                Serial.println("[WiFi] WiFi is disconnected");
                break;
            case WL_CONNECTED:
                Serial.println("[WiFi] WiFi is connected!");
                Serial.print("[WiFi] IP address: ");
                Serial.println(WiFi.localIP());
                wifi_connected = true;
                break;
            default:
                Serial.print("[WiFi] WiFi Status: ");
                Serial.println(WiFi.status());
                break;
        }

        delay(wifi_retry_delay);
    }

    if (wifi_retry_count == 0) {
        Serial.print("[WiFi] Failed to connect to WiFi!");
        // Use disconnect function to force stop trying to connect
        WiFi.disconnect();
    }

    configTime(0, 3600 * 2, "pool.ntp.org");

    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // set RTC time to compile time
}

void loop() {
    // DateTime now = rtc.now();
    // Serial.print("ESP32 RTC Date Time: ");
    // Serial.print(now.year(), DEC);
    // Serial.print('/');
    // Serial.print(now.month(), DEC);
    // Serial.print('/');
    // Serial.print(now.day(), DEC);
    // Serial.print(" (");
    // Serial.print(now.dayOfTheWeek());
    // Serial.print(") ");
    // Serial.print(now.hour(), DEC);
    // Serial.print(':');
    // Serial.print(now.minute(), DEC);
    // Serial.print(':');
    // Serial.println(now.second(), DEC);

    delay(1000);

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return;
    }

    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    Serial.print("Day of week: ");
    Serial.println(&timeinfo, "%A");
    Serial.print("Month: ");
    Serial.println(&timeinfo, "%B");
    Serial.print("Day of Month: ");
    Serial.println(&timeinfo, "%d");
    Serial.print("Year: ");
    Serial.println(&timeinfo, "%Y");
    Serial.print("Hour: ");
    Serial.println(&timeinfo, "%H");
    Serial.print("Hour (12 hour format): ");
    Serial.println(&timeinfo, "%I");
    Serial.print("Minute: ");
    Serial.println(&timeinfo, "%M");
    Serial.print("Second: ");
    Serial.println(&timeinfo, "%S");
}
