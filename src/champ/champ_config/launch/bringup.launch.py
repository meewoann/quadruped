# Copyright (c) 2021 Juan Miguel Jimeno
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http:#www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.conditions import IfCondition, UnlessCondition
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    this_package = FindPackageShare('champ_config')
    joints_config = PathJoinSubstitution(
        [this_package, 'config', 'joints', 'joints.yaml']
    )
    gait_config = PathJoinSubstitution(
        [this_package, 'config', 'gait', 'gait.yaml']
    )
    links_config = PathJoinSubstitution(
        [this_package, 'config', 'links', 'links.yaml']
    )
    bringup_launch_path = PathJoinSubstitution(
        [FindPackageShare('champ_bringup'), 'launch', 'bringup.launch.py']
    )
    rplidar_launch_path = PathJoinSubstitution(
        [FindPackageShare('champ_bringup'), 'launch', 'include', 'laser', 'rplidar_ros2.launch.py']
    )

    sim = LaunchConfiguration('sim')

    return LaunchDescription([
        DeclareLaunchArgument(
            name='robot_name',
            default_value='',
            description='Set robot name for multi robot'
        ),

        DeclareLaunchArgument(
            name='sim',
            default_value='false',
            description='true = Gazebo simulation, false = real robot'
        ),

        DeclareLaunchArgument(
            name='rviz',
            default_value='false',
            description='Run rviz'
        ),

        DeclareLaunchArgument(
            name='hardware_connected',
            default_value='false',
            description='Set to true if connected to a physical robot'
        ),

        # Core CHAMP bringup (gait controller, state estimation, EKF)
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(bringup_launch_path),
            launch_arguments={
                'use_sim_time': sim,
                'robot_name': LaunchConfiguration('robot_name'),
                'gazebo': sim,
                'rviz': LaunchConfiguration('rviz'),
                'hardware_connected': LaunchConfiguration('hardware_connected'),
                'orientation_from_imu': 'true',
                'publish_foot_contacts': 'true',
                'close_loop_odom': 'true',
                'joint_controller_topic': 'joint_group_effort_controller/joint_trajectory',
                'joints_map_path': joints_config,
                'links_map_path': links_config,
                'gait_config_path': gait_config,
            }.items(),
        ),

        # --- Real robot only nodes (sim:=false) ---

        # Serial bridge: sends servo commands to Arduino, receives IMU data → imu/raw
        Node(
            package='champ_base',
            executable='servo_pwm_converter_node',
            name='servo_pwm_converter_node',
            output='screen',
            condition=UnlessCondition(sim),
        ),

        # Converts imu/raw (champ_msgs/Imu) → imu/data (sensor_msgs/Imu) with EMA filter
        Node(
            package='champ_base',
            executable='message_relay_node',
            name='message_relay_node',
            output='screen',
            parameters=[
                {'has_imu': True},
                joints_config,
            ],
            condition=UnlessCondition(sim),
        ),

        # RPLIDAR A1M8 → /scan (sensor_msgs/LaserScan)
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(rplidar_launch_path),
            condition=UnlessCondition(sim),
        ),
    ])
