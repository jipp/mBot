#define DISTANCECMCRITICAL 20
#define DISTANCECMWARNING 30
#define PUBLSIHINTERVAL 2000
#define TRYFOLLOWLINEINTERVAL 3000
#define WAITINTERVAL 5000
#define MOVESPEED 100

#include "Arduino.h"

#include <Streaming.h>
#include <ArduinoJson.h>
#include <MeMCore.h>

enum direction { STOP, FORWARD, BACKWARD, LEFT, RIGHT } move = STOP;

MeDCMotor motorL(M1);
MeDCMotor motorR(M2);
MeLineFollower lineFollower(PORT_2);
MeTemperature temperature(PORT_4, SLOT2);
MeUltrasonicSensor ultrasonicSensor(PORT_3);
MeLightSensor lightSensor(PORT_6);
MeIR ir;
MeRGBLed rgbLed(PORT_7, 2);
MeBuzzer buzzer;
MePIRMotionSensor pirMotionSensor(PORT_1);

int lineFollowFlag = 0;
bool watch = false;
bool blocked = false;
bool obstacleDetected = false;

unsigned long publishTimer = millis();
bool wait = false;
unsigned long waitTimer = millis();
bool tryFollowLine = true;
unsigned long tryFollowLineTimer = millis();

DynamicJsonBuffer jsonBuffer;
JsonObject& root = jsonBuffer.createObject();

void forward() {
  motorL.run(-MOVESPEED);
  motorR.run(MOVESPEED);
}

void backward() {
  motorL.run(MOVESPEED);
  motorR.run(-MOVESPEED);
}

void turnLeft() {
  motorL.run(-MOVESPEED / 10);
  motorR.run(MOVESPEED);
}

void turnRight() {
  motorL.run(-MOVESPEED);
  motorR.run(MOVESPEED / 10);
}

void stop() {
  motorL.run(0);
  motorR.run(0);
}

bool distanceWarning(double distanceCmLimit) {
  if (ultrasonicSensor.distanceCm() < distanceCmLimit) {
    return true;
  } else {
    return false;
  }
}

void drive() {
  if (distanceWarning(DISTANCECMCRITICAL)) {
    if (move == BACKWARD) {
      backward();
    } else {
      stop();
    }
  } else {
    switch (move) {
      case FORWARD:
      forward();
      break;
      case BACKWARD:
      backward();
      break;
      case LEFT:
      turnLeft();
      break;
      case RIGHT:
      turnRight();
      break;
      case STOP:
      stop();
      break;
    }
  }
}

void toggleWatch() {
  watch = !watch;
  if (watch) {
    rgbLed.setColor(1, 0, 10, 0);
    rgbLed.show();
  } else {
    rgbLed.setColor(1, 0, 0, 0);
    rgbLed.show();
    move = STOP;
  }
  delay(250);
}

void bottomCheck() {
  if (analogRead(A7) == 0) {
    toggleWatch();
  }
}

void irCheck() {
  unsigned long value;

  if (ir.decode()) {
    value = ir.value;
    switch (value >> 16 & 0xff) {
      case IR_BUTTON_LEFT:
      move = LEFT;
      break;
      case IR_BUTTON_RIGHT:
      move = RIGHT;
      break;
      case IR_BUTTON_DOWN:
      move = BACKWARD;
      break;
      case IR_BUTTON_UP:
      move = FORWARD;
      break;
      case IR_BUTTON_SETTING:
      move = STOP;
      break;
      case IR_BUTTON_A:
      toggleWatch();
      break;
    }
  }
}

void silent() {
  buzzer.noTone();
  rgbLed.setColor(2, 0, 0, 0);
  rgbLed.show();
}

void warning() {
  rgbLed.setColor(2, 0, 0, 10);
  rgbLed.show();
}

void alarm() {
  buzzer.tone(262, 100);
  rgbLed.setColor(2, 10, 0, 0);
  rgbLed.show();
}

void noiseCheck() {
  if (watch and distanceWarning(DISTANCECMCRITICAL)) {
    if (wait == false) {
      wait = true;
      waitTimer = millis();
    }
    if (wait and millis() - waitTimer > WAITINTERVAL) {
      blocked = true;
      alarm();
    } else {
      warning();
    }
  } else {
    wait = false;
    blocked = false;
    silent();
  }
}

void autonomous() {
  byte randNumber;

  randomSeed(analogRead(6));
  randNumber = random(2);
  if (!distanceWarning(DISTANCECMCRITICAL) and !obstacleDetected) {
    move = FORWARD;
  } else {
    obstacleDetected = true;
  }
  if (obstacleDetected) {
    if (distanceWarning(DISTANCECMWARNING)) {
      move = BACKWARD;
    } else {
      switch (randNumber) {
        case 0:
        move = LEFT;
        turnLeft();
        delay(500);
        break;
        case 1:
        move = RIGHT;
        turnRight();
        delay(500);
        break;
      }
      obstacleDetected = false;
    }
  }
}

void sendData() {
  if (millis() - publishTimer > PUBLSIHINTERVAL) {
    publishTimer = millis();
    root["watch"] = watch;
    root["move"] = move;
    root["wait"] = wait;
    root["blocked"] = blocked;
    root["obstacleDetected"] = obstacleDetected;
    root["distanceCm"] = ultrasonicSensor.distanceCm();
    root["lightSensor"] = lightSensor.read();
    root["temperature"] = temperature.temperature();
    root["isHumanDetected"] = pirMotionSensor.isHumanDetected();
    //root.prettyPrintTo(Serial);
    root.printTo(Serial);
    Serial << "<eom>" << endl;
  }
}


void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial << endl << endl;
  pinMode(A7, INPUT);
  ir.begin();
  pirMotionSensor.SetPirMotionMode(1);
  rgbLed.setColor(0, 0, 0, 0);
  rgbLed.show();
}

void loop() {
  bottomCheck();
  irCheck();
  noiseCheck();
  sendData();
  drive();
  if (watch) {
    switch (lineFollower.readSensors()) {
      case S1_IN_S2_IN:
      tryFollowLine = true;
      move = FORWARD;
      lineFollowFlag = 10;
      break;
      case S1_IN_S2_OUT:
      tryFollowLine = true;
      move = FORWARD;
      if (lineFollowFlag > 1) lineFollowFlag--;
      break;
      case S1_OUT_S2_IN:
      tryFollowLine = true;
      move = FORWARD;
      if (lineFollowFlag < 20) lineFollowFlag++;
      break;
      case S1_OUT_S2_OUT:
      if (tryFollowLine) {
        tryFollowLine = !tryFollowLine;
        tryFollowLineTimer = millis();
      }
      if (millis() - tryFollowLineTimer < TRYFOLLOWLINEINTERVAL) {
        if (lineFollowFlag == 10) move = BACKWARD;
        if (lineFollowFlag < 10) move = LEFT;
        if (lineFollowFlag > 10) move = RIGHT;
      } else {
        autonomous();
      }
      break;
    }
  }
}
