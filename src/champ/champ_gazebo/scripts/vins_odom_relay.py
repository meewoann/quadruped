#!/usr/bin/env python3
"""
vins_odom_relay.py
------------------
Bridges VINS-Fusion odometry output into the standard ROS 2 nav stack frame.

VINS publishes:
  /odometry   (frame_id = "world")   — body pose in world frame

This node publishes:
  TF:  odom  →  base_footprint    (dynamic, from VINS pose)
  /odom                      (nav_msgs/Odometry, remapped from VINS)

This makes VINS the sole source of odom→base_footprint, replacing CHAMP's EKF.

The CHAMP EKF (footprint_to_odom_ekf) must be disabled via:
    ros2 launch champ_config gazebo.launch.py use_vio_odom:=true
"""

import rclpy
from rclpy.node import Node
from nav_msgs.msg import Odometry
from geometry_msgs.msg import TransformStamped
import tf2_ros


class VinsOdomRelay(Node):
    def __init__(self):
        super().__init__('vins_odom_relay')

        self._tf_broadcaster = tf2_ros.TransformBroadcaster(self)

        # Re-publish VINS odometry as /odom (used by nav stack, RViz, slam_toolbox)
        self._odom_pub = self.create_publisher(Odometry, '/odom', 10)

        self.create_subscription(
            Odometry,
            '/odometry',
            self._odom_cb,
            10
        )

        self.get_logger().info(
            'vins_odom_relay started: /odometry → TF odom→base_footprint + /odom'
        )

    def _odom_cb(self, msg: Odometry):
        # ── 1. Broadcast TF: odom → base_footprint ─────────────────────────
        t = TransformStamped()
        t.header.stamp = msg.header.stamp
        t.header.frame_id = 'odom'
        t.child_frame_id = 'base_footprint'   # NOT base_link — robot_state_publisher owns base_footprint→base_link

        t.transform.translation.x = msg.pose.pose.position.x
        t.transform.translation.y = msg.pose.pose.position.y
        t.transform.translation.z = msg.pose.pose.position.z
        t.transform.rotation = msg.pose.pose.orientation

        self._tf_broadcaster.sendTransform(t)

        # ── 2. Re-publish as /odom with corrected frame IDs ─────────────────
        out = Odometry()
        out.header.stamp = msg.header.stamp
        out.header.frame_id = 'odom'
        out.child_frame_id = 'base_footprint'
        out.pose = msg.pose
        out.twist = msg.twist
        self._odom_pub.publish(out)


def main():
    rclpy.init()
    node = VinsOdomRelay()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
