# flashing

see <https://www.wemos.cc/en/latest/s2/s2_mini.html>

## Arduino

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
