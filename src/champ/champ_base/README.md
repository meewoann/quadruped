# Real Robot Sensor Integration

This document describes the changes made to support real hardware sensors (IMU + LiDAR) and seamless switching between Gazebo simulation and the physical robot.

---

## What Changed

### 1. `src/hardware/hardware.cpp/hardware.cpp.ino` — Arduino Firmware

**Added:** MPU6050 IMU support over I2C.

The Arduino now does two things concurrently:
- **Receives** servo angle commands from ROS2 (existing behavior, unchanged)
- **Sends** IMU data to ROS2 every 20 ms (50 Hz)

**IMU packet format** (sent over serial):
```
IMU,qw,qx,qy,qz,ax,ay,az,gx,gy,gz\n
```

| Field | Description | Unit |
|-------|-------------|------|
| qw,qx,qy,qz | Orientation quaternion (identity for now) | — |
| ax,ay,az | Linear acceleration | m/s² |
| gx,gy,gz | Angular velocity | rad/s |

> **Note:** The quaternion is sent as identity `(1,0,0,0)` because the MPU6050 requires a DMP or fusion filter to produce accurate orientation. The ROS2 EKF (`robot_localization`) estimates orientation from the raw accel/gyro data instead. Upgrading to DMP-based quaternion output is straightforward if needed.

**Required Arduino libraries:**
- `Adafruit MPU6050` (install via Arduino Library Manager)
- `Adafruit Unified Sensor`
- `Adafruit PWM Servo Driver Library` (existing)

---

### 2. `src/servo_serial_converter_node.cpp` — ROS2 Serial Bridge Node

**Added:** Bidirectional serial communication — the node now also reads incoming data from Arduino.

A background thread continuously reads lines from `/dev/ttyACM0`. When an `IMU,...` line is received, it is parsed and published as `champ_msgs/Imu` on the `imu/raw` topic.

**Topics:**

| Topic | Type | Direction |
|-------|------|-----------|
| `joints_debug` | `champ_msgs/Joints` | Subscribed (servo commands from gait controller) |
| `imu/raw` | `champ_msgs/Imu` | Published (IMU data from Arduino) |

The `imu/raw` data is then picked up by `message_relay_node`, which applies EMA filtering and republishes as standard `sensor_msgs/Imu` on `imu/data` for the EKF and state estimation nodes.

---

### 3. `launch/include/laser/rplidar_ros2.launch.py` *(new file)*

A ROS2-compatible launch file for the RPLIDAR A1M8.

The existing `rplidar.launch` in this directory is ROS1 XML format and cannot be used in ROS2. This new file replaces it.

**Default parameters:**
- Serial port: `/dev/ttyUSB0`
- Baud rate: 115200
- Output topic: `/scan` (`sensor_msgs/LaserScan`)
- Frame ID: `laser`

To use a different port:
```bash
ros2 launch champ_config bringup.launch.py sim:=false serial_port:=/dev/ttyUSB1
```

**Required package:**
```bash
sudo apt install ros-humble-rplidar-ros
```

---

### 4. `champ_config/launch/bringup.launch.py` — Sim/Real Launch Switch

**Added:** The `sim` argument now gates which nodes are launched.

| Argument | Value | Effect |
|----------|-------|--------|
| `sim` | `true` | Gazebo simulation (existing behavior) |
| `sim` | `false` | Real robot: serial bridge + IMU + LiDAR |

When `sim:=false`, three additional nodes start automatically:
1. `servo_pwm_converter_node` — serial bridge (servo commands out, IMU data in)
2. `message_relay_node` — converts `imu/raw` → `imu/data` with EMA filter
3. `rplidar_node` — publishes `/scan` from the A1M8

`orientation_from_imu` is set to `true` in both modes so the state estimator uses IMU data for orientation.

---

## Full Data Flow (Real Robot Mode)

```
Arduino (MPU6050 via I2C)
    │  serial /dev/ttyACM0
    ▼
servo_pwm_converter_node
    │  imu/raw  (champ_msgs/Imu)
    ▼
message_relay_node
    │  imu/data  (sensor_msgs/Imu)
    ▼
base_to_footprint_ekf ──► odom/local
footprint_to_odom_ekf ──► odom (TF: odom → base_footprint)

RPLIDAR A1M8
    │  USB /dev/ttyUSB0
    ▼
rplidar_node
    │  /scan  (sensor_msgs/LaserScan)
    ▼
slam_toolbox / AMCL (navigation)
```

---

## Usage

**Real robot:**
```bash
ros2 launch champ_config bringup.launch.py sim:=false
```

**Simulation (Gazebo, unchanged):**
```bash
ros2 launch champ_config bringup.launch.py sim:=true
# or use the dedicated Gazebo launch:
ros2 launch champ_config gazebo.launch.py
```

**Verify sensors:**
```bash
# IMU raw from Arduino
ros2 topic echo /imu/raw

# IMU filtered (sensor_msgs/Imu for EKF)
ros2 topic echo /imu/data

# LiDAR scan
ros2 topic echo /scan --no-arr

# Visualize in RViz
ros2 launch champ_config bringup.launch.py sim:=false rviz:=true
```

---

## Hardware Wiring

| Device | Connection | Default port |
|--------|-----------|-------------|
| Arduino (servo + IMU) | USB | `/dev/ttyACM0` |
| RPLIDAR A1M8 | USB | `/dev/ttyUSB0` |
| MPU6050 | I2C to Arduino (SDA/SCL) | I2C address `0x68` |
| PCA9685 | I2C to Arduino (SDA/SCL) | I2C address `0x40` |
