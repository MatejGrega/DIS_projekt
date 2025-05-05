# MPC-DIS project

Used hardware:

* ESP32S2-KALUGA-1 V1.3
* ESP-LyraT-8311A V1.3 (standard extension board)
* ESP-LyraP-LCD32 V1.2 (standard extension board)

Used software:
* Visual Studio Code version: (1.99.3)
* ESP-IDF extension for VS Code (version: 1.9.1)
* ESP-IDF v5.4.0

## Project description

This project features simple audio signal generator and analysator. Generator can produce sinusoidal signal in frequency range from 50 Hz to 7500 Hz and 10 volume levels. Audio analysator is just measuring sound level of signal sensed by microphone.  Device is controlled by six push buttons. Information is displayed on the LCD. RGB LED is used to signal sound level represented by color: green = low level, red = high level.

Project uses 5 peripherals:

* LCD
* buttons sensed by ADC
* RGB LED
* microphone (I2S downstream)
* speaker (I2S upstream)