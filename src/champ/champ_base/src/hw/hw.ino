#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pca9685 = Adafruit_PWMServoDriver();

// PWM config cho servo (tùy loại servo có thể chỉnh lại)
#define SERVOMIN  120   // xung min (~0 độ)
#define SERVOMAX  600   // xung max (~180 độ)

String inputString = "";
bool stringComplete = false;

void setup() {
  Serial.begin(115200);

  // init PCA9685
  pca9685.begin();
  pca9685.setPWMFreq(50);  // servo ~50Hz

  inputString.reserve(32);

  Serial.println("Arduino ready");
}

void loop() {
  if (stringComplete) {
    inputString.trim(); // remove potential \r or whitespace
    int commaIdx = inputString.indexOf(',');
    
    if (commaIdx > 0) {
      float val1 = inputString.substring(0, commaIdx).toFloat();
      float val2 = inputString.substring(commaIdx + 1).toFloat();
      
      int angle1 = constrain((int)val1, 0, 180);
      int angle2 = constrain((int)val2, 0, 180);
      
      int pulse1 = map(angle1, 0, 180, SERVOMIN, SERVOMAX);
      int pulse2 = map(angle2, 0, 180, SERVOMIN, SERVOMAX);
      
      pca9685.setPWM(0, 0, pulse1);
      pca9685.setPWM(1, 0, pulse2);
    }

    // reset buffer
    inputString = "";
    stringComplete = false;
  }
}

// đọc serial từng ký tự
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