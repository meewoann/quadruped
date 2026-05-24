// Tách capture ra thread riêng, dùng double-buffer
#include <atomic>
#include <thread>
#include <mutex>

class CameraReaderNode : public rclcpp::Node {
public:
  CameraReaderNode() : Node("camera_reader"), running_(true) {
    // ... params như cũ ...

    // QoS: BEST_EFFORT + depth nhỏ hơn cho low latency
    auto qos = rclcpp::QoS(2).best_effort();
    publisher_ = this->create_publisher<sensor_msgs::msg::Image>("camera_raw", qos);

    cap_.open(video_device);
    cap_.set(cv::CAP_PROP_FRAME_WIDTH, frame_width);
    cap_.set(cv::CAP_PROP_FRAME_HEIGHT, frame_height);
    cap_.set(cv::CAP_PROP_FPS, frame_rate);
    // Giảm internal buffer của V4L2 driver xuống 1
    cap_.set(cv::CAP_PROP_BUFFERSIZE, 1);

    // Capture thread riêng
    capture_thread_ = std::thread(&CameraReaderNode::capture_loop, this);

    // Timer chỉ lo publish
    timer_ = this->create_wall_timer(
      std::chrono::milliseconds(1000 / frame_rate),
      std::bind(&CameraReaderNode::publish_callback, this));
  }

  ~CameraReaderNode() {
    running_ = false;
    if (capture_thread_.joinable()) capture_thread_.join();
  }

private:
  void capture_loop() {
    while (running_) {
      cv::Mat frame;
      cap_ >> frame;  // blocking, nhưng trên thread riêng
      if (!frame.empty()) {
        std::lock_guard<std::mutex> lock(mutex_);
        latest_frame_ = frame.clone();
      }
    }
  }

  void publish_callback() {
    cv::Mat frame;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (latest_frame_.empty()) return;
      frame = latest_frame_;  // shallow copy, fast
    }
    std_msgs::msg::Header header;
    header.stamp = this->now();
    header.frame_id = "camera_link";
    publisher_->publish(*cv_bridge::CvImage(header, "bgr8", frame).toImageMsg());
  }

  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr publisher_;
  rclcpp::TimerBase::SharedPtr timer_;
  cv::VideoCapture cap_;
  cv::Mat latest_frame_;
  std::mutex mutex_;
  std::thread capture_thread_;
  std::atomic<bool> running_;
};
