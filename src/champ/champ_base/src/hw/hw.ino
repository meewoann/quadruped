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

    // Simple additive modulo checksum
    int csum = ((int)(ax + ay + az + gx + gy + gz)) & 0xFF;

    Serial.print("IMU,");
    Serial.print(qw); Serial.print(",");
    Serial.print(qx); Serial.print(",");
    Serial.print(qy); Serial.print(",");
    Serial.print(qz); Serial.print(",");
    Serial.print(ax); Serial.print(",");
    Serial.print(ay); Serial.print(",");
    Serial.print(az); Serial.print(",");
    Serial.print(gx); Serial.print(",");
    Serial.print(gy); Serial.print(",");
    Serial.print(gz); Serial.print(",");
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