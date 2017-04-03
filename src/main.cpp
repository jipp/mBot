#define DISTANCECMCRITICAL 20
#define DISTANCECMWARNING 30
#define PUBLSIHINTERVAL 1000
#define TRYFOLLOWLINEINTERVAL 3000
#define WAITINTERVAL 5000
#define MOVESPEED 100

#include <Arduino.h>

#include <Streaming.h>
#include <ArduinoJson.h>
#include <MeMCore.h>

enum direction { FORWARD, BACKWARD, LEFT, RIGHT, STOP } moveDirection = STOP;

MeDCMotor motorL(M1);
MeDCMotor motorR(M2);
MeLineFollower lineFollower(PORT_2);
MeTemperature temperature(PORT_1, SLOT2);
MeUltrasonicSensor ultrasonicSensor(PORT_3);
MeLightSensor lightSensor(PORT_6);
MeIR ir;
MeRGBLed rgbLed(PORT_7, 2);
MeBuzzer buzzer;

double distanceCm = 0.0;
int moveSpeed = 100;
int lineFollowFlag = 0;
byte readSensors = 0;
bool start = false;
bool moveBot = false;
bool makeNoise = false;
bool obstacle = false;

unsigned long publishTimer = millis();
bool wait = false;
unsigned long waitTimer = millis();
bool tryFollowLine = true;
unsigned long tryFollowLineTimer = millis();

DynamicJsonBuffer jsonBuffer;
JsonObject& root = jsonBuffer.createObject();

void forward() {
  moveBot = true;
  motorL.run(-MOVESPEED);
  motorR.run(MOVESPEED);
}

void backward() {
  moveBot = true;
  motorL.run(MOVESPEED);
  motorR.run(-MOVESPEED);
}

void turnLeft() {
  moveBot = true;
  motorL.run(-MOVESPEED / 10);
  motorR.run(MOVESPEED);
}

void turnRight() {
  moveBot = true;
  motorL.run(-MOVESPEED);
  motorR.run(MOVESPEED / 10);
}

void stop() {
  moveBot = false;
  motorL.run(0);
  motorR.run(0);
}

bool distanceCheck(double distanceCmLimit) {
  if (distanceCm < distanceCmLimit) {
    return false;
  } else {
    return true;
  }
}

void move() {
  if (distanceCheck(DISTANCECMCRITICAL)) {
    switch (moveDirection) {
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
  } else {
    switch (moveDirection) {
      case BACKWARD:
      backward();
      break;
      default:
      moveDirection = STOP;
      stop();
      break;
    }
  }
}

void toggleStart() {
  start = !start;
  if (start) {
    rgbLed.setColor(1, 0, 10, 0);
    rgbLed.show();
  } else {
    rgbLed.setColor(1, 0, 0, 0);
    rgbLed.show();
    moveDirection = STOP;
  }
  delay(250);
}

void bottomCheck() {
  if (analogRead(A7) == 0) {
    toggleStart();
  }
}

void irCheck() {
  unsigned long value;

  if (ir.decode()) {
    value = ir.value;
    switch (value >> 16 & 0xff) {
      case IR_BUTTON_LEFT:
      moveDirection = LEFT;
      break;
      case IR_BUTTON_RIGHT:
      moveDirection = RIGHT;
      break;
      case IR_BUTTON_DOWN:
      moveDirection = BACKWARD;
      break;
      case IR_BUTTON_UP:
      moveDirection = FORWARD;
      break;
      case IR_BUTTON_SETTING:
      moveDirection = STOP;
      break;
      case IR_BUTTON_A:
      toggleStart();
      break;
    }
  }
}

void noiseCheck() {
  if (!moveBot and start) {
    if (wait == false) {
      wait = true;
      waitTimer = millis();
    }
    if (wait and millis() - waitTimer > WAITINTERVAL) {
      makeNoise = true;
      //buzzer.tone(262, 100);
      rgbLed.setColor(2, 10, 0, 0);
      rgbLed.show();
    } else {
      rgbLed.setColor(2, 0, 0, 10);
      rgbLed.show();
    }
  } else {
    wait = false;
    makeNoise = false;
    buzzer.noTone();
    rgbLed.setColor(2, 0, 0, 0);
    rgbLed.show();
  }
}

void autonomous() {
  byte randNumber;

  randomSeed(analogRead(6));
  randNumber = random(2);
  if (distanceCheck(DISTANCECMCRITICAL) and !obstacle) {
    moveDirection = FORWARD;
  } else {
    obstacle = true;
  }

  if (obstacle) {
    if (!distanceCheck(DISTANCECMWARNING)) {
      moveDirection = BACKWARD;
    } else {
      switch (randNumber) {
        case 0:
        moveDirection = LEFT;
        turnLeft();
        delay(200);
        break;
        case 1:
        moveDirection = RIGHT;
        turnRight();
        delay(200);
        break;
      }
      obstacle = false;
    }
  }
}

void readData() {
  readSensors = lineFollower.readSensors();
  distanceCm = ultrasonicSensor.distanceCm();
}

void sendData() {
  if (millis() - publishTimer > PUBLSIHINTERVAL) {
    publishTimer = millis();
    root["moveBot"] = moveBot;
    root["makeNoise"] = makeNoise;
    root["distanceCm"] = distanceCm;
    root["temperature"] = temperature.temperature();
    root["lightSensor"] = lightSensor.read();
    Serial << endl;
    root.prettyPrintTo(Serial);
  }
}


void setup() {
  Serial.begin(115200);
  Serial << endl << endl;
  pinMode(A7, INPUT);
  ir.begin();
  rgbLed.setColor(0, 0, 0, 0);
  rgbLed.show();
}

void loop() {
  bottomCheck();
  irCheck();
  readData();
  sendData();
  noiseCheck();
  move();
  if (start) {
    switch (readSensors) {
      case S1_IN_S2_IN:
      tryFollowLine = true;
      moveDirection = FORWARD;
      lineFollowFlag = 10;
      break;
      case S1_IN_S2_OUT:
      tryFollowLine = true;
      moveDirection = FORWARD;
      if (lineFollowFlag > 1) lineFollowFlag--;
      break;
      case S1_OUT_S2_IN:
      tryFollowLine = true;
      moveDirection = FORWARD;
      if (lineFollowFlag < 20) lineFollowFlag++;
      break;
      case S1_OUT_S2_OUT:
      if (tryFollowLine) {
        tryFollowLine = !tryFollowLine;
        tryFollowLineTimer = millis();
      }
      if (millis() - tryFollowLineTimer < TRYFOLLOWLINEINTERVAL) {
        if (lineFollowFlag == 10) moveDirection = BACKWARD;
        if (lineFollowFlag < 10) moveDirection = LEFT;
        if (lineFollowFlag > 10) moveDirection = RIGHT;
      } else {
        autonomous();
      }
      break;
    }
  }
}
