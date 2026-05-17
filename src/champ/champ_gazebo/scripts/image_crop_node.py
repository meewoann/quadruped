#!/usr/bin/env python3
"""
Image crop node — removes a white-line artifact from the top of the Gazebo
camera frame before sending the image to VIO (VINS).

Subscribes:  /camera/image_raw        sensor_msgs/Image      (640x480)
             /camera/camera_info_raw sensor_msgs/CameraInfo (640x480)
Publishes:   /camera/image_cropped    sensor_msgs/Image      (640x465)
             /camera/camera_info     sensor_msgs/CameraInfo (640x465)

Crop: TOP_ROWS rows removed from the top.
After cropping, update cam0_pinhole.yaml:
  image_height: 465
  cy:           225.0   (= 240.0 - 15)
"""

import rclpy
from rclpy.node import Node
from sensor_msgs.msg import CameraInfo, Image

TOP_ROWS = 15   # pixels to remove from the top


class ImageCropNode(Node):

    def __init__(self):
        super().__init__('image_crop_node')
        self._image_pub = self.create_publisher(Image, '/camera/image_cropped', 10)
        self._info_pub = self.create_publisher(CameraInfo, '/camera/camera_info', 10)
        self.create_subscription(Image, '/camera/image_raw', self._image_cb, 10)
        self.create_subscription(CameraInfo, '/camera/camera_info_raw', self._info_cb, 10)
        self.get_logger().info(
            f'Image crop node started — removing top {TOP_ROWS} rows and updating camera_info')

    def _image_cb(self, msg: Image):
        # Each row = width * bytes_per_pixel
        bytes_per_pixel = len(msg.data) // (msg.height * msg.width)
        row_bytes = msg.width * bytes_per_pixel
        offset = TOP_ROWS * row_bytes

        out = Image()
        out.header = msg.header
        out.encoding = msg.encoding
        out.width = msg.width
        out.height = msg.height - TOP_ROWS
        out.step = msg.step
        out.is_bigendian = msg.is_bigendian
        out.data = msg.data[offset:]

        self._image_pub.publish(out)

    def _info_cb(self, msg: CameraInfo):
        out = CameraInfo()
        out.header = msg.header
        out.height = msg.height - TOP_ROWS
        out.width = msg.width
        out.distortion_model = msg.distortion_model
        out.d = list(msg.d)
        out.k = list(msg.k)
        out.r = list(msg.r)
        out.p = list(msg.p)
        out.binning_x = msg.binning_x
        out.binning_y = msg.binning_y
        out.roi = msg.roi

        out.k[5] = msg.k[5] - TOP_ROWS
        out.p[6] = msg.p[6] - TOP_ROWS

        # The output image is a new, already-cropped image. Its full extent is
        # valid in the cropped coordinate system, so no ROI offset remains.
        out.roi.x_offset = 0
        out.roi.y_offset = 0
        out.roi.width = out.width
        out.roi.height = out.height
        out.roi.do_rectify = msg.roi.do_rectify

        self._info_pub.publish(out)


def main():
    rclpy.init()
    node = ImageCropNode()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
