#!/usr/bin/env python3
"""
vins_mono.launch.py — VINS-Fusion monocular+IMU for CHAMP quadruped (Gazebo).

TF strategy (single odom frame):
  VINS publishes /odometry (frame_id = "world")
  vins_odom_relay converts this into:
    TF:   odom  →  base_footprint   (dynamic, from VINS pose)
    /odom (nav_msgs/Odometry)

  CHAMP's footprint_to_odom_ekf is disabled by launching Gazebo with:
    ros2 launch champ_config gazebo.launch.py use_vio_odom:=true

Result: single, clean  odom → base_footprint → base_link  TF from VINS + robot_state_publisher.
RViz Fixed Frame: odom

Usage:
    ros2 launch vins vins_mono.launch.py
    ros2 launch vins vins_mono.launch.py config:=/path/to/custom.yaml
"""

import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():

    default_config = os.path.join(
        '/home/meewoan/quad_ws/src/vins_fusion',
        'config', 'champ_gazebo', 'champ_gazebo_mono_imu.yaml'
    )

    declare_config = DeclareLaunchArgument(
        name='config',
        default_value=default_config,
        description='Absolute path to the VINS-Fusion YAML config file'
    )

    # -----------------------------------------------------------------------
    # vins_estimator node
    # Subscribes (from YAML):
    #   /imu/data_raw           sensor_msgs/Imu     (200 Hz, raw Gazebo)
    #   /camera/image_cropped   sensor_msgs/Image   (640x465, ~20–30 Hz)
    #
    # Publishes:
    #   /odometry                    (frame_id = "world")
    #   /vins_estimator/path         (frame_id = "world")
    #   /vins_estimator/imu_propagate
    #   /vins_estimator/image_track
    # -----------------------------------------------------------------------
    vins_node = Node(
        package='vins',
        executable='vins_node',
        name='vins_estimator',
        output='screen',
        arguments=[LaunchConfiguration('config')],
        parameters=[{'use_sim_time': True}],
    )

    # -----------------------------------------------------------------------
    # Static TF: world → odom  (identity)
    #
    # VINS visualization.cpp hardcodes frame_id = "world" on all published
    # messages (path, odometry, point clouds, camera poses).
    # RViz Fixed Frame = "odom".
    # Without this bridge, RViz cannot resolve "world"-stamped messages and
    # the green path / point clouds never appear.
    # -----------------------------------------------------------------------
    static_tf_world_odom = Node(
        package='tf2_ros',
        executable='static_transform_publisher',
        name='static_tf_world_odom',
        output='screen',
        arguments=['0', '0', '0', '0', '0', '0', 'world', 'odom'],
        parameters=[{'use_sim_time': True}],
    )

    # -----------------------------------------------------------------------
    # vins_odom_relay node
    # Converts /odometry → TF odom→base_footprint + /odom topic.
    # This makes VINS the sole odom source, replacing CHAMP's EKF.
    # CHAMP EKF must be disabled: launch Gazebo with use_vio_odom:=true
    # -----------------------------------------------------------------------
    relay_node = Node(
        package='champ_gazebo',
        executable='vins_odom_relay.py',
        name='vins_odom_relay',
        output='screen',
        parameters=[{'use_sim_time': True}],
    )

    return LaunchDescription([
        declare_config,
        vins_node,
        static_tf_world_odom,
        relay_node,
    ])
