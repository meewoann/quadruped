# System Context: Quadruped Robot (Champ Framework)

## Core Environment
* **OS:** Linux
* **ROS Version:** ROS 2 Humble
* **Workspace:** `~/quad_ws`
* **Important CLI Rule:** ALWAYS source before running commands:
  `source /opt/ros/humble/setup.bash && source install/setup.bash`

## Hardware & Connections
* **Master SBC/PC:** Runs `champ` inverse kinematics & state estimation.
* **Arduino (Bridger):** Connected via USB `ttyUSB0` @ 115200 baud.
  * **PCA9685 (Servos):** I2C `0x40`. 50Hz PWM frequency.
  * **MPU6050 (IMU):** I2C `0x68`. Hardware DLPF enabled at 44Hz.
* **LiDAR:** RPLidar A1M8 via USB `ttyUSB1`.

## Real Robot Launch Flow
`ros2 launch champ_config bringup.launch.py sim:=false rviz:=true`

## Real Robot IMU/Control Pipeline
1. **`hw.ino` (Arduino):** 
   - Receives joint targets and commands PCA9685.
   - Reads MPU6050 and streams 100Hz serial message: `IMU,qw,qx,qy,qz,ax,ay,az,gx,gy,gz,csum\n`.
2. **`servo_pwm_converter_node` (`servo_serial_converter_node.cpp`):** 
   - Validates serial checksum, parses floats, publishes `champ_msgs/Imu` to `imu/raw`.
   - Sends joint states from ROS to Arduino.
3. **`message_relay_node` (`message_relay.cpp`):** 
   - Subscribes to `imu/raw`. Applies raw covariances (no EMA phase-lag). 
   - Publishes formatted `sensor_msgs/Imu` to `imu/data_raw`.
4. **`imu_filter_madgwick`:** 
   - Subscribes to `imu/data_raw`. Computes orientation quaternions (external sensor fusion).
   - Publishes complete tracking data to `imu/data`.
5. **EKF (`robot_localization`):** 
   - Subscribes to `imu/data` for accurate `odom` -> `base_footprint` stability.
