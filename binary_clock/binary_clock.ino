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
const int NUM_PIXELS = 24;
const int LED_PIN = 16;
const int WIFI_SWITCH_PIN = 1;
const int HOUR_BUTTON_PIN = 2;
const int MINUTE_BUTTON_PIN = 3;
const int RTC_INTERRUPT_PIN = 10;

// reference for timezones: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
const String timezone = "CET-1CEST,M3.5.0,M10.5.0/3";  // timezone for Europe/Berlin
const char ntp_server[] = "pool.ntp.org";
const long gmt_offset_sec = 0;  // not needed if timezone is set
// const int daylight_offset_sec = 3600 * 2;  // not needed if timezone is set
const int daylight_offset_sec = 0;  // not needed if timezone is set

unsigned int button_debounce_time = 200;  // in ms
unsigned int switch_debounce_time = 100;  // in ms

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// ESP32Time rtc(3600);  // offset in seconds GMT+1
RTC_DS3231 rtc;

BH1750 light_meter;

bool wifi_initially_connected = false;

bool wifi_switch_changed = false;
bool hour_button_pressed = false;
bool minute_buttom_pressed = false;
bool rtc_interrupt = false;

unsigned long wifi_switch_last_changed = 0;
unsigned long hour_button_last_pressed = 0;
unsigned long minute_button_last_pressed = 0;

void IRAM_ATTR handle_wifi_switch_interrupt() {  //
    wifi_switch_changed = true;
    wifi_switch_last_changed = millis();
}
void IRAM_ATTR handle_hour_button_interrupt() {  //
    unsigned long current_time = millis();

    // debounce button
    if (current_time - hour_button_last_pressed > button_debounce_time) {
        hour_button_pressed = true;
        hour_button_last_pressed = current_time;
    }
}
void IRAM_ATTR handle_minute_button_interrupt() {  //
    unsigned long current_time = millis();

    // debounce button
    if (current_time - minute_button_last_pressed > button_debounce_time) {
        minute_buttom_pressed = true;
        minute_button_last_pressed = current_time;
    }
}
void IRAM_ATTR handle_rtc_interrupt() {  //
    rtc_interrupt = true;
    // Serial.println("rtc interrupt");
}

void setup() {  //
    pinMode(WIFI_SWITCH_PIN, INPUT_PULLDOWN);
    pinMode(HOUR_BUTTON_PIN, INPUT_PULLDOWN);
    pinMode(MINUTE_BUTTON_PIN, INPUT_PULLDOWN);

    pixels.begin();
    Serial.begin(115200);

    delay(1000);  // to give computer time to connect to serial port

    // initialize external RTC
    if (!rtc.begin()) {
        Serial.println("RTC module is NOT found");
        while (1);
    }

    light_meter.begin();

    if (digitalRead(WIFI_SWITCH_PIN)) {
        configure_wifi();

        update_time();
    } else {
        Serial.println("WiFi disabled by switch");
    }

    attachInterrupt(WIFI_SWITCH_PIN, &handle_wifi_switch_interrupt, CHANGE);
    attachInterrupt(HOUR_BUTTON_PIN, &handle_hour_button_interrupt, RISING);
    attachInterrupt(MINUTE_BUTTON_PIN, &handle_minute_button_interrupt, RISING);

    rtc.writeSqwPinMode(DS3231_SquareWave1Hz);
    attachInterrupt(RTC_INTERRUPT_PIN, &handle_rtc_interrupt, FALLING);

    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // set RTC time to compile time
}

void loop() {
    if (rtc_interrupt) {
        rtc_interrupt = false;
        display_time();
    }
    if (wifi_switch_changed && millis() - wifi_switch_last_changed > switch_debounce_time) {
        wifi_switch_changed = false;
        process_wifi_switch_change();
    }
    if (hour_button_pressed) {
        hour_button_pressed = false;
        process_hour_button_press();
    }
    if (minute_buttom_pressed) {
        minute_buttom_pressed = false;
        process_minute_button_press();
    }
    delayMicroseconds(250);
}

