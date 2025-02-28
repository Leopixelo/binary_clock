#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <BH1750.h>
// #include <ESP32Time.h> // internal RTC
#include <RTClib.h>  // external RTC (DS3231)
#include <WiFi.h>
#include <time.h>

#include "config.cpp"

const uint16_t hue_degrees_factor = 182;

const uint16_t led_hue_degrees = 0;  // between 0 and 360
const uint16_t led_hue = led_hue_degrees * hue_degrees_factor;
const uint8_t led_saturation = 0;

// reference for timezones: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
const String timezone = "CET-1CEST,M3.5.0,M10.5.0/3";  // timezone for Europe/Berlin
const char ntp_server[] = "pool.ntp.org";
const long gmt_offset_sec = 0;  // not needed if timezone is set
// const int daylight_offset_sec = 3600 * 2;  // not needed if timezone is set
const int daylight_offset_sec = 0;  // not needed if timezone is set

const unsigned int button_debounce_time = 200;  // in ms
const unsigned int switch_debounce_time = 100;  // in ms

unsigned int status_counter = 0;
const unsigned int status_duration = 10;  // in seconds
const unsigned int status_counter_init = status_duration * 4000;
bool show_status = false;

// const int NUM_PIXELS = 104;
const int NUM_PIXELS = 24;
const int LED_PIN = 17;
const int WIFI_SWITCH_PIN = 1;
const int HOUR_BUTTON_PIN = 2;
const int MINUTE_BUTTON_PIN = 3;
const int STATUS_BUTTON_PIN = 4;
const int RTC_INTERRUPT_PIN = 10;
const int LIGHT_POTI_PIN = 8;

// defines how often the light adjustment runs
const unsigned int light_refresh_counter_init = 400;  // 4 kHz / 400 = 10 Hz
unsigned int light_refresh_counter = light_refresh_counter_init;
// defines size of rolling average
const unsigned int light_average_size = 50;  // 50 / 10 Hz = 5 s
double light_average = 0.0625;

const float light_sensor_max_gained_lux = 500.0;
const float light_sensor_gain = 1.0;  // is applied before capping at light_sensor_max_gained_lux
const float light_poti_gain = 3.0;

bool wifi_initially_connected = false;

bool wifi_switch_changed = false;
bool hour_button_pressed = false;
bool minute_buttom_pressed = false;
bool status_buttom_pressed = false;
bool rtc_interrupt = false;

unsigned long wifi_switch_last_changed = 0;
unsigned long hour_button_last_pressed = 0;
unsigned long minute_button_last_pressed = 0;
unsigned long status_button_last_pressed = 0;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// ESP32Time rtc(3600);  // offset in seconds GMT+1
RTC_DS3231 rtc;

BH1750 light_meter;

uint32_t color = pixels.ColorHSV(led_hue, led_saturation, UINT8_MAX* light_average);
uint32_t last_color = color;

uint8_t led_brightness = 16;

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
void IRAM_ATTR handle_status_button_interrupt() {  //
    unsigned long current_time = millis();

    // debounce button
    if (current_time - status_button_last_pressed > button_debounce_time) {
        status_buttom_pressed = true;
        status_button_last_pressed = current_time;
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
    pinMode(STATUS_BUTTON_PIN, INPUT_PULLDOWN);
    pinMode(LIGHT_POTI_PIN, INPUT);

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
        if (wifi_ssid[0] == '\0') {  // if WiFi SSID is empty
            Serial.println("no WiFi SSID set");
        } else {
            configure_wifi();

            update_time();
        }
    } else {
        Serial.println("WiFi disabled by switch");
    }

    attachInterrupt(WIFI_SWITCH_PIN, &handle_wifi_switch_interrupt, CHANGE);
    attachInterrupt(HOUR_BUTTON_PIN, &handle_hour_button_interrupt, RISING);
    attachInterrupt(MINUTE_BUTTON_PIN, &handle_minute_button_interrupt, RISING);
    attachInterrupt(STATUS_BUTTON_PIN, &handle_status_button_interrupt, RISING);

    rtc.writeSqwPinMode(DS3231_SquareWave1Hz);
    attachInterrupt(RTC_INTERRUPT_PIN, &handle_rtc_interrupt, FALLING);

    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // set RTC time to compile time
}

void loop() {
    if (rtc_interrupt) {
        rtc_interrupt = false;
        display_time();
    }
    if (light_refresh_counter == 0) {
        light_refresh_counter = light_refresh_counter_init;

        adjust_brightness();
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
    if (status_buttom_pressed) {
        status_buttom_pressed = false;
        status_counter = status_counter_init;

        show_status = true;
        display_time();
    }

    if (status_counter > 0) {
        status_counter--;
    } else {  // status == 0
        show_status = false;
    }

    light_refresh_counter--;

    delayMicroseconds(250);
}

void process_wifi_switch_change() {
    if (digitalRead(WIFI_SWITCH_PIN)) {
        if (wifi_ssid[0] == '\0') {  // if WiFi SSID is empty
            Serial.println("no WiFi SSID set");

            return;
        }

        if (!WiFi.isConnected()) {
            configure_wifi();
        }

        update_time();
    } else {
        WiFi.disconnect();
        Serial.println("WiFi disabled");
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

    display_time();
}

void process_minute_button_press() {
    DateTime current_time = rtc.now();

    // increment minute and reset seconds
    DateTime new_time =
        DateTime(current_time.year(), current_time.month(), current_time.day(), current_time.hour(), (current_time.minute() + 1) % 60, 0);

    rtc.adjust(new_time);

    Serial.print("new time: ");
    print_time(new_time);

    display_time();
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

    display_digit(hour_0, color, 0, true);
    display_digit(hour_1, color, 4, false);
    display_digit(minute_0, color, 8, true);
    display_digit(minute_1, color, 12, false);
    display_digit(second_0, color, 16, true);
    display_digit(second_1, color, 20, false);

    if (show_status) {
        display_status(0);
    }

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

void adjust_brightness() {
    float light_measurement = measure_light() / light_sensor_max_gained_lux;
    double light_poti = analogRead(LIGHT_POTI_PIN) / 8192.0;
    light_poti = 1 - light_poti;
    light_poti *= light_poti_gain;

    double current_light = light_measurement * light_poti;

    if (current_light > 1.0) current_light = 1.0;

    // calculate rolling average
    light_average -= light_average / light_average_size;
    light_average += (current_light) / light_average_size;

    uint8_t bright = 255 * light_average;
    bright = max((uint8_t)1, bright);

    last_color = color;
    color = pixels.ColorHSV(led_hue, led_saturation, bright);
    led_brightness = bright;

    if (last_color != color) {
        display_time();
    }
}

void display_status(uint8_t offset) {
    uint32_t status_0_color;

    if (WiFi.status() == WL_CONNECTED) {
        status_0_color = pixels.ColorHSV(130 * hue_degrees_factor, UINT8_MAX, led_brightness);
    } else {
        status_0_color = pixels.ColorHSV(0 * hue_degrees_factor, UINT8_MAX, led_brightness);
    }

    pixels.setPixelColor(offset, status_0_color);
}

float measure_light() {
    float lux = light_meter.readLightLevel();
    return min(lux * light_sensor_gain, light_sensor_max_gained_lux);
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
        Serial.println("[WiFi] Failed to connect to WiFi!");
        // Use disconnect function to force stop trying to connect
        WiFi.disconnect();
    }
}
