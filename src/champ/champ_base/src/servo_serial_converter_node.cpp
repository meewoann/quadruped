// serial_joint_sender_node.cpp
#include <champ_msgs/msg/imu.hpp>
#include <champ_msgs/msg/joints.hpp>
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

// Serial
#include <array>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

// Threading
#include <atomic>
#include <sstream>
#include <thread>

class SerialJointSender : public rclcpp::Node {
public:
  SerialJointSender() : Node("serial_joint_sender_node"), running_(false) {
    serial_fd_ = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY | O_SYNC);
    if (serial_fd_ < 0) {
      RCLCPP_ERROR(this->get_logger(), "Cannot open /dev/ttyUSB0");
      return;
    }

    configureSerial_(serial_fd_, B115200);
    RCLCPP_INFO(this->get_logger(), "Opened /dev/ttyUSB0 @ 115200");

    imu_pub_ = this->create_publisher<champ_msgs::msg::Imu>("imu/raw", 10);

    subscription_ = this->create_subscription<champ_msgs::msg::Joints>(
        "joints_debug", 10,
        std::bind(&SerialJointSender::jointsCallback_, this,
                  std::placeholders::_1));

    joint_offset_sub_ = this->create_subscription<std_msgs::msg::String>(
        "/joint_offset_cmd", 10,
        std::bind(&SerialJointSender::jointOffsetCallback_, this,
                  std::placeholders::_1));

    timer_ = this->create_wall_timer(
        std::chrono::milliseconds(100),
        std::bind(&SerialJointSender::timerCallback_, this));

    running_ = true;
    read_thread_ = std::thread(&SerialJointSender::readLoop_, this);
  }

  ~SerialJointSender() {
    running_ = false;
    if (read_thread_.joinable())
      read_thread_.join();
    if (serial_fd_ >= 0)
      close(serial_fd_);
  }

