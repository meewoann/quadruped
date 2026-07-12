# Nav2 Key Parameters Reference

## Default Config File
`~/nav2_ws/src/navigation2/nav2_bringup/params/nav2_params.yaml`

## AMCL (Localization)

```yaml
amcl:
  ros__parameters:
    # Motion model
    robot_model_type: "nav2_amcl::DifferentialMotionModel"
    alpha1: 0.2   # Rotation noise from rotation
    alpha2: 0.2   # Rotation noise from translation
    alpha3: 0.2   # Translation noise from translation
    alpha4: 0.2   # Translation noise from rotation
    alpha5: 0.2   # Translation noise (omnidirectional)

    # Particle filter
    max_particles: 2000
    min_particles: 500
    pf_err: 0.05
    pf_z: 0.99
    recovery_alpha_slow: 0.0
    recovery_alpha_fast: 0.0
    resample_interval: 1

    # Sensor model
    laser_model_type: "likelihood_field"
    max_beams: 60
    laser_max_range: 100.0
    z_hit: 0.5
    z_rand: 0.5
    sigma_hit: 0.2

    # Update thresholds
    update_min_a: 0.2    # Min rotation to trigger update (rad)
    update_min_d: 0.25   # Min translation to trigger update (m)

    # Frames
    global_frame_id: "map"
    base_frame_id: "base_footprint"
    odom_frame_id: "odom"
```

## Controller Server

```yaml
controller_server:
  ros__parameters:
    controller_frequency: 20.0
    min_x_velocity_threshold: 0.001
    min_y_velocity_threshold: 0.5
    min_theta_velocity_threshold: 0.001
    costmap_update_timeout: 0.30
    progress_checker_plugins: ["progress_checker"]
    goal_checker_plugins: ["general_goal_checker"]
    controller_plugins: ["FollowPath"]

    # Progress checker
    progress_checker:
      plugin: "nav2_controller::SimpleProgressChecker"
      required_movement_radius: 0.5
      movement_time_allowance: 10.0

    # Goal checker
    general_goal_checker:
      plugin: "nav2_controller::SimpleGoalChecker"
      xy_goal_tolerance: 0.25
      yaw_goal_tolerance: 0.25
      stateful: True
```

## MPPI Controller (Default)

```yaml
    FollowPath:
      plugin: "nav2_mppi_controller::MPPIController"
      time_steps: 56
      model_dt: 0.05
      batch_size: 2000
      ax_max: 3.0
      ax_min: -3.0
      ay_max: 3.0
      az_max: 3.5
      vx_std: 0.2
      vy_std: 0.2
      wz_std: 0.4
      vx_max: 0.5
      vx_min: -0.35
      vy_max: 0.5
      wz_max: 1.9
      iteration_count: 1
      temperature: 0.3
      gamma: 0.015
      motion_model: "DiffDrive"  # or "Omnidirectional", "Ackermann"
      critics:
        - "ConstraintCritic"
        - "CostCritic"
        - "GoalCritic"
        - "GoalAngleCritic"
        - "PathAlignCritic"
        - "PathFollowCritic"
        - "PathAngleCritic"
        - "PreferForwardCritic"
```

## DWB Controller

```yaml
    FollowPath:
      plugin: "dwb_core::DWBLocalPlanner"
      min_vel_x: 0.0
      min_vel_y: 0.0
      max_vel_x: 0.26
      max_vel_y: 0.0
      max_vel_theta: 1.0
      min_speed_xy: 0.0
      max_speed_xy: 0.26
      min_speed_theta: 0.0
      acc_lim_x: 2.5
      acc_lim_y: 0.0
      acc_lim_theta: 3.2
      decel_lim_x: -2.5
      decel_lim_y: 0.0
      decel_lim_theta: -3.2
      vx_samples: 20
      vy_samples: 5
      vtheta_samples: 20
      sim_time: 1.7
      critics:
        - "RotateToGoal"
        - "Oscillation"
        - "BaseObstacle"
        - "GoalAlign"
        - "PathAlign"
        - "PathDist"
        - "GoalDist"
```

## Regulated Pure Pursuit Controller

```yaml
    FollowPath:
      plugin: "nav2_regulated_pure_pursuit_controller::RegulatedPurePursuitController"
      desired_linear_vel: 0.5
      lookahead_dist: 0.6
      min_lookahead_dist: 0.3
      max_lookahead_dist: 0.9
      lookahead_time: 1.5
      rotate_to_heading_angular_vel: 1.8
      use_velocity_scaled_lookahead_dist: false
      min_approach_linear_velocity: 0.05
      approach_velocity_scaling_dist: 1.0
      use_collision_detection: true
      max_allowed_time_to_collision_up_to_carrot: 1.0
      use_regulated_linear_velocity_scaling: true
      use_cost_regulated_linear_velocity_scaling: false
      regulated_linear_scaling_min_radius: 0.9
      regulated_linear_scaling_min_speed: 0.25
      allow_reversing: false
      max_robot_pose_search_dist: -1.0  # -1=full path
```

## Planner Server

```yaml
planner_server:
  ros__parameters:
    planner_plugins: ["GridBased"]
    GridBased:
      plugin: "nav2_navfn_planner::NavfnPlanner"
      tolerance: 0.5
      use_astar: true
      allow_unknown: true
```

## SMAC Planners

