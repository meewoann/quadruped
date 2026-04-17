from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

from ament_index_python.packages import get_package_share_directory, PackageNotFoundError
import logging

try:
    get_package_share_directory('rplidar_ros')
    _rplidar_available = True
except PackageNotFoundError:
    _rplidar_available = False
    logging.getLogger('launch').warning(
        '\n[rplidar_ros2.launch.py] rplidar_ros package NOT found — LiDAR will not start.\n'
        '  Install with:  sudo apt install ros-humble-rplidar-ros\n'
    )


def generate_launch_description():
    if not _rplidar_available:
        return LaunchDescription([])

    return LaunchDescription([
        DeclareLaunchArgument(
            'serial_port',
            default_value='/dev/ttyUSB0',
            description='Serial port for RPLIDAR A1M8',
        ),
        Node(
            package='rplidar_ros',
            executable='rplidar_node',
            name='rplidar_node',
            output='screen',
            parameters=[{
                'serial_port': LaunchConfiguration('serial_port'),
                'serial_baudrate': 115200,
                'frame_id': 'hokuyo_frame',
                'inverted': False,
                'angle_compensate': True,
                'scan_mode': 'Standard',
            }],
        ),
    ])
