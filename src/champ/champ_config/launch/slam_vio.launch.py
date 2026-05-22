import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    this_package = FindPackageShare('champ_config')

    slam_params_file = PathJoinSubstitution(
        [this_package, 'config/autonomy', 'slam.yaml']
    )

    slam_launch_path = PathJoinSubstitution(
        [FindPackageShare('champ_navigation'), 'launch', 'slam.launch.py']
    )

    vins_launch_path = PathJoinSubstitution(
        [FindPackageShare('vins'), 'launch', 'vins_mono.launch.py']
    )

    return LaunchDescription([
        DeclareLaunchArgument(
            name='slam_params_file',
            default_value=slam_params_file,
            description='slam_toolbox params file'
        ),

        DeclareLaunchArgument(
            name='vins_config',
            default_value='/home/meewoan/quad_ws/src/VINS-Fusion-ROS2-humble-arm/config/champ_gazebo/champ_gazebo_mono_imu.yaml',
            description='Path to VINS config yaml'
        ),

        # Launch VINS-Fusion for VIO odometry (odom -> base_footprint TF)
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(vins_launch_path),
            launch_arguments={
                'config': LaunchConfiguration('vins_config'),
            }.items()
        ),

        # Launch slam_toolbox (uses odom -> base_footprint TF from VINS)
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(slam_launch_path),
            launch_arguments={
                'slam_params_file': LaunchConfiguration('slam_params_file'),
                'sim': 'true',
                'rviz': 'false',
            }.items()
        ),
    ])
