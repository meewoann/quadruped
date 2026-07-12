# ros2_controllers — Package Catalog Reference

Per-package reference for every controller and broadcaster in
`~/nav2_ws/src/ros2_controllers/` (~v6.7.0). Framework concepts are in
`ros2_control_architecture.md`. Each package's authoritative docs are in
`<pkg>/doc/userdoc.rst`; tunable parameters in
`<pkg>/src/<pkg>_parameters.yaml`.

**Plugin type column:** `Plain` = `controller_interface::ControllerInterface`
(overrides `update()`); `Chain` = `controller_interface::ChainableControllerInterface`
(overrides `update_reference_from_subscribers()` + `update_and_write_commands()`,
can expose reference/state interfaces for chaining).

---

## Mobile-base drive controllers

| Package | Plugin class | Type | Command ifaces | State ifaces | ROS I/O |
|---------|-------------|------|----------------|--------------|---------|
| `diff_drive_controller` | `diff_drive_controller/DiffDriveController` | Chain | wheel `velocity` (L/R) | wheel `velocity`/`position` | in `~/cmd_vel` (TwistStamped) · out `~/odom` (nav_msgs/Odometry) + TF |
| `mecanum_drive_controller` | `mecanum_drive_controller/MecanumDriveController` | Chain | 4× wheel `velocity` | wheel feedback | cmd_vel → odom |
| `omni_wheel_drive_controller` | `omni_wheel_drive_controller/OmniWheelDriveController` | Chain | N× wheel `velocity` | wheel feedback | cmd_vel → odom |
| `tricycle_controller` | `tricycle_controller/TricycleController` | Plain | traction `velocity` + steering `position` | feedback | cmd_vel → odom |

## Steering controllers (Ackermann-family)

Built on the shared **`steering_controllers_library`** (Chain base +
`steering_odometry` / `steering_kinematics`). Each derived package only
supplies the kinematics specialization:

| Package | Plugin class | Geometry |
|---------|-------------|----------|
| `steering_controllers_library` | *(base library, no plugin)* | common odometry + chaining for all below |
| `ackermann_steering_controller` | `ackermann_steering_controller/AckermannSteeringController` | 2 steered + 2 traction |
| `bicycle_steering_controller` | `bicycle_steering_controller/BicycleSteeringController` | 1 steered + 1 traction |
| `tricycle_steering_controller` | `tricycle_steering_controller/TricycleSteeringController` | 1 steered + 2 traction |

## Joint / manipulator controllers

| Package | Plugin class | Type | Notes |
|---------|-------------|------|-------|
| `joint_trajectory_controller` (JTC) | `joint_trajectory_controller/JointTrajectoryController` | Plain | Executes `trajectory_msgs/JointTrajectory`; action `FollowJointTrajectory`; spline interpolation; supports position/velocity/effort/acceleration command combos, speed scaling. The workhorse for arms. |
| `forward_command_controller` | `forward_command_controller/ForwardCommandController` | Plain | Passes a `std_msgs` command array straight to one command interface type. Base: `ForwardControllersBase`. Multi-interface variant: `MultiInterfaceForwardCommandController`. |
| `pid_controller` | `pid_controller/PidController` | Chain | Generic PID around a reference/state interface; chainable so it can sit between a higher controller and hardware. |
| `parallel_gripper_controller` | `parallel_gripper_action_controller/GripperActionController` | Plain | `control_msgs/GripperCommand` action for a parallel-jaw gripper. |
| `gpio_controllers` | `gpio_controllers/GpioCommandController` | Plain | Read/write arbitrary `gpio/*` command & state interfaces. |
| `admittance_controller` | `admittance_controller/AdmittanceController` | Chain | 6-DOF force-based admittance using an FT sensor + kinematics plugin; compliant interaction. |
| `motion_primitives_controllers` | `motion_primitives_controllers/MotionPrimitivesForwardController` | Plain | Executes motion primitives (PTP/LIN/CIRC) via an execution action. |
| `chained_filter_controller` | `chained_filter_controller/ChainedFilter` | Chain | Applies a `filters`-library filter to a state interface and re-exports it — pure chaining building block. |

## Broadcasters (read-only, publish state to ROS)

All claim **NONE** command interfaces; they read state interfaces and
publish a message. (`Type` reflects the base class actually used.)

| Package | Plugin class | Type | Publishes |
|---------|-------------|------|-----------|
| `joint_state_broadcaster` | `joint_state_broadcaster/JointStateBroadcaster` | Plain | `sensor_msgs/JointState` + `control_msgs/DynamicJointState` for all or selected joints |
| `imu_sensor_broadcaster` | `imu_sensor_broadcaster/IMUSensorBroadcaster` | Chain | `sensor_msgs/Imu` (uses `semantic_components::IMUSensor`) |
| `force_torque_sensor_broadcaster` | `force_torque_sensor_broadcaster/ForceTorqueSensorBroadcaster` | Chain | `geometry_msgs/WrenchStamped` |
| `pose_broadcaster` | `pose_broadcaster/PoseBroadcaster` | Plain | `geometry_msgs/PoseStamped` (+ TF) |
| `range_sensor_broadcaster` | `range_sensor_broadcaster/RangeSensorBroadcaster` | Plain | `sensor_msgs/Range` |
| `gps_sensor_broadcaster` | `gps_sensor_broadcaster/GPSSensorBroadcaster` | Plain | `sensor_msgs/NavSatFix` |
| `magnetometer_broadcaster` | `magnetometer_broadcaster/MagnetometerBroadcaster` | Plain | `sensor_msgs/MagneticField` |
| `state_interfaces_broadcaster` | `state_interfaces_broadcaster/StateInterfacesBroadcaster` | Plain | generic `control_msgs/DynamicInterfaceGroupValues` for arbitrary state interfaces |

## Tooling / non-controller packages

| Package | Purpose |
|---------|---------|
| `ros2_controllers` | metapackage (depends on all of the above) |
| `ros2_controllers_test_nodes` | Python test publishers (e.g. `publisher_joint_trajectory_controller`) for manual bring-up |
| `rqt_joint_trajectory_controller` | rqt GUI to jog JTC-controlled joints |

---

## How to run a controller (recap)

Controllers are spawned into a running `controller_manager`:

```yaml
# controllers.yaml
controller_manager:
  ros__parameters:
    update_rate: 100  # Hz
    diff_drive_controller:
      type: diff_drive_controller/DiffDriveController   # ← the plugin alias
    joint_state_broadcaster:
      type: joint_state_broadcaster/JointStateBroadcaster

diff_drive_controller:
  ros__parameters:
    left_wheel_names: ["left_wheel_joint"]
    right_wheel_names: ["right_wheel_joint"]
    wheel_separation: 0.40
    wheel_radius: 0.05
```

```bash
ros2 run controller_manager spawner diff_drive_controller
ros2 run controller_manager spawner joint_state_broadcaster
ros2 control list_controllers          # ros2controlcli
ros2 control list_hardware_interfaces
```

For field-level parameter detail, read the generated header or the
`*_parameters.yaml`; for behaviour, read `<pkg>/doc/userdoc.rst`.
