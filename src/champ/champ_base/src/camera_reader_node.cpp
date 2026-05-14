#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>

class CameraReaderNode : public rclcpp::Node
{
public:
    CameraReaderNode() : Node("camera_reader")
    {
        // Declare and get parameters
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

        cap_.open(video_device);
        if (!cap_.isOpened()) {
            RCLCPP_ERROR(this->get_logger(), "Failed to open video device: %s", video_device.c_str());
            return;
        }

        cap_.set(cv::CAP_PROP_FRAME_WIDTH, frame_width);
        cap_.set(cv::CAP_PROP_FRAME_HEIGHT, frame_height);
        cap_.set(cv::CAP_PROP_FPS, frame_rate);

        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(1000 / frame_rate),
            std::bind(&CameraReaderNode::timer_callback, this));

        RCLCPP_INFO(this->get_logger(), "Camera reader node started, publishing to camera_raw");
    }

private:
    void timer_callback()
    {
        cv::Mat frame;
        cap_ >> frame;

        if (!frame.empty()) {
            std_msgs::msg::Header header;
            header.stamp = this->now();
            header.frame_id = "camera_link";
            
            sensor_msgs::msg::Image::SharedPtr msg = cv_bridge::CvImage(header, "bgr8", frame).toImageMsg();
            publisher_->publish(*msg);
        } else {
            RCLCPP_WARN(this->get_logger(), "Empty frame received");
        }
    }

    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr publisher_;
    rclcpp::TimerBase::SharedPtr timer_;
    cv::VideoCapture cap_;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<CameraReaderNode>());
    rclcpp::shutdown();
    return 0;
}
