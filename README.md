# ESP32 RGB Controller
ESP32-based IoT RGB Strip Controller. Allows control of a RGB LED strip via a web interface.

The ESP32 code is built on the Arduino framework. 

## Features
- custom RGB color
- color presets
- motion detection via PIR sensor
- automatic brightness adjustment based on ambient light sensing
- automatic power off when room is dark
- react to sound

## Software Dependencies
- ESPAsyncWebServer
- ESP32 Wi-Fi library
- SPIFFS
- I2S driver

## Hardware Components
- ESP32 devboard
- 12V RGB strip
- PIR sensor module
- photoresistor
- INMP441 I2S microphone
- 3 x TIP41C NPN bipolar junction transistors
- 12V power supply for the LED strip, 5V USB power supply for the devboard

**Note:** Please replace the corresponding placeholder strings with your 2.4GHz Wi-Fi SSID and password.
