# quadruped-vio

A low-cost 12-DOF quadruped robot with monocular Visual-Inertial Odometry (VIO), built on ROS 2 and simulated in Gazebo. The system combines trot gait control and legged locomotion with VIO for real-time state estimation, enabling odometry-driven SLAM without wheel or leg encoders.

![Quadruped robot in Gazebo](assets/quadrupedrobot.jpg)

## Overview

Traditional legged robot odometry relies on leg kinematics and IMU fusion, which drifts significantly on uneven or slippery terrain. This project replaces that approach with a monocular VIO pipeline that fuses camera and IMU data to produce more robust odometry, which is then fed into the navigation and SLAM stack.

**Hardware (physical platform):**
- 12x DS5180 servos (3-DOF per leg, linkage-based leg mechanism)
- Logitech C270 monocular camera
- MPU-6050 IMU
- Jetson Nano (onboard compute)
- Arduino (low-level servo control)
- PCA9685
**Note:** VIO in this repo (`main` branch) is currently validated in Gazebo simulation. The real hardware deployment — running on the Jetson Nano with the physical robot — lives on the [`jetson`](../../tree/jetson) branch.

## Hardware Platform

The leg uses a linkage-based mechanism (not direct-drive) to transmit servo motion to the foot, improving mechanical advantage and reducing torque load on the hip/knee servos.

| Real Robot | Single-Leg Linkage Motion |
|---|---|
| ![Real robot hardware](assets/hardware.jpg) | ![Leg linkage motion](assets/leg.gif) |

**Full robot walking on real hardware:**

![Robot moving on real hardware](assets/moving.gif)

## Demo

| Gazebo Simulation | RViz Map |
|---|---|
| ![Gazebo map](assets/gazebomap.jpg) | ![RViz map](assets/rvizmap.jpg) |

**VIO running live:**

![VIO demo](assets/output.gif)

## System Architecture

- **Locomotion:** CHAMP-based trot gait control with Catmull–Rom spline foot trajectory generation
- **State Estimation:** VINS-Fusion (monocular + IMU), using FAST feature detection + KLT optical flow front-end and IMU pre-integration back-end
- **Simulation:** Gazebo, with `use_vio_odom` flag to switch odometry source from ground-truth/leg odometry to VIO
- **Mapping:** SLAM via CHAMP's `slam.launch.py`, consuming VIO odometry as the localization input

## Prerequisites

- Ubuntu 22.04
- ROS 2 Humble
- Gazebo (Classic, bundled with `ros-humble-gazebo-ros-pkgs`)
- OpenCV, Ceres Solver (required by VINS-Fusion)

## Installation

```bash
# Create workspace
mkdir -p ~/quad_ws/src
cd ~/quad_ws/src

# Clone this repository
git clone https://github.com/<your-username>/quadruped-vio.git

# Install ROS 2 dependencies
cd ~/quad_ws
rosdep install --from-paths src --ignore-src -r -y

# Install VINS-Fusion external dependencies (Ceres, etc.)
cd src/vins_fusion
./install_external_deps.sh

# Build the workspace
cd ~/quad_ws
colcon build --symlink-install

# Source the workspace
source install/setup.bash
```

## Usage

Run each command in a separate terminal (remember to `source install/setup.bash` in each):

**1. Launch the Gazebo simulation with VIO odometry enabled**
```bash
ros2 launch champ_config gazebo.launch.py use_vio_odom:=true
```

**2. Launch the VINS-Fusion VIO estimator**
```bash
ros2 launch vins vins_mono.launch.py
```

**3. Launch RViz with the preconfigured VIO view**
```bash
rviz2 -d ~/quad_ws/src/vins_fusion/config/vins_rviz_config.rviz
```

**4. Launch SLAM (simulation mode)**
```bash
ros2 launch champ_config slam.launch.py sim:=true
```

## Repository Structure

```
quadruped-vio/
├── champ/              # Quadruped locomotion, control, and navigation stack
├── vins_fusion/         # VINS-Fusion VIO packages (vins, camera_models, global_fusion, loop_fusion)
├── assets/              # Images and GIFs used in this README
└── ...
```

## License

See [LICENCE](src/vins_fusion/LICENCE) for VINS-Fusion licensing terms.
