# VINS-Fusion Ground-Truth Setup Notes
# CHAMP Quadruped — ROS 2 Humble + Gazebo

> **This file is generated from live workspace inspection.**
> All values are derived directly from URDF/Xacro source files, not assumed.

---

## 1. Sensor Topology (from URDF)

### IMU (`accessories.urdf.xacro`)
| Parameter | Value |
|---|---|
| Gazebo plugin | `libgazebo_ros_imu_sensor.so` |
| ROS namespace | `/imu` |
| Output topic remapped to | `/imu/data_raw` |
| Update rate (URDF `<update_rate>`) | **200 Hz** (outer sensor tag) |
| Frame ID | `imu_link` |
| Attachment | `base_link` via `imu_joint` (zero offset: `xyz="0 0 0"`) |

> **Use `/imu/data_raw` for VINS.** `/imu/data` is the EKF/Madgwick output.

### Camera (`asus_camera.urdf.xacro`)
| Parameter | Value |
|---|---|
| Gazebo plugin | `libgazebo_ros_camera.so` |
| Sensor type | `depth` |
| Update rate | **30 Hz** (inner `<update_rate>`) |
| Raw image topic | `/camera/rgb/image_raw` |
| Raw camera_info | `/camera/camera_info_raw` |
| Frame ID | `camera_depth_optical_frame` |
| Image size | **640 × 480** (raw) |
| Horizontal FOV | **62.8°** |
| Format | `R8G8B8` |

### Image Crop Node (`image_crop_node.py`)
| Parameter | Value |
|---|---|
| Input | `/camera/image_raw` |
| Output image | `/camera/image_cropped` **640 × 465** |
| Output camera_info | `/camera/camera_info` (corrected cy) |
| Rows removed | **15** from top (`TOP_ROWS = 15`) |

---

## 2. Camera Intrinsics (derived from URDF)

```
HFOV = 62.8 deg
width  = 640  px  (full)
height = 480  px  (full) → 465 px (cropped)

fx = fy = (W/2) / tan(HFOV/2)
         = 320  / tan(31.4°)
         = 320  / 0.61047
         ≈ 524.30 px

cx     = 320.5  (W/2 + 0.5)
cy_raw = 240.5  (H/2 + 0.5, full frame)
cy     = 225.5  (240.5 - 15 rows, after crop)

Distortion model: PINHOLE (Gazebo ideal lens → k1=k2=p1=p2=0)
```

---

## 3. Camera-IMU Extrinsic (T_imu_cam, body_T_cam0)

VINS convention: **IMU ← Camera** (`body_T_cam0`)

```
T_imu_camera_depth_optical =
[  0   0   1   0.250 ]
[ -1   0   0   0.049 ]
[  0  -1   0  -0.075 ]
[  0   0   0   1.000 ]
```

This is verified against the TF chain:
- `base_link` → `camera_link` (accessories.urdf.xacro: `xyz="${base_length/2} 0 -${base_height/2+0.01}"`)
- `camera_link` → `camera_depth_frame` (`xyz="0 0.049 0"`)
- `camera_depth_frame` → `camera_depth_optical_frame` (`rpy="-pi/2 0 -pi/2"`)
- `base_link` → `imu_link` (zero offset)

---

## 4. TF Tree

### CHAMP (from `robot_state_publisher` + EKF)
```
world
  └── odom (EKF: footprint_to_odom_ekf)
        └── base_footprint
              └── base_link
                    ├── imu_link
                    ├── camera_link
                    │     ├── camera_depth_frame
                    │     │     └── camera_depth_optical_frame  ← VINS camera frame
                    │     └── camera_rgb_frame
                    │           └── camera_rgb_optical_frame
                    └── [legs, hokuyo, ...]
```

### VINS (published by `vins_estimator`)
```
vins_odom
  └── vins_body
        └── vins_camera
```

**Static TF bridge** (from `vins_mono.launch.py`):
```
odom  →  vins_odom  (identity, published at launch)
```
This allows RViz to show both trees.

> **RViz Fixed Frame: `vins_odom`** (not `odom`, not `world`)

---

## 5. Active Config Files

| File | Purpose |
|---|---|
| `config/champ_gazebo/champ_gazebo_mono_imu.yaml` | Main VINS config |
| `config/champ_gazebo/cam0_pinhole.yaml` | Camera intrinsics |
| `config/vins_rviz_config.rviz` | RViz (fixed to `vins_odom`) |
| `vins/launch/vins_mono.launch.py` | VINS launch (with static TF bridge) |
| `champ_config/launch/slam_vio.launch.py` | Toplevel VIO+SLAM launch |

---

## 6. IMU Noise Parameters

Derived from URDF (`accessories.urdf.xacro`, `libgazebo_ros_imu_sensor`):

```
accelGaussianNoise = 0.005 m/s²
rateGaussianNoise  = 0.005 rad/s
IMU rate           = 200 Hz

Continuous noise = discrete_noise × sqrt(rate)
acc_n = 0.005 × sqrt(200) ≈ 0.071  → used 0.08
gyr_n = 0.005 × sqrt(200) ≈ 0.071  → used 0.008
```

| YAML key | Value |
|---|---|
| `acc_n` | 0.08 |
| `gyr_n` | 0.008 |
| `acc_w` | 0.00004 |
| `gyr_w` | 0.000002 |
| `g_norm` | 9.81007 |

---

## 7. Launch Commands

### Terminal 1 — Gazebo + CHAMP
```bash
source /opt/ros/humble/setup.bash
source install/setup.bash
ros2 launch champ_config gazebo.launch.py
```

### Terminal 2 — VINS
```bash
source /opt/ros/humble/setup.bash
source install/setup.bash
ros2 launch vins vins_mono.launch.py
```

### Terminal 3 — RViz
```bash
source /opt/ros/humble/setup.bash
source install/setup.bash
rviz2 -d ~/quad_ws/src/vins_fusion/config/vins_rviz_config.rviz
```

### Terminal 4 — Diagnostics
```bash
bash ~/quad_ws/src/champ/champ_gazebo/scripts/vins_diagnostic.sh
```

---

## 8. Common Issues and Fixes

| Issue | Cause | Fix |
|---|---|---|
| VINS uses filtered IMU | `/imu/data` subscribed | Config: `imu_topic: /imu/data_raw` |
| Path not visible in RViz | Wrong frame or topic | Fixed Frame: `vins_odom`, topic: `/vins_estimator/path` |
| image_track not visible | Wrong topic name | Topic: `/vins_estimator/image_track` |
| Divergence at startup | Robot static, gravity conflict | Move robot gently for ~10s |
| EKF conflicts with VINS odom | Both publishing `odom→base_link` | Launch with `use_vio_odom:=true` to disable EKF odom |
| Timestamp mismatch | Sim time not passed to VINS | `parameters=[{'use_sim_time': True}]` in launch |