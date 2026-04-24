#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pca9685 = Adafruit_PWMServoDriver();

#define SERVOMIN  120
#define SERVOMAX  430

int angleToPulse(int angle) {
  return map(angle, 0, 180, SERVOMIN, SERVOMAX);
}

void setup() {
  Serial.begin(115200);

  // init PCA9685
  pca9685.begin();
  pca9685.setPWMFreq(50);  // 50Hz cho servo

  delay(500);

  int pulse90 = angleToPulse(90);

  // set toàn bộ 16 channel về 90°
  for (int i = 0; i < 16; i++) {
    pca9685.setPWM(i, 0, pulse90);
  }

  Serial.println("All servos set to 90 degrees");
}

void loop() {
  // không cần làm gì
}