```yaml
    # Hybrid-A*
    GridBased:
      plugin: "nav2_smac_planner::SmacPlannerHybrid"
      tolerance: 0.25
      downsample_costmap: false
      allow_unknown: true
      max_iterations: 1000000
      max_on_approach_iterations: 1000
      max_planning_time: 5.0
      angle_quantization_bins: 72
      minimum_turning_radius: 0.40
      motion_model_for_search: "REEDS_SHEPP"  # or "DUBIN"
      cost_travel_multiplier: 2.0
      reverse_penalty: 2.0
      change_penalty: 0.0
      non_straight_penalty: 1.2
      cost_penalty: 2.0
      retrospective_penalty: 0.015
      analytic_expansion_ratio: 3.5
      analytic_expansion_max_length: 3.0
      smooth_path: true

    # 2D A*
    GridBased:
      plugin: "nav2_smac_planner::SmacPlanner2D"
      tolerance: 0.125
      allow_unknown: true
      max_iterations: 1000000

    # State Lattice
    GridBased:
      plugin: "nav2_smac_planner::SmacPlannerLattice"
      tolerance: 0.25
      allow_unknown: true
      lattice_filepath: ""  # Path to lattice file
```

## Local Costmap

```yaml
local_costmap:
  local_costmap:
    ros__parameters:
      update_frequency: 5.0
      publish_frequency: 2.0
      global_frame: odom
      robot_base_frame: base_link
      rolling_window: true
      width: 3
      height: 3
      resolution: 0.05
      footprint: "[[0.15,0.15],[0.15,-0.15],[-0.15,-0.15],[-0.15,0.15]]"
      plugins: ["voxel_layer", "inflation_layer"]
      voxel_layer:
        plugin: "nav2_costmap_2d::VoxelLayer"
        enabled: True
        observation_sources: scan
        scan:
          topic: /scan
          max_obstacle_height: 2.0
          clearing: True
          marking: True
          raytrace_max_range: 3.0
          raytrace_min_range: 0.0
          obstacle_max_range: 2.5
          obstacle_min_range: 0.0
      inflation_layer:
        plugin: "nav2_costmap_2d::InflationLayer"
        cost_scaling_factor: 3.0
        inflation_radius: 0.55
```

## Global Costmap

```yaml
global_costmap:
  global_costmap:
    ros__parameters:
      update_frequency: 1.0
      publish_frequency: 1.0
      global_frame: map
      robot_base_frame: base_link
      robot_radius: 0.22
      resolution: 0.05
      track_unknown_space: true
      plugins: ["static_layer", "obstacle_layer", "inflation_layer"]
      static_layer:
        plugin: "nav2_costmap_2d::StaticLayer"
        map_subscribe_transient_local: True
      obstacle_layer:
        plugin: "nav2_costmap_2d::ObstacleLayer"
        enabled: True
        observation_sources: scan
        scan:
          topic: /scan
          max_obstacle_height: 2.0
          clearing: True
          marking: True
      inflation_layer:
        plugin: "nav2_costmap_2d::InflationLayer"
        cost_scaling_factor: 3.0
        inflation_radius: 0.55
```

## Behavior Server

```yaml
behavior_server:
  ros__parameters:
    behavior_plugins: ["spin", "backup", "drive_on_heading", "assisted_teleop", "wait"]
    spin:
      plugin: "nav2_behaviors::Spin"
    backup:
      plugin: "nav2_behaviors::BackUp"
    drive_on_heading:
      plugin: "nav2_behaviors::DriveOnHeading"
    wait:
      plugin: "nav2_behaviors::Wait"
    assisted_teleop:
      plugin: "nav2_behaviors::AssistedTeleop"
```

## Velocity Smoother

```yaml
velocity_smoother:
  ros__parameters:
    smoothing_frequency: 20.0
    scale_velocities: false
    feedback: "OPEN_LOOP"
    max_velocity: [0.26, 0.0, 1.0]
    min_velocity: [-0.26, 0.0, -1.0]
    max_accel: [2.5, 0.0, 3.2]
    max_decel: [-2.5, 0.0, -3.2]
    deadband_velocity: [0.0, 0.0, 0.0]
    velocity_timeout: 1.0
```

## Collision Monitor

```yaml
collision_monitor:
  ros__parameters:
    cmd_vel_in_topic: "cmd_vel_smoothed"
    cmd_vel_out_topic: "cmd_vel"
    transform_tolerance: 0.5
    source_timeout: 5.0
    polygons: ["PolygonStop", "FootprintApproach"]
    PolygonStop:
      type: "circle"
      radius: 0.3
      action_type: "stop"
    FootprintApproach:
      type: "polygon"
      action_type: "approach"
      footprint_topic: "/local_costmap/published_footprint"
      time_before_collision: 2.0
      simulation_time_step: 0.02
      max_points: 5
    observation_sources: ["scan"]
    scan:
      type: "scan"
      topic: "/scan"
```

## Lifecycle Manager

```yaml
lifecycle_manager:
  ros__parameters:
    autostart: true
    bond_timeout: 4.0
    node_names:
      - "map_server"
      - "amcl"
      - "controller_server"
      - "smoother_server"
      - "planner_server"
      - "behavior_server"
      - "bt_navigator"
      - "waypoint_follower"
      - "velocity_smoother"
      - "collision_monitor"
```

## Costmap Cost Values

| Value | Meaning |
|-------|---------|
| 0 | FREE_SPACE |
| 1-252 | Cost gradient |
| 253 | INSCRIBED_INFLATED_OBSTACLE |
| 254 | LETHAL_OBSTACLE |
| 255 | NO_INFORMATION (unknown) |
