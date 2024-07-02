#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <BH1750.h>
// #include <ESP32Time.h> // internal RTC
#include <RTClib.h>  // external RTC (DS3231)
#include <WiFi.h>
#include <time.h>

#include "config.cpp"

#define LED_BUILTIN 15

// const int NUM_PIXELS = 104;
const int NUM_PIXELS = 16;
const int LED_PIN = 16;
const char ntp_server[] = "pool.ntp.org";
const long gmt_offset_sec = 0;
const int daylight_offset_sec = 3600 * 2;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// ESP32Time rtc(3600);  // offset in seconds GMT+1
RTC_DS3231 rtc;

BH1750 light_meter;

bool wifi_connected = false;

void setup() {  //
    pixels.begin();
    Serial.begin(115200);

    delay(1000);  // to give computer time to connect to serial port

    // initialize external RTC
    if (!rtc.begin()) {
        Serial.println("RTC module is NOT found");
        while (1);
    }

    light_meter.begin();

    configure_wifi();

    if (wifi_connected) {
        configTime(gmt_offset_sec, daylight_offset_sec, ntp_server);
        delay(500);
        configTime(gmt_offset_sec, daylight_offset_sec, ntp_server);

        set_rtc_to_ntp();
    } else {
        Serial.println("couldn't get time from NTP server because wifi is not connected");
    }

    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // set RTC time to compile time
}

void loop() {
    float lux = light_meter.readLightLevel();
    Serial.print("Light: ");
    Serial.print(lux);
    Serial.println(" lx");
    delay(200);

    DateTime now = rtc.now();
    Serial.print("ESP32 RTC Date Time: ");
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" (");
    Serial.print(now.dayOfTheWeek());
    Serial.print(") ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.println(now.second(), DEC);

    delay(1000);

    // struct tm timeinfo;
    // get_local_time(&timeinfo);

    // Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    // Serial.print("Day of week: ");
    // Serial.println(&timeinfo, "%A");
    // Serial.print("Month: ");
    // Serial.println(&timeinfo, "%B");
    // Serial.print("Day of Month: ");
    // Serial.println(&timeinfo, "%d");
    // Serial.print("Year: ");
    // Serial.println(&timeinfo, "%Y");
    // Serial.print("Hour: ");
    // Serial.println(&timeinfo, "%H");
    // Serial.print("Hour (12 hour format): ");
    // Serial.println(&timeinfo, "%I");
    // Serial.print("Minute: ");
    // Serial.println(&timeinfo, "%M");
    // Serial.print("Second: ");
    // Serial.println(&timeinfo, "%S");
}

void set_rtc_to_ntp() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("failed to obtain time");
        return;
    }

    if (timeinfo.tm_year > 100) {
        timeinfo.tm_year -= 100;
    }

    rtc.adjust(DateTime(timeinfo.tm_year, timeinfo.tm_mon, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec));

    Serial.print("got and set following time from NTP server: ");
    print_time(&timeinfo);
}

void print_time(tm* timeinfo) {
    Serial.println(timeinfo, "%Y-%m-%d %H:%M:%S");
    // Serial.println(timeinfo, "%A, %B %d %Y %H:%M:%S");
    // Serial.print("Day of week: ");
    // Serial.println(timeinfo, "%A");
    // Serial.print("Month: ");
    // Serial.println(timeinfo, "%B");
    // Serial.print("Day of Month: ");
    // Serial.println(timeinfo, "%d");
    // Serial.print("Year: ");
    // Serial.println(timeinfo, "%Y");
    // Serial.print("Hour: ");
    // Serial.println(timeinfo, "%H");
    // Serial.print("Hour (12 hour format): ");
    // Serial.println(timeinfo, "%I");
    // Serial.print("Minute: ");
    // Serial.println(timeinfo, "%M");
    // Serial.print("Second: ");
    // Serial.println(timeinfo, "%S");
}

void configure_wifi() {
    Serial.print("[WiFi] Connecting to ");
    Serial.println(wifi_ssid);

    WiFi.begin(wifi_ssid, wifi_passwd);
    // auto reconnect is set true as default
    // to set auto connect off, use the following function
    // WiFi.setAutoReconnect(false);

    // will try for about 10 seconds (20x 500ms)
    const int wifi_retry_delay = 500;
    int wifi_retry_count = 20;

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
}
