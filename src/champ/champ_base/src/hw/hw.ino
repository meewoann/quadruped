#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

Adafruit_PWMServoDriver pca9685 = Adafruit_PWMServoDriver();
Adafruit_MPU6050 mpu;

// PWM config cho servo (tùy loại servo có thể chỉnh lại)
#define SERVOMIN  120   // xung min (~0 độ)
#define SERVOMAX  600   // xung max (~180 độ)

String inputString = "";
bool stringComplete = false;

unsigned long lastImuTime = 0;
const unsigned long imuInterval = 10; // 100 Hz

void setup() {
  Serial.begin(115200);

  // init PCA9685
  pca9685.begin();
  pca9685.setPWMFreq(50);  // servo ~50Hz

  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
  } else {
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_44_HZ); // Enable DLPF
  }

  inputString.reserve(256);

  Serial.println("Arduino ready");
}

void loop() {
  if (stringComplete) {
    inputString.trim(); // remove potential \r or whitespace
    
    // Process if string is not empty and is not the IMU response itself (to prevent echoing parsing issues just in case)
    if (inputString.length() > 0 && inputString.indexOf("IMU") == -1) {
      int startIdx = 0;
      int expectedJoints = 12;
      for (int i = 0; i < expectedJoints; i++) {
        int commaIdx = inputString.indexOf(',', startIdx);
        String valStr = "";
        
        if (commaIdx == -1) {
          valStr = inputString.substring(startIdx);
        } else {
          valStr = inputString.substring(startIdx, commaIdx);
          startIdx = commaIdx + 1;
        }
        
        if (valStr.length() > 0) {
          float val = valStr.toFloat();
          int angle = constrain((int)val, 0, 180);
          int pulse = map(angle, 0, 180, SERVOMIN, SERVOMAX);
          pca9685.setPWM(i, 0, pulse);
        }
        
        if (commaIdx == -1) break;
      }
    }

    // reset buffer
    inputString = "";
    stringComplete = false;
  }

  // --- Send IMU Data at 100 Hz ---
  if (millis() - lastImuTime >= imuInterval) {
    lastImuTime = millis();
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    float qw = 1.0, qx = 0.0, qy = 0.0, qz = 0.0;
    float ax = a.acceleration.x;
    float ay = a.acceleration.y;
    float az = a.acceleration.z;
    float gx = g.gyro.x;
    float gy = g.gyro.y;
    float gz = g.gyro.z;

    String payload = "IMU," + String(qw) + "," + String(qx) + "," + String(qy) + "," + String(qz) + "," +
                     String(ax) + "," + String(ay) + "," + String(az) + "," +
                     String(gx) + "," + String(gy) + "," + String(gz) + ",";

    int csum = 0;
    for (unsigned int i = 0; i < payload.length(); i++) {
        csum ^= payload[i];
    }
    Serial.print(payload);
    Serial.println(csum);
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