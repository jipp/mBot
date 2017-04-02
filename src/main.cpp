// todo:
// autonomous drive

#define VERSION "mBot Hackaton"
#define DISTANCECMCRITICAL 20
#define PUBLSIHINTERVAL 1000
#define WAITINTERVAL 5000
#define MOVESPEEDMAX 230

#include <Arduino.h>

#include <Streaming.h>
#include <MeMCore.h>

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
uint8_t readSensors = 0;
uint8_t randNumber;
bool start = false;
bool moveBot = false;
bool makeNoise = false;

unsigned long publishTimer = millis();
bool wait = false;
unsigned long waitTimer = millis();
bool tryFollowLine = true;
unsigned long tryFollowLineTimer = millis();


void Forward() {
  moveBot = true;
  motorL.run(-moveSpeed);
  motorR.run(moveSpeed);
}

void Backward() {
  moveBot = true;
  motorL.run(moveSpeed);
  motorR.run(-moveSpeed);
}

void TurnLeft() {
  moveBot = true;
  motorL.run(-moveSpeed / 10);
  motorR.run(moveSpeed);
}

void TurnRight() {
  moveBot = true;
  motorL.run(-moveSpeed);
  motorR.run(moveSpeed / 10);
}

void Stop() {
  moveBot = false;
  motorL.run(0);
  motorR.run(0);
}

void Autonomous() {
    randomSeed(analogRead(6));
    randNumber = random(2);
    if (distanceCm > 15 || distanceCm == 0) {
      Forward();
    } else if (distanceCm > 10) {
      switch (randNumber)
      {
        case 0:
          TurnLeft();
          delay(200);
          break;
        case 1:
          TurnRight();
          delay(200);
          break;
      }
    } else {
      Backward();
      delay(400);
    }
    delay(100);
}

void bottomPressed() {
    start = !start;
    if (start) {
      rgbLed.setColor(1, 0, 10, 0);
      rgbLed.show();
    } else {
      rgbLed.setColor(1, 0, 0, 0);
      rgbLed.show();
      Stop();
    }
    delay(250);
}

void bottomCheck() {
  if (analogRead(A7) == 0) {
    bottomPressed();
  }
}

void irControl() {
  uint32_t value;

  if (ir.decode()) {
    value = ir.value;
    switch (value >> 16 & 0xff) {
      case IR_BUTTON_LEFT:
        TurnLeft();
        break;
      case IR_BUTTON_RIGHT:
        TurnRight();
        break;
      case IR_BUTTON_DOWN:
        Backward();
        break;
      case IR_BUTTON_UP:
        Forward();
        break;
      case IR_BUTTON_SETTING:
        Stop();
        break;
      case IR_BUTTON_A:
        bottomPressed();
      break;
    }
  }
}

void noise() {
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

void readData() {
  readSensors = lineFollower.readSensors();
  distanceCm = ultrasonicSensor.distanceCm();
}

void sendData() {
  if (millis() - publishTimer > PUBLSIHINTERVAL) {
    publishTimer = millis();
    Serial << endl << "start: " << start << endl;
    Serial << "moveBot: " << moveBot << endl;
    Serial << "tryFollowLine: " << tryFollowLine << endl;
    Serial << "makeNoise: " << makeNoise << endl;
    Serial << "readSensors: " << readSensors << endl;
    Serial << "temperature: " << temperature.temperature() << endl;
    Serial << "lightSensor: " << lightSensor.read() << endl;
    Serial << "distanceCm: " << distanceCm << endl;
  }
}


void setup() {
  Serial.begin(115200);
  Serial << endl << VERSION << endl << endl;
  pinMode(A7, INPUT);
  ir.begin();
  rgbLed.setColor(0, 0, 0, 0);
  rgbLed.show();
}

void loop() {
  bottomCheck();
  irControl();
  readData();
  sendData();
  noise();
  if (start) {
    if (distanceCm < DISTANCECMCRITICAL and distanceCm > 0) {
      Stop();
      tryFollowLine = true;
    } else {
      moveBot = true;
      if (moveSpeed > MOVESPEEDMAX) {
        moveSpeed = 230;
      }
      switch (readSensors) {
        case S1_IN_S2_IN:
          tryFollowLine = true;
          Forward();
          lineFollowFlag = 10;
          break;
        case S1_IN_S2_OUT:
          tryFollowLine = true;
          Forward();
          if (lineFollowFlag > 1) lineFollowFlag--;
          break;
        case S1_OUT_S2_IN:
          tryFollowLine = true;
          Forward();
          if (lineFollowFlag < 20) lineFollowFlag++;
          break;
        case S1_OUT_S2_OUT:
          if (tryFollowLine) {
            tryFollowLine = !tryFollowLine;
            tryFollowLineTimer = millis();
          }
          if (millis() - tryFollowLineTimer < 3000) {
            if (lineFollowFlag == 10) Backward();
            if (lineFollowFlag < 10) TurnLeft();
            if (lineFollowFlag > 10) TurnRight();
          } else {
            Stop();
//            Autonomous();
          }
          break;
      }
    }
  }
}
