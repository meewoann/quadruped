# Nav2 Commands Reference

## Build & Launch

```bash
# Build Nav2 workspace
cd ~/nav2_ws && colcon build --symlink-install

# Build specific Nav2 package
colcon build --packages-select nav2_controller

# Source workspace
source ~/nav2_ws/install/setup.bash

# Launch full Nav2 stack
ros2 launch nav2_bringup bringup_launch.py \
  map:=/path/to/map.yaml \
  params_file:=/path/to/nav2_params.yaml \
  use_sim_time:=false

# Launch navigation only (without localization)
ros2 launch nav2_bringup navigation_launch.py \
  params_file:=/path/to/nav2_params.yaml

# Launch localization only
ros2 launch nav2_bringup localization_launch.py \
  map:=/path/to/map.yaml \
  params_file:=/path/to/nav2_params.yaml

# Launch with loopback simulator (no Gazebo)
ros2 launch nav2_bringup tb3_loopback_simulation_launch.py \
  params_file:=/path/to/nav2_params.yaml

# Launch RViz
ros2 launch nav2_bringup rviz_launch.py
```

## Navigation Commands

```bash
# Send navigation goal
ros2 action send_goal /navigate_to_pose nav2_msgs/action/NavigateToPose \
  "{pose: {header: {frame_id: 'map'}, pose: {position: {x: 1.0, y: 2.0}, orientation: {w: 1.0}}}}"

# Navigate through poses
ros2 action send_goal /navigate_through_poses nav2_msgs/action/NavigateThroughPoses \
  "{poses: [...]}"

# Follow waypoints
ros2 action send_goal /follow_waypoints nav2_msgs/action/FollowWaypoints \
  "{poses: [...]}"

# Compute path only (no execution)
ros2 action send_goal /compute_path_to_pose nav2_msgs/action/ComputePathToPose \
  "{goal: {header: {frame_id: 'map'}, pose: {position: {x: 5.0, y: 3.0}, orientation: {w: 1.0}}}, planner_id: 'GridBased', use_start: false}"
```

## Recovery Behavior Commands

```bash
# Spin in place (1.57 rad = 90 degrees)
ros2 action send_goal /spin nav2_msgs/action/Spin \
  "{target_yaw: 1.57, time_allowance: {sec: 10}}"

# Backup 0.3 meters
ros2 action send_goal /backup nav2_msgs/action/BackUp \
  "{target: {x: -0.3}, speed: 0.15, time_allowance: {sec: 10}}"

# Wait 5 seconds
ros2 action send_goal /wait nav2_msgs/action/Wait \
  "{time: {sec: 5}}"

# Drive on heading 0.5 meters
ros2 action send_goal /drive_on_heading nav2_msgs/action/DriveOnHeading \
  "{target: {x: 0.5}, speed: 0.2, time_allowance: {sec: 10}}"
```

## Costmap Commands

```bash
# Clear entire global costmap
ros2 service call /global_costmap/clear_entirely_global_costmap \
  nav2_msgs/srv/ClearEntireCostmap

# Clear entire local costmap
ros2 service call /local_costmap/clear_entirely_local_costmap \
  nav2_msgs/srv/ClearEntireCostmap

# Clear around robot
ros2 service call /global_costmap/clear_around_global_costmap \
  nav2_msgs/srv/ClearCostmapAroundRobot "{reset_distance: 2.0}"

# Check path validity
ros2 service call /is_path_valid nav2_msgs/srv/IsPathValid \
  "{path: {poses: [...]}, check_full_path: true}"

# Get costs at poses
ros2 service call /global_costmap/get_costs nav2_msgs/srv/GetCosts \
  "{use_footprint: true, poses: [...]}"
```

## Map Commands

```bash
# Load a new map
ros2 service call /map_server/load_map nav2_msgs/srv/LoadMap \
  "{map_url: '/path/to/map.yaml'}"

# Save current map
ros2 run nav2_map_server map_saver_cli -f ~/maps/my_map

# Save map via service
ros2 service call /map_saver/save_map nav2_msgs/srv/SaveMap \
  "{map_url: '/path/to/save', map_topic: '/map', image_format: 'pgm'}"
```

## Lifecycle Management

