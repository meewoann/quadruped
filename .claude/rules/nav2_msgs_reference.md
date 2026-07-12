# Nav2 Messages, Services & Actions Reference

Source: `~/nav2_ws/src/navigation2/nav2_msgs/`

## Messages (30)

### Costmap
| Message | Key Fields |
|---------|------------|
| `Costmap` | header, metadata (CostmapMetaData), uint8[] data |
| `CostmapMetaData` | map_load_time, update_time, layer, resolution, size_x/y, origin |
| `CostmapUpdate` | header, x, y, size_x/y, uint8[] data (partial update) |
| `CostmapFilterInfo` | type (0=keepout,1=speed%,2=speed_m/s), filter_mask_topic, base, multiplier |

### Path/Trajectory
| Message | Key Fields |
|---------|------------|
| `Trajectory` | header, TrajectoryPoint[] points |
| `TrajectoryPoint` | time_from_start, pose, velocity, acceleration, effort |

### Localization
| Message | Key Fields |
|---------|------------|
| `ParticleCloud` | header, Particle[] particles |
| `Particle` | pose (Pose), weight (float64) |
| `VoxelGrid` | header, uint32[] data, origin, resolutions, size_x/y/z |

### Route/Graph
| Message | Key Fields |
|---------|------------|
| `Route` | header, route_cost, RouteNode[], RouteEdge[] |
| `RouteNode` | nodeid (uint16), position (Point) |
| `RouteEdge` | edgeid (uint16), start (Point), end (Point) |
| `EdgeCost` | edgeid (uint16), cost (float32) |

### Control
| Message | Key Fields |
|---------|------------|
| `SpeedLimit` | header, percentage (bool), speed_limit (float64) |
| `CollisionMonitorState` | action_type (0-4), polygon_name |
| `CollisionDetectorState` | polygons[], detections[] |

### Geometry Objects
| Message | Key Fields |
|---------|------------|
| `PolygonObject` | header, uuid, Point32[] points, closed, value |
| `CircleObject` | header, uuid, center, radius, fill, value |

### BT Logging
| Message | Key Fields |
|---------|------------|
| `BehaviorTreeLog` | timestamp, BehaviorTreeStatusChange[] event_log |
| `BehaviorTreeStatusChange` | timestamp, node_name, uid, previous/current_status |

### Tracking
| Message | Key Fields |
|---------|------------|
| `TrackingFeedback` | header, position/heading_tracking_error, current_path_index, distance_to_goal, speed |
| `WaypointStatus` | status (PENDING=0,COMPLETED=1,SKIPPED=2,FAILED=3), waypoint_index, error_code |
| `CriticsStats` | stamp, critics[], changed[], costs_sum[] |

---

## Services (18)

### Map Management
| Service | Request | Response |
|---------|---------|----------|
| `LoadMap` | map_url | map (OccupancyGrid), result (0=OK,1=NOT_EXIST,2=INVALID_DATA) |
| `SaveMap` | map_topic, map_url, image_format, map_mode, thresholds | result (bool) |

### Costmap
| Service | Request | Response |
|---------|---------|----------|
| `GetCostmap` | specs (CostmapMetaData) | map (Costmap) |
| `ClearEntireCostmap` | (empty) | (empty) |
| `ClearCostmapAroundPose` | pose, reset_distance | (empty) |
| `ClearCostmapAroundRobot` | reset_distance | (empty) |
| `ClearCostmapExceptRegion` | reset_distance | (empty) |

### Path Validation
| Service | Request | Response |
|---------|---------|----------|
| `IsPathValid` | path, max_cost, consider_unknown, layer_name, footprint, check_full_path | is_valid, invalid_pose_indices[] |
| `GetCosts` | use_footprint, poses[] | costs[], success |

### Shapes
| Service | Request | Response |
|---------|---------|----------|
| `AddShapes` | circles[], polygons[] | success |
| `RemoveShapes` | all_objects, uuids[] | success |
| `GetShapes` | (none) | circles[], polygons[] |

