# Quadruped Leg Angle Mapping for Hardware Input

## Overview
The quadruped controller uses a 12-element array `target_joint_positions[12]` where each leg has 3 joint angles.

## Leg Indexing

The four legs are indexed as follows in the `QuadrupedBase` class:

| Index | Abbreviation | Full Name | Position |
|-------|--------------|-----------|----------|
| 0 | `lf` | Left Front | Front-left |
| 1 | `rf` | Right Front | Front-right |
| 2 | `lh` | Left Hind | Back-left |
| 3 | `rh` | Right Hind | Back-right |

## Joint Angle Array Structure

Each leg has 3 joints in this order: **Hip → Upper Leg → Lower Leg**

```
target_joint_positions[12] = {
    // Leg 0 (Left Front)
    [0] = Hip angle
    [1] = Upper leg angle
    [2] = Lower leg angle
    
    // Leg 1 (Right Front)
    [3] = Hip angle
    [4] = Upper leg angle
    [5] = Lower leg angle
    
    // Leg 2 (Left Hind)
    [6] = Hip angle
    [7] = Upper leg angle
    [8] = Lower leg angle
    
    // Leg 3 (Right Hind)
    [9] = Hip angle
    [10] = Upper leg angle
    [11] = Lower leg angle
}
```

## Formula to Access Angle Per Leg

For any leg `i` (0-3), use this formula:

```cpp
// Hip joint for leg i
float hip_angle = target_joint_positions[i * 3 + 0];

// Upper leg joint for leg i
float upper_leg_angle = target_joint_positions[i * 3 + 1];

// Lower leg joint for leg i
float lower_leg_angle = target_joint_positions[i * 3 + 2];
```

## Example: Accessing All Angles

```cpp
void printAllLegAngles(float target_joint_positions[12])
{
    const char* leg_names[] = {"Left Front", "Right Front", "Left Hind", "Right Hind"};
    const char* joint_names[] = {"Hip", "Upper Leg", "Lower Leg"};
    
    for(int leg = 0; leg < 4; leg++)
    {
        printf("%s Leg:\n", leg_names[leg]);
        for(int joint = 0; joint < 3; joint++)
        {
            int index = leg * 3 + joint;
            printf("  %s: %.4f rad (%.2f°)\n", 
                   joint_names[joint],
                   target_joint_positions[index],
                   target_joint_positions[index] * 180.0 / M_PI);
        }
    }
}
```

## Where Angles Come From

In `quadruped_controller.cpp` → `controlLoop_()`:

1. **Body & Leg Controllers** generate target foot positions
2. **Inverse Kinematics** converts foot positions → joint angles
   ```cpp
   kinematics_.inverse(target_joint_positions, target_foot_positions);
   ```
3. **Angles are published** via `publishJoints_(target_joint_positions)`

## Publishing to Hardware

The angles are currently published to:

1. **Joint Trajectory Message** (for ROS2 control):
   - Topic: `joint_group_position_controller/command`
   - Message type: `trajectory_msgs::msg::JointTrajectory`
   - Contains: Joint names in order, Position values matching the 12-element array

2. **Joint States** (optional):
   - Topic: `joint_states`
   - Only published when `publish_joint_states_=true` and not in Gazebo

## To Modify for Hardware Input

### Option 1: In the Control Loop
Modify the `publishJoints_()` call in `controlLoop_()` to format angles for your hardware:

```cpp
void QuadrupedController::controlLoop_()
{
    float target_joint_positions[12];
    // ... inverse kinematics computation ...
    
    // Send to hardware
    sendToHardware(target_joint_positions);  // Your function here
    
    publishJoints_(target_joint_positions);
}
```

### Option 2: Create a Helper Function
```cpp
struct LegAngles {
    float hip;
    float upper_leg;
    float lower_leg;
};

LegAngles getLegAngles(float joint_positions[12], int leg_index)
{
    return {
        joint_positions[leg_index * 3 + 0],
        joint_positions[leg_index * 3 + 1],
        joint_positions[leg_index * 3 + 2]
    };
}
```

### Option 3: Send Per-Leg to Custom Hardware
```cpp
void sendToHardware(float target_joint_positions[12])
{
    for(int leg = 0; leg < 4; leg++)
    {
        // Get angles for this leg
        float hip = target_joint_positions[leg * 3 + 0];
        float upper = target_joint_positions[leg * 3 + 1];
        float lower = target_joint_positions[leg * 3 + 2];
        
        // Send to servo controller for this leg
        hardware_interface_->setLegAngles(leg, hip, upper, lower);
    }
}
```

## Related Files

- **Controller Logic**: [src/champ/champ_base/src/quadruped_controller.cpp](src/champ/champ_base/src/quadruped_controller.cpp)
- **Header**: [src/champ/champ_base/include/quadruped_controller.h](src/champ/champ_base/include/quadruped_controller.h)
- **Kinematics**: `src/champ/champ/include/champ/kinematics/kinematics.h`
- **Quadruped Base**: `src/champ/champ/include/champ/quadruped_base/quadruped_base.h`