private:
  void jointsCallback_(const champ_msgs::msg::Joints::SharedPtr msg) {
    latest_joints_ = *msg;
    has_data_ = true;
  }

  void jointOffsetCallback_(const std_msgs::msg::String::SharedPtr msg) {
    // Expected format: "<LEG><JOINT_INDEX> <DELTA>", e.g. "LF0 1"
    std::stringstream ss(msg->data);
    std::string joint_id;
    double delta = 0.0;
    std::string extra;
    if (!(ss >> joint_id >> delta) || (ss >> extra) || joint_id.size() != 3) {
      RCLCPP_WARN(this->get_logger(),
                  "Invalid joint offset cmd: '%s' (expected like 'LF0 1')",
                  msg->data.c_str());
      return;
    }

    const std::string leg = joint_id.substr(0, 2);
    int base = -1;
    if (leg == "LF") {
      base = 0;
    } else if (leg == "RF") {
      base = 3;
    } else if (leg == "LH") {
      base = 6;
    } else if (leg == "RH") {
      base = 9;
    } else {
      RCLCPP_WARN(this->get_logger(), "Invalid leg in cmd: '%s'",
                  msg->data.c_str());
      return;
    }

    if (joint_id[2] < '0' || joint_id[2] > '2') {
      RCLCPP_WARN(this->get_logger(), "Invalid joint index in cmd: '%s'",
                  msg->data.c_str());
      return;
    }

    const int joint_index = base + (joint_id[2] - '0');
    runtime_offsets_[joint_index] += delta;
    RCLCPP_INFO(this->get_logger(),
                "Runtime offset updated %s (index %d): delta=%.2f current=%.2f",
                joint_id.c_str(), joint_index, delta,
                runtime_offsets_[joint_index]);
  }

  void timerCallback_() {
    if (!has_data_ || serial_fd_ < 0)
      return;
    if (latest_joints_.position.size() < 12)
      return;

    char buf[256];
    int len = snprintf(
        buf, sizeof(buf),
        "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
        latest_joints_.position[0] + base_joint_offsets_[0] + runtime_offsets_[0],
        latest_joints_.position[1] + base_joint_offsets_[1] + runtime_offsets_[1],
        latest_joints_.position[2] + base_joint_offsets_[2] + runtime_offsets_[2],
        latest_joints_.position[3] + base_joint_offsets_[3] + runtime_offsets_[3],
        latest_joints_.position[4] + base_joint_offsets_[4] + runtime_offsets_[4],
        latest_joints_.position[5] + base_joint_offsets_[5] + runtime_offsets_[5],
        latest_joints_.position[6] + base_joint_offsets_[6] + runtime_offsets_[6],
        latest_joints_.position[7] + base_joint_offsets_[7] + runtime_offsets_[7],
        latest_joints_.position[8] + base_joint_offsets_[8] + runtime_offsets_[8],
        latest_joints_.position[9] + base_joint_offsets_[9] + runtime_offsets_[9],
        latest_joints_.position[10] + base_joint_offsets_[10] +
            runtime_offsets_[10],
        latest_joints_.position[11] + base_joint_offsets_[11] +
            runtime_offsets_[11]);

    ssize_t written = write(serial_fd_, buf, len);
    if (written < 0) {
      RCLCPP_WARN(this->get_logger(), "Serial write error");
    }
  }

  // Background thread: reads lines from Arduino and parses IMU packets
  void readLoop_() {
    std::string line;
    line.reserve(128);

    while (running_) {
      char c;
      ssize_t n = read(serial_fd_, &c, 1);
      if (n <= 0)
        continue;

      if (c == '\n') {
        if (!line.empty()) {
          parseIncoming_(line);
          line.clear();
        }
      } else if (c != '\r') {
        line += c;
      }
    }
  }

  void parseIncoming_(const std::string &line) {
    // Expected format: IMU,qw,qx,qy,qz,ax,ay,az,gx,gy,gz,csum
    if (line.size() < 4 || line.substr(0, 4) != "IMU,")
      return;

    size_t last_comma = line.find_last_of(',');
    if (last_comma == std::string::npos || last_comma == line.size() - 1)
      return;

    // XOR Checksum over all characters including the trailing comma
    int expected_csum = 0;
    for (size_t i = 0; i <= last_comma; i++) {
      expected_csum ^= line[i];
    }

    int received_csum = -1;
    try {
      received_csum = std::stoi(line.substr(last_comma + 1));
    } catch (...) {
      return;
    }

    if (expected_csum != received_csum) {
      RCLCPP_WARN(this->get_logger(), "IMU checksum mismatch!");
      return;
    }

    float vals[10] = {};
    int count = 0;

    // payload is what sits between "IMU," and the last comma
    std::string payload = line.substr(4, last_comma - 4);
    std::stringstream ss(payload);
    std::string token;

    while (std::getline(ss, token, ',') && count < 10) {
      try {
        vals[count++] = std::stof(token);
      } catch (...) {
        return;
      }
    }

    if (count < 10)
      return;

    champ_msgs::msg::Imu msg;
    msg.orientation.w = vals[0];
    msg.orientation.x = vals[1];
    msg.orientation.y = vals[2];
    msg.orientation.z = vals[3];
    msg.linear_acceleration.x = vals[4];
    msg.linear_acceleration.y = vals[5];
    msg.linear_acceleration.z = vals[6];
    msg.angular_velocity.x = vals[7];
    msg.angular_velocity.y = vals[8];
    msg.angular_velocity.z = vals[9];

    imu_pub_->publish(msg);
  }

  void configureSerial_(int fd, speed_t baud) {
    struct termios tty;
    memset(&tty, 0, sizeof tty);
    tcgetattr(fd, &tty);

    cfsetospeed(&tty, baud);
    cfsetispeed(&tty, baud);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    tty.c_iflag &= ~IGNBRK;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_lflag = 0;
    tty.c_oflag = 0;

    tty.c_cc[VMIN] = 0;  // non-blocking to allow timeout
    tty.c_cc[VTIME] = 1; // 0.1s timeout so read thread can exit on Ctrl+C

    tcsetattr(fd, TCSANOW, &tty);
  }

  int serial_fd_ = -1;
  bool has_data_ = false;
  champ_msgs::msg::Joints latest_joints_;
  std::array<double, 12> base_joint_offsets_ = {
      85.0, 80.0, 180.0, // LF
      95.0, 35.0, 200.0, // RF
      85.0, 15.0, 155.0, // LH
      85.0, 80.0, 180.0  // RH
  };
  std::array<double, 12> runtime_offsets_ = {
      0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

  std::atomic<bool> running_;
  std::thread read_thread_;

  rclcpp::Publisher<champ_msgs::msg::Imu>::SharedPtr imu_pub_;
  rclcpp::Subscription<champ_msgs::msg::Joints>::SharedPtr subscription_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr joint_offset_sub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char *argv[]) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<SerialJointSender>());
  rclcpp::shutdown();
  return 0;
}