void process_wifi_switch_change() {
    if (digitalRead(WIFI_SWITCH_PIN)) {
        if (!WiFi.isConnected()) {
            configure_wifi();
        }

        update_time();
    } else {
        WiFi.disconnect();
    }
}

void process_hour_button_press() {
    DateTime current_time = rtc.now();

    // increment hour
    DateTime new_time = DateTime(current_time.year(), current_time.month(), current_time.day(), (current_time.hour() + 1) % 24, current_time.minute(),
                                 current_time.second());

    rtc.adjust(new_time);

    Serial.print("new time: ");
    print_time(new_time);
}

void process_minute_button_press() {
    DateTime current_time = rtc.now();

    // increment minute and reset seconds
    DateTime new_time =
        DateTime(current_time.year(), current_time.month(), current_time.day(), current_time.hour(), (current_time.minute() + 1) % 60, 0);

    rtc.adjust(new_time);

    Serial.print("new time: ");
    print_time(new_time);
}

void display_time() {
    DateTime now = rtc.now();

    // print_time(now);

    uint8_t hour = now.hour();
    uint8_t minute = now.minute();
    uint8_t second = now.second();

    uint8_t hour_0 = hour / 10;
    uint8_t hour_1 = hour % 10;

    uint8_t minute_0 = minute / 10;
    uint8_t minute_1 = minute % 10;

    uint8_t second_0 = second / 10;
    uint8_t second_1 = second % 10;

    uint32_t color = pixels.Color(16, 16, 16);

    display_digit(hour_0, color, 0, true);
    display_digit(hour_1, color, 4, false);
    display_digit(minute_0, color, 8, true);
    display_digit(minute_1, color, 12, false);
    display_digit(second_0, color, 16, true);
    display_digit(second_1, color, 20, false);

    pixels.show();
}

void display_digit(uint8_t digit, uint32_t color, uint8_t offset, bool most_significant_bit_first) {
    const uint8_t end_index = offset + 4;

    uint8_t mask = 0b0001;

    if (most_significant_bit_first) {
        mask = 0b1000;
    }

    for (uint8_t i = offset; i < end_index; i++) {
        if ((bool)(digit & mask)) {
            pixels.setPixelColor(i, color);
            // Serial.print("1");
        } else {
            pixels.setPixelColor(i, 0);
            // Serial.print("0");
        }

        if (most_significant_bit_first) {
            mask >>= 1;
        } else {
            mask <<= 1;
        }
    }
}

float get_light() {
    float lux = light_meter.readLightLevel();
    return lux;
}

void set_rtc_to_ntp() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("failed to obtain time");
        return;
    }

    // Serial.println(timeinfo.tm_year);

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

void print_time(DateTime datetime) {
    Serial.print(datetime.year());
    Serial.print("-");
    Serial.printf("%02d", datetime.month());
    Serial.print("-");
    Serial.printf("%02d", datetime.day());
    Serial.print(" ");
    Serial.printf("%02d", datetime.hour());
    Serial.print(":");
    Serial.printf("%02d", datetime.minute());
    Serial.print(":");
    Serial.printf("%02d\n", datetime.second());
}

void update_time() {
    if (wifi_initially_connected) {
        configTime(gmt_offset_sec, daylight_offset_sec, ntp_server);
        delay(500);
        configTime(gmt_offset_sec, daylight_offset_sec, ntp_server);

        set_timezone(timezone);

        set_rtc_to_ntp();
    } else {
        Serial.println("couldn't get time from NTP server because WiFi is not connected");
    }
}

void set_timezone(String timezone) {
    Serial.printf("setting Timezone to %s\n", timezone.c_str());
    setenv("TZ", timezone.c_str(), 1);  //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
    tzset();
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
    while (wifi_retry_count > 0 && wifi_initially_connected == false) {
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
                wifi_initially_connected = true;
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