### Lifecycle
| Service | Request | Response |
|---------|---------|----------|
| `ManageLifecycleNodes` | command (STARTUP=0,PAUSE=1,RESUME=2,RESET=3,SHUTDOWN=4) | success |
| `SetInitialPose` | pose (PoseWithCovarianceStamped) | (none) |

### Route
| Service | Request | Response |
|---------|---------|----------|
| `SetRouteGraph` | graph_filepath | success |
| `DynamicEdges` | closed_edges[], opened_edges[], adjust_edges[] | success |

### Other
| Service | Request | Response |
|---------|---------|----------|
| `ReloadDockDatabase` | filepath | success |
| `Toggle` | enable (bool) | success, message |

---

## Actions (19)

### Navigation
| Action | Goal | Result Key Fields |
|--------|------|-------------------|
| `NavigateToPose` | pose, behavior_tree | error_code (9000-9003) |
| `NavigateThroughPoses` | poses, behavior_tree | error_code (9100-9103), waypoint_statuses |

### Planning
| Action | Goal | Result Key Fields |
|--------|------|-------------------|
| `ComputePathToPose` | goal, start, planner_id, use_start | path, planning_time, error_code (200-208) |
| `ComputePathThroughPoses` | goals, start, planner_id, use_start | path, planning_time, error_code (300-309) |

### Path Following
| Action | Goal | Result Key Fields |
|--------|------|-------------------|
| `FollowPath` | path, controller_id, goal_checker_id, progress_checker_id, path_handler_id | error_code (100-107) |
| `SmoothPath` | path, smoother_id, max_duration, check_collisions | path, was_completed, error_code (500-505) |

### Route
| Action | Goal | Result Key Fields |
|--------|------|-------------------|
| `ComputeRoute` | start, goal, use_start, use_poses | path, route, error_code (400-407) |
| `ComputeAndTrackRoute` | start, goal, use_start, use_poses | execution_duration, error_code (400-407) |

### Waypoints
| Action | Goal | Result Key Fields |
|--------|------|-------------------|
| `FollowWaypoints` | number_of_loops, goal_index, poses[] | missed_waypoints, error_code (600-603) |
| `FollowGPSWaypoints` | number_of_loops, goal_index, gps_poses[] | missed_waypoints, error_code (600-603) |

### Recovery Behaviors
| Action | Goal | Result Key Fields |
|--------|------|-------------------|
| `Spin` | target_yaw, time_allowance, disable_collision_checks | error_code (700-703) |
| `BackUp` | target (Point), speed, time_allowance, disable_collision_checks | error_code (710-714) |
| `DriveOnHeading` | target, speed, time_allowance, disable_collision_checks | error_code (720-724) |
| `Wait` | time (Duration) | error_code (740-741) |
| `AssistedTeleop` | time_allowance | error_code (730-732) |

### Docking
| Action | Goal | Result Key Fields |
|--------|------|-------------------|
| `DockRobot` | dock_id, dock_pose, dock_type, navigate_to_staging | success, error_code (901-999) |
| `UndockRobot` | dock_type, max_undocking_time | success, error_code (902-999) |

### Following
| Action | Goal | Result Key Fields |
|--------|------|-------------------|
| `FollowObject` | pose_topic, tracked_frame, max_duration | error_code (901-999) |

### Utility
| Action | Goal | Result Key Fields |
|--------|------|-------------------|
| `DummyBehavior` | command (String) | error_code |

---

## Error Code Ranges

| Range | Component |
|-------|-----------|
| 100-107 | Controller / FollowPath |
| 200-208 | Planner / ComputePathToPose |
| 300-309 | Planner / ComputePathThroughPoses |
| 400-407 | Route Server |
| 500-505 | Smoother |
| 600-603 | Waypoint Follower |
| 700-703 | Spin |
| 710-714 | BackUp |
| 720-724 | DriveOnHeading |
| 730-732 | AssistedTeleop |
| 740-741 | Wait |
| 901-999 | Docking / Following |
| 9000-9003 | NavigateToPose |
| 9100-9103 | NavigateThroughPoses |
