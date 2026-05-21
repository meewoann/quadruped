#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>
#include <thread>
#include <atomic>
#include <mutex>

class CameraReaderNode : public rclcpp::Node
{
public:
    CameraReaderNode() : Node("camera_reader"), running_(false)
    {
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

        publisher_ = this->create_publisher<sensor_msgs::msg::Image>("camera_raw", 10);

        cap_.open(video_device, cv::CAP_V4L2);  // explicit backend
        if (!cap_.isOpened()) {
            RCLCPP_ERROR(this->get_logger(), "Failed to open: %s", video_device.c_str());
            return;
        }

        cap_.set(cv::CAP_PROP_FRAME_WIDTH, frame_width);
        cap_.set(cv::CAP_PROP_FRAME_HEIGHT, frame_height);
        cap_.set(cv::CAP_PROP_FPS, frame_rate);

        // KEY FIX: giảm buffer xuống 1 để luôn lấy frame mới nhất
        cap_.set(cv::CAP_PROP_BUFFERSIZE, 1);

        frame_interval_ms_ = 1000.0 / frame_rate;
        running_ = true;

        // Capture thread riêng, không block ROS executor
        capture_thread_ = std::thread(&CameraReaderNode::capture_loop, this);

        // Timer chỉ lo publish, không capture
        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(static_cast<int>(frame_interval_ms_)),
            std::bind(&CameraReaderNode::publish_callback, this));

        RCLCPP_INFO(this->get_logger(), "Camera node started");
    }

    ~CameraReaderNode()
    {
        running_ = false;
        if (capture_thread_.joinable())
            capture_thread_.join();
        cap_.release();
    }

private:
    // Thread này chạy liên tục, grab frame nhanh nhất có thể
    void capture_loop()
    {
        while (running_) {
            cv::Mat frame;
            cap_ >> frame;
            if (!frame.empty()) {
                std::lock_guard<std::mutex> lock(frame_mutex_);
                latest_frame_ = frame.clone();
                latest_stamp_ = this->now();
            }
        }
    }

    // Timer callback chỉ lấy frame mới nhất và publish
    void publish_callback()
    {
        cv::Mat frame;
        rclcpp::Time stamp;
        {
            std::lock_guard<std::mutex> lock(frame_mutex_);
            if (latest_frame_.empty()) return;
            frame = latest_frame_;   // shallow copy, fast
            stamp = latest_stamp_;
        }

        std_msgs::msg::Header header;
        header.stamp = stamp;
        header.frame_id = "camera_link";
        auto msg = cv_bridge::CvImage(header, "bgr8", frame).toImageMsg();
        publisher_->publish(*msg);
    }

    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr publisher_;
    rclcpp::TimerBase::SharedPtr timer_;
    cv::VideoCapture cap_;

    std::thread capture_thread_;
    std::atomic<bool> running_;
    std::mutex frame_mutex_;
    cv::Mat latest_frame_;
    rclcpp::Time latest_stamp_;
    double frame_interval_ms_;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<CameraReaderNode>());
    rclcpp::shutdown();
    return 0;
}