```bash
# Startup all nodes
ros2 service call /lifecycle_manager_navigation/manage_nodes \
  nav2_msgs/srv/ManageLifecycleNodes "{command: 0}"

# Pause (deactivate) all nodes
ros2 service call /lifecycle_manager_navigation/manage_nodes \
  nav2_msgs/srv/ManageLifecycleNodes "{command: 1}"

# Resume (activate) all nodes
ros2 service call /lifecycle_manager_navigation/manage_nodes \
  nav2_msgs/srv/ManageLifecycleNodes "{command: 2}"

# Shutdown all nodes
ros2 service call /lifecycle_manager_navigation/manage_nodes \
  nav2_msgs/srv/ManageLifecycleNodes "{command: 4}"

# Check individual node lifecycle state
ros2 lifecycle get /controller_server
ros2 lifecycle get /planner_server
ros2 lifecycle get /bt_navigator
```

## Localization Commands

```bash
# Set initial pose
ros2 topic pub --once /initialpose \
  geometry_msgs/msg/PoseWithCovarianceStamped \
  "{header: {frame_id: 'map'}, pose: {pose: {position: {x: 0.0, y: 0.0}, orientation: {w: 1.0}}}}"

# Reinitialize global localization (spread particles)
ros2 service call /reinitialize_global_localization std_srvs/srv/Empty

# Force AMCL update without motion
ros2 service call /request_nomotion_update std_srvs/srv/Empty
```

## Docking Commands

```bash
# Dock robot
ros2 action send_goal /dock_robot nav2_msgs/action/DockRobot \
  "{use_dock_id: true, dock_id: 'home_dock', navigate_to_staging_pose: true}"

# Undock robot
ros2 action send_goal /undock_robot nav2_msgs/action/UndockRobot \
  "{dock_type: 'simple_charging_dock'}"

# Reload dock database
ros2 service call /docking_server/reload_database nav2_msgs/srv/ReloadDockDatabase \
  "{filepath: '/path/to/docks.yaml'}"
```

## Route Commands

```bash
# Set route graph
ros2 service call /route_server/set_route_graph nav2_msgs/srv/SetRouteGraph \
  "{graph_filepath: '/path/to/graph.geojson'}"

# Compute route
ros2 action send_goal /compute_route nav2_msgs/action/ComputeRoute \
  "{goal: {header: {frame_id: 'map'}, pose: {...}}, use_poses: true}"

# Dynamic edge control
ros2 service call /route_server/dynamic_edges nav2_msgs/srv/DynamicEdges \
  "{closed_edges: [5, 12], opened_edges: [3]}"
```

## Collision Monitor Commands

```bash
# Toggle collision monitor
ros2 service call /collision_monitor/toggle nav2_msgs/srv/Toggle \
  "{enable: true}"

# Monitor state
ros2 topic echo /collision_monitor_state
```

## Debugging & Monitoring

```bash
# List all Nav2 topics
ros2 topic list | grep -E "nav2|costmap|plan|cmd_vel|amcl|scan"

# Monitor controller output
ros2 topic echo /cmd_vel

# Monitor planned path
ros2 topic echo /plan

# Monitor costmap updates
ros2 topic echo /global_costmap/costmap
ros2 topic echo /local_costmap/costmap

# Monitor AMCL pose
ros2 topic echo /amcl_pose

# Monitor BT status
ros2 topic echo /behavior_tree_log

# Check TF tree
ros2 run tf2_tools view_frames

# Monitor specific transform
ros2 run tf2_ros tf2_echo map base_link

# Parameter inspection
ros2 param list /controller_server
ros2 param get /controller_server controller_frequency
ros2 param set /controller_server controller_frequency 30.0

# Node info
ros2 node info /controller_server
ros2 node info /planner_server
ros2 node info /bt_navigator

# Topic frequencies
ros2 topic hz /cmd_vel
ros2 topic hz /scan
ros2 topic hz /local_costmap/costmap
```

## Dynamic Parameter Changes

```bash
# Change controller frequency
ros2 param set /controller_server controller_frequency 30.0

# Change planner
ros2 param set /planner_server planner_plugins "['GridBased']"

# Change inflation radius
ros2 param set /local_costmap/local_costmap inflation_layer.inflation_radius 0.8

# Change AMCL particles
ros2 param set /amcl max_particles 5000
```
