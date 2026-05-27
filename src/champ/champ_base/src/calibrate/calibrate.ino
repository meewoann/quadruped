#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pca9685 = Adafruit_PWMServoDriver();

#define SERVOMIN 120
#define SERVOMAX 430

double JOINT_OFFSETS[12] = {
  85.0, 35.0, 187.0, // LF
  80.0, 55.0, 180.0, // RF
  90.0, -5.0, 190.0, // LH
  95.0, 55.0, 180.0  // RH
};

double position[12] = {
  -4.212025487504434e-06,
  44.84374237060547,
  -88.35194396972656,

  -7.969308626343263e-07,
  44.84374237060547,
  -88.35194396972656,

  -4.212025487504434e-06,
  44.84374237060547,
  -88.35194396972656,

  -7.969308626343263e-07,
  44.84374237060547,
  -88.35194396972656
};

String inputString = "";
bool stringComplete = false;

void setup() {

  Serial.begin(115200);

  pca9685.begin();
  pca9685.setPWMFreq(50);

  inputString.reserve(64);

  delay(1000);

  moveRobot();

  Serial.println("Ready");
  Serial.println("Examples:");
  Serial.println("LF0 5");
  Serial.println("RF2 -10");
  Serial.println("LH1 3");
}

void loop() {

  if (stringComplete) {

    processCommand(inputString);

    inputString = "";
    stringComplete = false;
  }
}

void moveRobot() {

  for (int i = 0; i < 12; i++) {

    float angle = position[i] + JOINT_OFFSETS[i];

    angle = constrain(angle, 0, 180);

    int pulse = map(angle, 0, 180, SERVOMIN, SERVOMAX);

    pca9685.setPWM(i, 0, pulse);
  }
}

void processCommand(String cmd) {

  cmd.trim();

  int spaceIdx = cmd.indexOf(' ');

  if (spaceIdx == -1) {
    Serial.println("Invalid command");
    return;
  }

  // ví dụ: LF0
  String jointName = cmd.substring(0, spaceIdx);

  // ví dụ: +5
  float delta = cmd.substring(spaceIdx + 1).toFloat();

  if (jointName.length() != 3) {
    Serial.println("Invalid joint format");
    return;
  }

  String leg = jointName.substring(0, 2);

  int jointIdx = jointName.substring(2).toInt();

  if (jointIdx < 0 || jointIdx > 2) {
    Serial.println("Joint index must be 0-2");
    return;
  }

  int baseIdx = -1;

  if (leg == "LF") baseIdx = 0;
  else if (leg == "RF") baseIdx = 3;
  else if (leg == "LH") baseIdx = 6;
  else if (leg == "RH") baseIdx = 9;

  if (baseIdx == -1) {
    Serial.println("Unknown leg");
    return;
  }

  int targetIdx = baseIdx + jointIdx;

  JOINT_OFFSETS[targetIdx] += delta;

  moveRobot();

  Serial.println();
  Serial.print("Updated ");
  Serial.print(jointName);
  Serial.print(" by ");
  Serial.println(delta);

  printOffsets();
}

void printOffsets() {

  Serial.println();
  Serial.println("double JOINT_OFFSETS[12] = {");

  for (int i = 0; i < 12; i++) {

    Serial.print("  ");
    Serial.print(JOINT_OFFSETS[i], 1);

    if (i != 11) Serial.print(",");

    // comment tên chân
    if (i == 2) Serial.print(" // LF");
    if (i == 5) Serial.print(" // RF");
    if (i == 8) Serial.print(" // LH");
    if (i == 11) Serial.print(" // RH");

    Serial.println();
  }

  Serial.println("};");
  Serial.println();
}

void serialEvent() {

  while (Serial.available()) {

    char inChar = (char)Serial.read();

    if (inChar == '\n') {

      stringComplete = true;

    } else {

      inputString += inChar;
    }
  }
}
