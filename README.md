[![Build Status](https://travis-ci.org/jipp/mBot.svg?branch=master)](https://travis-ci.org/jipp/mBot)

# mBot
Firmware for the mBot from Makeblock written with PlatformIO.

## IDE
* PlatformIO IDE: used for development
* Arduino IDE: copy content of main.cpp into Arduino IDE

## The following features are implemented
* To switch between the modes either press the button on the mBot or the 'A' on the IR remote control
* manual mode: Control the mBot with the IR remote control. In case an obstacle is in the way, the only possible way is backward.
* autonomous mode: Follow the black line/path. In case the path is lost it will try to find it again. In general obstacles while following the path stop the mBot and will generate a signal when hindering the mBot too long. While searching for the path, it tries to avoid obstacles and moves backwards and will then either turn left or right.
* Data from sensors and also behavior data will be send to the Serial interfaces (USB, Bluetooth, WiFi) using a JsonString terminated with '\<eom\>'.

## Sensor data
* watch:
  * true: autonomous mode is on
  * false: manual mode is on (use IR to control, beware that all mBot use the same IR and influence each other)
* move:
  * true: is moving
  * false: is not moving
* wait:
  * true: obstacle detected and wait 5 sec before being blocked, blue light switches to on
  * false: no obstacle detected
* blocked (for autonomous mode):
  * true: obstacle did not go away -> be noisy, red light is on
  * false: no obstacle detected
* obstacleDetected (for manual mode):
  * true: be in front of an obstacle, only move possible is backwards
  * false: no obstacle
* distanceCm: distance measured with ultrasonic sensor
* lightSensor: build in sensor (LDR)
* temperature: measured with DS18B20 (water proved)
* isHumanDetected: PIR Sensor (not working and not completely integrated)

## Needed Libraries
* [  64 ] ArduinoJson
* [ 560 ] Streaming
