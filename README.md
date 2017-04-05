[![Build Status](https://travis-ci.org/jipp/mBot.svg?branch=master)](https://travis-ci.org/jipp/mBot)

# mBot
Firmware for the mBot from Makeblock written with PlatformIO.

## IDE
* PlatformIO IDE: used for development
* Arduino IDE: copy content of main.cpp into Arduino IDE

## The following features are implemented
* To switch between the modes either press the buttom on the mBot or the 'A' on the IR remote control
* manual mode: Control the mBot with the IR remote control. In case an obstacle is in the way, the only possible way is backward.
* autonomous mode: Follow the black line/path. In case the path is lost it will try to find it again. In general obstacles while following the path stop the mBot and will generate a signal when hindering the mBot too long. While searching for the path, it tries to avoid obstacles and moves backwards and will then either turn left or right.
* Data from sensors and also behaviour data will be send to the Serial interfaces (USB, Bluetooth, WiFi) using a Json string terminated with '\<eom\>\'.

## Needed Libraries
* [  64 ] ArduinoJson
* [ 560 ] Streaming
