#include <memory>
#include <cmath>
#include <vector>
#include <algorithm>
#include <string>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/float32_multi_array.hpp"
#include "champ_msgs/msg/joints.hpp"

using std::placeholders::_1;

class ServoPwmConverter : public rclcpp::Node
{
public:
  ServoPwmConverter()
  : Node("servo_pwm_converter_node")
  {
    // Parameter
    this->declare_parameter<bool>("add_center_offset", true);
    add_center_offset_ = this->get_parameter("add_center_offset").as_bool();

    angles_pub_ = this->create_publisher<std_msgs::msg::Float32MultiArray>("/servo_angles", 10);

    joints_sub_ = this->create_subscription<champ_msgs::msg::Joints>(
      "/joints_debug", 10,
      std::bind(&ServoPwmConverter::jointsCallback, this, _1));

    // 🔥 MỞ SERIAL
    initSerial("/dev/ttyUSB0", B115200);

    RCLCPP_INFO(this->get_logger(),
      "servo_pwm_converter_node started (Serial: /dev/ttyUSB0 @115200)");
  }

  ~ServoPwmConverter()
  {
    if (serial_fd_ != -1) {
      close(serial_fd_);
    }
  }

private:
  int serial_fd_ = -1;

  // ================= SERIAL SETUP =================
  void initSerial(const std::string &port, int baudrate)
  {
    serial_fd_ = open(port.c_str(), O_RDWR | O_NOCTTY | O_SYNC);

    if (serial_fd_ < 0) {
      RCLCPP_ERROR(this->get_logger(), "Cannot open serial port %s", port.c_str());
      return;
    }

    struct termios tty;
    memset(&tty, 0, sizeof tty);

    if (tcgetattr(serial_fd_, &tty) != 0) {
      RCLCPP_ERROR(this->get_logger(), "Error from tcgetattr");
      return;
    }

    cfsetospeed(&tty, baudrate);
    cfsetispeed(&tty, baudrate);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_iflag &= ~IGNBRK;
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 5;

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr(serial_fd_, TCSANOW, &tty) != 0) {
      RCLCPP_ERROR(this->get_logger(), "Error from tcsetattr");
    }
  }

  // ================= CALLBACK =================
  void jointsCallback(const champ_msgs::msg::Joints::SharedPtr msg)
  {
    const size_t expected = 12;
    std::vector<float> angles_rad(expected, 0.0f);

    if (msg->position.size() < expected) {
      RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 5000,
        "Received %zu elements (expected 12)", msg->position.size());
    }

    for (size_t i = 0; i < expected; ++i) {
      if (i < msg->position.size()) angles_rad[i] = msg->position[i];
    }

    constexpr double RAD2DEG = 180.0 / M_PI;

    std_msgs::msg::Float32MultiArray out_msg;
    out_msg.data.reserve(expected);

    for (size_t i = 0; i < expected; ++i) {
      double deg = angles_rad[i] * RAD2DEG;
      if (add_center_offset_) deg += 90.0;

      deg = std::clamp(deg, 0.0, 180.0);
      out_msg.data.push_back(static_cast<float>(deg));
    }

    angles_pub_->publish(out_msg);

    // ================= GỬI SERIAL =================
    if (serial_fd_ != -1 && !out_msg.data.empty()) {
      int value = static_cast<int>(out_msg.data[0]);  // lấy phần tử đầu tiên

      std::string data = std::to_string(value) + "\n";
      ssize_t written = write(serial_fd_, data.c_str(), data.size());
      if (written < 0) {
        RCLCPP_ERROR(this->get_logger(), "Failed to write to serial: %s", std::strerror(errno));
        // Optionally close serial to attempt reopen later
        close(serial_fd_);
        serial_fd_ = -1;
      } else {
        RCLCPP_INFO(this->get_logger(), "Sent serial data: '%s' (%zd bytes) angle=%d",
          data.c_str(), written, value);
      }
    }
  }

  rclcpp::Publisher<std_msgs::msg::Float32MultiArray>::SharedPtr angles_pub_;
  rclcpp::Subscription<champ_msgs::msg::Joints>::SharedPtr joints_sub_;
  bool add_center_offset_ = true;
};

// ================= MAIN =================
int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<ServoPwmConverter>());
  rclcpp::shutdown();
  return 0;
}