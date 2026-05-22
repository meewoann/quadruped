#!/bin/bash
# VINS Runtime Diagnostic Script — v3 (Gazebo + CHAMP quadruped)
# Inspects live topics from the actual workspace configuration.
#
# Raw sensor topics (from URDF inspection):
#   /imu/data_raw           → libgazebo_ros_imu_sensor (accessories.urdf.xacro)
#   /camera/rgb/image_raw   → libgazebo_ros_camera (asus_camera.urdf.xacro) raw output
#   /camera/image_cropped   → image_crop_node.py (640x465, 15 rows removed)
#   /camera/camera_info     → image_crop_node.py (corrected cy=225.5)
#
# Filtered (DO NOT use for VINS):
#   /imu/data               → EKF or Madgwick output
#
# Usage: bash ~/quad_ws/src/champ/champ_gazebo/scripts/vins_diagnostic.sh

source /opt/ros/humble/setup.bash
source /home/meewoan/quad_ws/install/setup.bash

YELLOW='\033[1;33m'
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

check() {
    local label="$1"; local topic="$2"; local expected="$3"
    printf "  %-45s" "$label"
    RATE=$(timeout 5 ros2 topic hz "$topic" 2>/dev/null | grep "average rate" | tail -1 | awk '{print $3}' | tr -d ':')
    if [ -z "$RATE" ]; then
        echo -e "${RED}NOT ACTIVE${NC}"
    else
        echo -e "${GREEN}${RATE} Hz${NC}  (expected: ${expected})"
    fi
}

echo ""
echo "=========================================================="
echo "  VINS Runtime Diagnostics v3 — CHAMP Quadruped"
echo "=========================================================="

echo ""
echo -e "${YELLOW}[A] RAW Sensor Topics (VINS inputs)${NC}"
check "/imu/data_raw  (raw Gazebo IMU)"  /imu/data_raw    "~200 Hz"
check "/camera/image_cropped  (VIO image)"  /camera/image_cropped  "~20 Hz"
check "/camera/camera_info  (corrected)"  /camera/camera_info    "~20 Hz"

echo ""
echo -e "${YELLOW}[B] Raw camera output from Gazebo plugin${NC}"
check "/camera/rgb/image_raw"  /camera/rgb/image_raw  "~20 Hz"

echo ""
echo -e "${YELLOW}[C] Filtered IMU (should NOT be used by VINS)${NC}"
check "/imu/data  (EKF/Madgwick — skip for VINS)"  /imu/data  "N/A"

echo ""
echo -e "${YELLOW}[D] VINS Estimator Outputs${NC}"
check "/odometry (VINS raw)"           /odometry                       "~10 Hz (after init)"
check "/vins_estimator/imu_propagate"  /vins_estimator/imu_propagate   "~200 Hz (after init)"
check "/vins_estimator/path"           /vins_estimator/path            "~10 Hz"
check "/vins_estimator/image_track"    /vins_estimator/image_track     "~10 Hz"

echo ""
echo "=========================================================="
echo -e "${YELLOW}[E] IMU gravity check (static: z should be ~+9.81 m/s^2)${NC}"
timeout 3 ros2 topic echo /imu/data_raw --once 2>/dev/null \
    | grep -A 4 "linear_acceleration:" \
    || echo "  /imu/data_raw NOT available"

echo ""
echo "=========================================================="
echo -e "${YELLOW}[F] VINS initialization check${NC}"
if ros2 topic list 2>/dev/null | grep -q "^/odometry$"; then
    echo -e "  ${GREEN}✓ /odometry topic EXISTS${NC}"
    MSGS=$(timeout 2 ros2 topic echo /odometry 2>/dev/null | grep -c "frame_id")
    if [ "$MSGS" -gt "0" ]; then
        echo -e "  ${GREEN}✓ VINS IS INITIALIZED and publishing!${NC}"
    else
        echo -e "  ${RED}✗ Topic exists but no messages yet — still initializing${NC}"
        echo "    → Move the robot gently for ~10s to initialize"
    fi
else
    echo -e "  ${RED}✗ /odometry NOT in topic list${NC}"
    echo "    → Nodes running:"
    ros2 node list 2>/dev/null | grep -i vins || echo "    (none)"
fi

echo ""
echo "=========================================================="
echo -e "${YELLOW}[G] TF Tree check${NC}"
echo "  Active TF frames:"
timeout 3 ros2 run tf2_ros tf2_echo odom base_footprint 2>/dev/null | head -5 || \
    echo "  odom→base_footprint: NOT AVAILABLE"
timeout 3 ros2 run tf2_ros tf2_echo vins_odom vins_body 2>/dev/null | head -5 || \
    echo "  vins_odom→vins_body: NOT AVAILABLE (VINS not initialized)"

echo ""
echo "=========================================================="
echo -e "${YELLOW}[H] Image encoding check${NC}"
timeout 3 ros2 topic echo /camera/image_cropped --once 2>/dev/null \
    | grep -E "encoding|width|height" \
    || echo "  /camera/image_cropped NOT available"

echo ""
echo "=========================================================="
echo "  Expected values:"
echo "    /imu/data_raw rate:           ~200 Hz"
echo "    /camera/image_cropped rate:   ~20 Hz"
echo "    /odometry:                    ~10 Hz (after init)"
echo "    IMU z at rest:                ~+9.81 m/s^2"
echo "    Image encoding:               rgb8 or bgr8 (VINS converts internally)"
echo "    Image size:                   640 x 465 (cropped)"
echo "=========================================================="
