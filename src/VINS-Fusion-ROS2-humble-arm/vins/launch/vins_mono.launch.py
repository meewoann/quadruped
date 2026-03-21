from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    config = LaunchConfiguration("config")

    declare_config = DeclareLaunchArgument(
        "config",
        default_value="/home/meewoan/quad_ws/src/VINS-Fusion-ROS2-humble-arm/config/euroc/euroc_mono_imu_config.yaml",
        description="Path to VINS config yaml",
    )

    vins_node = Node(
        package="vins",
        executable="vins_node",
        name="vins_estimator",
        output="screen",
        parameters=[{"use_sim_time": True}],
        arguments=[config],
    )

    loop_fusion_node = Node(
        package="loop_fusion",
        executable="loop_fusion_node",
        name="loop_fusion",
        output="screen",
        parameters=[{"use_sim_time": True}],
        arguments=[config],
    )

    return LaunchDescription([
        declare_config,
        vins_node,
        loop_fusion_node,
    ])
