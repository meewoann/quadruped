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

// Gyro bias calibration (computed at startup)
float gyroBiasX = 0.0, gyroBiasY = 0.0, gyroBiasZ = 0.0;

// Accelerometer bias calibration (computed at startup)
// After calibration, the expected output when flat & still is (0, 0, 9.81)
float accelBiasX = 0.0, accelBiasY = 0.0, accelBiasZ = 0.0;

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

    // ── Gyro + Accelerometer bias calibration ────────────────────────
    // KEEP THE ROBOT COMPLETELY STILL AND LEVEL during this phase!
    Serial.println("Calibrating IMU bias... DO NOT MOVE!");
    const int CAL_SAMPLES = 500;
    float sumGx = 0.0, sumGy = 0.0, sumGz = 0.0;
    float sumAx = 0.0, sumAy = 0.0, sumAz = 0.0;
    for (int i = 0; i < CAL_SAMPLES; i++) {
      sensors_event_t a_cal, g_cal, t_cal;
      mpu.getEvent(&a_cal, &g_cal, &t_cal);
      sumGx += g_cal.gyro.x;
      sumGy += g_cal.gyro.y;
      sumGz += g_cal.gyro.z;
      sumAx += a_cal.acceleration.x;
      sumAy += a_cal.acceleration.y;
      sumAz += a_cal.acceleration.z;
      delay(2); // ~1 second total (500 * 2ms)
    }
    gyroBiasX = sumGx / CAL_SAMPLES;
    gyroBiasY = sumGy / CAL_SAMPLES;
    gyroBiasZ = sumGz / CAL_SAMPLES;
    // Accel bias: measured_mean - expected_gravity
    // When flat and still, expected reading is (0, 0, +9.81) m/s²
    accelBiasX = (sumAx / CAL_SAMPLES) - 0.0;
    accelBiasY = (sumAy / CAL_SAMPLES) - 0.0;
    accelBiasZ = (sumAz / CAL_SAMPLES) - 9.81;
    Serial.print("Gyro bias: X="); Serial.print(gyroBiasX, 6);
    Serial.print(" Y="); Serial.print(gyroBiasY, 6);
    Serial.print(" Z="); Serial.println(gyroBiasZ, 6);
    Serial.print("Accel bias: X="); Serial.print(accelBiasX, 6);
    Serial.print(" Y="); Serial.print(accelBiasY, 6);
    Serial.print(" Z="); Serial.println(accelBiasZ, 6);
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
    float ax = a.acceleration.x - accelBiasX;
    float ay = a.acceleration.y - accelBiasY;
    float az = a.acceleration.z - accelBiasZ;
    float gx = g.gyro.x - gyroBiasX;
    float gy = g.gyro.y - gyroBiasY;
    float gz = g.gyro.z - gyroBiasZ;

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