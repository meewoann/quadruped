# Nav2 (Navigation2) Architecture Reference

## Overview

Nav2 is the ROS2 navigation stack located at `~/nav2_ws/src/navigation2/`. Version 1.4.0.

## System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    nav2_bringup (orchestration)                 │
│  bringup_launch.py → navigation_launch.py + localization_..py  │
└─────────────────────────────────────────────────────────────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        │                     │                     │
┌───────▼────────┐  ┌────────▼──────────┐  ┌──────▼────────┐
│ Localization   │  │   Navigation      │  │   Optional    │
├────────────────┤  ├───────────────────┤  ├───────────────┤
│ • Map Server   │  │ • BT Navigator    │  │ • Docking     │
│ • AMCL         │  │ • Controller Srv  │  │ • Following   │
│ • TF Buffer    │  │ • Planner Srv     │  │ • Collision   │
│                │  │ • Smoother Srv    │  │   Monitor     │
└────────────────┘  │ • Behavior Srv    │  │ • Waypoint    │
                    │ • Route Server    │  │   Follower    │
                    │ • Vel. Smoother   │  │ • Keepout/    │
                    │                   │  │   Speed Zones │
                    └───────────────────┘  └───────────────┘
```

## Package Map (38 packages)

### Core & Interfaces
| Package | Purpose |
|---------|---------|
| `nav2_core` | Abstract plugin base classes (Controller, GlobalPlanner, Smoother, GoalChecker, ProgressChecker, Behavior, etc.) |
| `nav2_msgs` | 30 messages, 18 services, 16 actions |
| `nav2_util` | Geometry utils, costmap helpers, line iterator, smoother utils, parameter handler |
| `nav2_ros_common` | LifecycleNode, SimpleActionServer, ServiceClient, QoS profiles |
| `nav2_common` | CMake/build utilities |

### Servers (Action/Service servers)
| Package | Purpose |
|---------|---------|
| `nav2_bt_navigator` | BT-based navigation orchestration (NavigateToPose, NavigateThroughPoses) |
| `nav2_controller` | FollowPath action server, hosts controller plugins |
| `nav2_planner` | ComputePathToPose/ThroughPoses action server, hosts planner plugins |
| `nav2_smoother` | SmoothPath action server, hosts smoother plugins |
| `nav2_behaviors` | Spin/BackUp/Wait/DriveOnHeading/AssistedTeleop action servers |
| `nav2_route` | Graph-based route planning (ComputeRoute, ComputeAndTrackRoute) |
| `nav2_lifecycle_manager` | Manages lifecycle transitions of all Nav2 nodes |
| `nav2_waypoint_follower` | Sequential waypoint following (FollowWaypoints, FollowGPSWaypoints) |
| `nav2_docking` | Autonomous docking/undocking (DockRobot, UndockRobot) |
| `nav2_following` | Object following (FollowObject) |
| `nav2_collision_monitor` | Real-time collision avoidance layer |
| `nav2_velocity_smoother` | Acceleration-limited velocity smoothing |

### Controller Plugins
| Package | Algorithm | Best For |
|---------|-----------|----------|
| `nav2_dwb_controller` | Dynamic Window + 13 scoring critics | General purpose |
| `nav2_mppi_controller` | Model Predictive Path Integral + 11 critics | Complex/smooth paths |
| `nav2_regulated_pure_pursuit_controller` | Pure pursuit with velocity regulation | Basic tracking |
| `nav2_graceful_controller` | Ego-polar smooth control law | Tight spaces |
| `nav2_rotation_shim_controller` | Rotation middleware wrapper | Enhanced rotation before forward motion |

### Planner Plugins
| Package | Algorithm | Best For |
|---------|-----------|----------|
| `nav2_navfn_planner` | Dijkstra/A* wavefront | General (circular robots) |
| `nav2_smac_planner` | 2D A*, Hybrid-A* (SE2), State Lattice | Non-circular, kinematic constraints |
| `nav2_theta_star_planner` | Lazy Theta* (any-angle) | Smooth any-angle paths |

### Costmap & Perception
| Package | Purpose |
|---------|---------|
| `nav2_costmap_2d` | Layered costmap (static, obstacle, inflation, voxel, range, denoise, filters) |
| `nav2_voxel_grid` | 3D voxel grid for obstacle layer |
| `nav2_map_server` | Static map serving and saving |
| `nav2_amcl` | Particle filter localization |

### BT & Behaviors
| Package | Purpose |
|---------|---------|
| `nav2_behavior_tree` | 82 BT node plugins (47 actions, 22 conditions, 6 controls, 7 decorators) |
| `nav2_behaviors` | 5 recovery behavior plugins |

### Simulation & Tools
| Package | Purpose |
|---------|---------|
| `nav2_bringup` | Launch files, configs, simulation setups |
| `nav2_loopback_sim` | Lightweight sim without physics engine |
| `nav2_simple_commander` | Python API (BasicNavigator) |
| `nav2_rviz_plugins` | RViz visualization plugins |
| `nav2_system_tests` | System-level tests |

## Key Design Patterns

1. **Lifecycle Management**: All servers inherit `nav2::LifecycleNode` (configure/activate/deactivate/cleanup)
2. **Plugin Architecture**: pluginlib-based dynamic loading for controllers, planners, smoothers, behaviors, costmap layers
3. **Action Servers**: Primary interface for long-running tasks (planning, control, behaviors)
4. **Behavior Trees**: Orchestration layer composing actions/conditions/recovery via BT XML files
5. **Costmap Layers**: Composable perception pipeline (static + obstacle + inflation + filters)
6. **Bond-based Health**: `nav2_lifecycle_manager` monitors node health via bonds

## Nav2 Action Flow

```
User Goal → BT Navigator
  ├── ComputePathToPose (Planner Server)
  │     └── GlobalPlanner plugin (NavFn, SMAC, Theta*)
  ├── SmoothPath (Smoother Server) [optional]
  │     └── Smoother plugin (Simple, Savitzky-Golay, Constrained)
  ├── FollowPath (Controller Server)
  │     └── Controller plugin (DWB, MPPI, RPP, Graceful)
  │           ├── GoalChecker plugin
  │           ├── ProgressChecker plugin
  │           └── PathHandler plugin
  └── Recovery (Behavior Server) [on failure]
        └── Spin / BackUp / Wait / DriveOnHeading / AssistedTeleop
```

## Source Code Location

All packages: `~/nav2_ws/src/navigation2/<package_name>/`
