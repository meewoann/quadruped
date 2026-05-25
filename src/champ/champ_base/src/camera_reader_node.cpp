#include <atomic>
#include <mutex>
#include <thread>

#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <std_msgs/msg/header.hpp>

class CameraReaderNode : public rclcpp::Node {
public:
  CameraReaderNode() : Node("camera_reader"), running_(true), has_new_frame_(false) {
    this->declare_parameter<std::string>("video_device", "/dev/video0");
    this->declare_parameter<int>("frame_rate", 30);
    this->declare_parameter<int>("frame_width", 640);
    this->declare_parameter<int>("frame_height", 480);

    std::string video_device;
    int frame_rate, frame_width, frame_height;
    this->get_parameter("video_device", video_device);
    this->get_parameter("frame_rate", frame_rate);
    this->get_parameter("frame_width", frame_width);
    this->get_parameter("frame_height", frame_height);

    publisher_ = this->create_publisher<sensor_msgs::msg::Image>("camera_raw", rclcpp::QoS(10));

    cap_.open(video_device, cv::CAP_V4L2);
    if (!cap_.isOpened()) {
      RCLCPP_ERROR(this->get_logger(), "Failed to open video device: %s", video_device.c_str());
      return;
    }

    // MJPG phải set trước width/height/fps
    cap_.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
    cap_.set(cv::CAP_PROP_FRAME_WIDTH, frame_width);
    cap_.set(cv::CAP_PROP_FRAME_HEIGHT, frame_height);
    cap_.set(cv::CAP_PROP_FPS, frame_rate);
    cap_.set(cv::CAP_PROP_BUFFERSIZE, 1);

    // Log format thực tế để verify
    double fourcc = cap_.get(cv::CAP_PROP_FOURCC);
    char fourcc_str[5];
    fourcc_str[0] = (char)(((int)fourcc) & 0XFF);
    fourcc_str[1] = (char)(((int)fourcc >> 8) & 0XFF);
    fourcc_str[2] = (char)(((int)fourcc >> 16) & 0XFF);
    fourcc_str[3] = (char)(((int)fourcc >> 24) & 0XFF);
    fourcc_str[4] = '\0';
    RCLCPP_INFO(this->get_logger(), "Camera format: %s, %dx%d @ %.0ffps",
      fourcc_str,
      (int)cap_.get(cv::CAP_PROP_FRAME_WIDTH),
      (int)cap_.get(cv::CAP_PROP_FRAME_HEIGHT),
      cap_.get(cv::CAP_PROP_FPS));

    capture_thread_ = std::thread(&CameraReaderNode::capture_loop, this);

    timer_ = this->create_wall_timer(
      std::chrono::milliseconds(1000 / frame_rate),
      std::bind(&CameraReaderNode::publish_callback, this));

    RCLCPP_INFO(this->get_logger(), "Camera reader node started, publishing to camera_raw");
  }

  ~CameraReaderNode() {
    running_ = false;
    if (capture_thread_.joinable()) capture_thread_.join();
  }

private:
  void capture_loop() {
    while (running_) {
      cv::Mat frame;
      cap_ >> frame;
      if (!frame.empty()) {
        std::lock_guard<std::mutex> lock(mutex_);
        cv::swap(latest_frame_, frame);
        has_new_frame_ = true;
      }
    }
  }

  void publish_callback() {
    cv::Mat frame;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (!has_new_frame_) return;
      cv::swap(frame, latest_frame_);
      has_new_frame_ = false;
    }
    std_msgs::msg::Header header;
    header.stamp = this->now();
    header.frame_id = "camera_link";
    auto msg = cv_bridge::CvImage(header, "bgr8", frame).toImageMsg();
    publisher_->publish(std::move(*msg));
  }

  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr publisher_;
  rclcpp::TimerBase::SharedPtr timer_;
  cv::VideoCapture cap_;
  cv::Mat latest_frame_;
  std::mutex mutex_;
  std::thread capture_thread_;
  std::atomic<bool> running_;
  bool has_new_frame_;
};

int main(int argc, char *argv[]) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<CameraReaderNode>());
  rclcpp::shutdown();
  return 0;
}
