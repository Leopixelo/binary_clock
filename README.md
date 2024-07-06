# interface

![interface](<binary clock interface.png>)

## status

By pressing the status button the first to LEDs of the first hour digit display status information.

### first LED

| color | status                 |
| ----- | ---------------------- |
| red   | no WiFi connection     |
| green | active WiFi connection |

## WiFi

Enable or disable WiFi. When enabled, retrieve current time via NTP.

## h

Advance time by one hour.

## m

Advance time by one minute and set seconds to 0.

# set up

Before flashing to the micro-controller, you have to:

- make a copy of `binary_clock/config.template.cpp` in the same directory
- rename the copy to `config.cpp`
- optionally set your WiFi SSID and password to enable retrieving the current time via NTP

# flashing

see <https://www.wemos.cc/en/latest/s2/s2_mini.html>

## Arduino

- install libraries
    - `Adafruit NeoPixel` by Adafruit (Version 1.12.2)
    - `RTClib` by Adafruit (Version 2.1.4)
    - `BH1750` by Christopher Laws (Version 1.3.0)
- install `esp32` board package by Espressif Systems in Board Manager (<https://github.com/espressif/arduino-esp32>)
- set board to `LOLIN S2 Mini (esp32)`
    - other settings on defaults should work
- make sure your user has sufficient permissions to write to the USB device (e.g. user is in `dialout` group)
- put esp32 into download mode
    - when already connected / powered: press `RST` button while holding `0` button
    - when not connected / powered: hold `0` button while connecting / powering up
- upload sketch

# useful resources

- <https://www.theelectronics.co.in/2022/04/how-to-use-internal-rtc-of-esp32.html>
- <https://www.wemos.cc/en/latest/s2/s2_mini.html>
