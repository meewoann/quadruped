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
    real_joint_pub_ =
        this->create_publisher<champ_msgs::msg::Joints>("/real_joint", 10);

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
  int jointIndexFromId_(const std::string &joint_id) {
    if (joint_id.size() != 3)
      return -1;
    int base = -1;
    const std::string leg = joint_id.substr(0, 2);
    if (leg == "LF") {
      base = 0;
    } else if (leg == "RF") {
      base = 3;
    } else if (leg == "LH") {
      base = 6;
    } else if (leg == "RH") {
      base = 9;
    } else {
      return -1;
    }
    if (joint_id[2] < '0' || joint_id[2] > '2')
      return -1;
    return base + (joint_id[2] - '0');
  }

  void printCalibrationTable_(const std::array<double, 12> &profile_offsets,
                              const char *profile_name) {
    const std::array<double, 12> calibrated_offsets = {
        base_joint_offsets_[0] + profile_offsets[0],
        base_joint_offsets_[1] + profile_offsets[1],
        base_joint_offsets_[2] + profile_offsets[2],
        base_joint_offsets_[3] + profile_offsets[3],
        base_joint_offsets_[4] + profile_offsets[4],
        base_joint_offsets_[5] + profile_offsets[5],
        base_joint_offsets_[6] + profile_offsets[6],
        base_joint_offsets_[7] + profile_offsets[7],
        base_joint_offsets_[8] + profile_offsets[8],
        base_joint_offsets_[9] + profile_offsets[9],
        base_joint_offsets_[10] + profile_offsets[10],
        base_joint_offsets_[11] + profile_offsets[11]};

    RCLCPP_INFO(this->get_logger(),
                "[%s] std::array<double, 12> base_joint_offsets_ = {\n"
                "    %.1f, %.1f, %.1f, // LF\n"
                "    %.1f, %.1f, %.1f, // RF\n"
                "    %.1f, %.1f, %.1f, // LH\n"
                "    %.1f, %.1f, %.1f  // RH\n"
                "};",
                profile_name, calibrated_offsets[0], calibrated_offsets[1],
                calibrated_offsets[2], calibrated_offsets[3],
                calibrated_offsets[4], calibrated_offsets[5],
                calibrated_offsets[6], calibrated_offsets[7],
                calibrated_offsets[8], calibrated_offsets[9],
                calibrated_offsets[10], calibrated_offsets[11]);
  }

  void jointsCallback_(const champ_msgs::msg::Joints::SharedPtr msg) {
    latest_joints_ = *msg;
    has_data_ = true;
  }

  void jointOffsetCallback_(const std_msgs::msg::String::SharedPtr msg) {
    // Supported formats:
    // 1) "<profile> <joint> <delta>"  e.g. "static LF0 5"
    // 2) "print <profile>"            e.g. "print static"
    // 3) "reset <profile>"            e.g. "reset moving"
    std::stringstream ss(msg->data);
    std::string cmd;
    std::string arg1;
    std::string extra;
    if (!(ss >> cmd >> arg1)) {
      RCLCPP_WARN(this->get_logger(), "Invalid joint offset cmd: '%s'",
                  msg->data.c_str());
      return;
    }

    auto is_profile = [](const std::string &v) {
      return v == "static" || v == "moving";
    };

    if (cmd == "print") {
      if (!(ss >> extra) && is_profile(arg1)) {
        const auto &profile_offsets =
            (arg1 == "static") ? static_offsets_ : moving_offsets_;
        printCalibrationTable_(profile_offsets,
                               arg1 == "static" ? "STATIC" : "MOVING");
      } else {
        RCLCPP_WARN(
            this->get_logger(),
            "Invalid print cmd: '%s' (use 'print static' or 'print moving')",
            msg->data.c_str());
      }
      return;
    }

    if (cmd == "reset") {
      if (!(ss >> extra) && is_profile(arg1)) {
        auto &profile_offsets =
            (arg1 == "static") ? static_offsets_ : moving_offsets_;
        profile_offsets.fill(0.0);
        RCLCPP_INFO(this->get_logger(), "[%s] Offsets reset to zero",
                    arg1 == "static" ? "STATIC" : "MOVING");
        printCalibrationTable_(profile_offsets,
                               arg1 == "static" ? "STATIC" : "MOVING");
      } else {
        RCLCPP_WARN(
            this->get_logger(),
            "Invalid reset cmd: '%s' (use 'reset static' or 'reset moving')",
            msg->data.c_str());
      }
      return;
    }

    double delta = 0.0;
    if (!is_profile(cmd) || !(ss >> delta) || (ss >> extra)) {
      RCLCPP_WARN(
          this->get_logger(),
          "Invalid update cmd: '%s' (use '<static|moving> <joint> <delta>')",
          msg->data.c_str());
      return;
    }

    const int joint_index = jointIndexFromId_(arg1);
    if (joint_index < 0) {
      RCLCPP_WARN(this->get_logger(), "Invalid joint in cmd: '%s'",
                  msg->data.c_str());
      return;
    }

    auto &profile_offsets =
        (cmd == "static") ? static_offsets_ : moving_offsets_;
    profile_offsets[joint_index] += delta;
    RCLCPP_INFO(this->get_logger(), "[%s] %s += %.1f -> %.1f",
                cmd == "static" ? "STATIC" : "MOVING", arg1.c_str(), delta,
                profile_offsets[joint_index]);
    printCalibrationTable_(profile_offsets,
                           cmd == "static" ? "STATIC" : "MOVING");
  }

  void timerCallback_() {
    if (!has_data_ || serial_fd_ < 0)
      return;
    if (latest_joints_.position.size() < 12)
      return;

    if (has_prev_joints_ && prev_joints_.position.size() >= 12) {
      is_moving_state_ = false;
      for (size_t i = 0; i < 12; ++i) {
        if (std::fabs(latest_joints_.position[i] - prev_joints_.position[i]) >
            moving_threshold_) {
          is_moving_state_ = true;
          break;
        }
      }
    } else {
      is_moving_state_ = false;
    }
    prev_joints_ = latest_joints_;
    has_prev_joints_ = true;

    const auto &active_profile_offsets =
        is_moving_state_ ? moving_offsets_ : static_offsets_;

    champ_msgs::msg::Joints real_joint_msg;
    real_joint_msg.position.resize(12);
    real_joint_msg.position[0] = latest_joints_.position[0] +
                                 base_joint_offsets_[0] +
                                 active_profile_offsets[0];
    real_joint_msg.position[1] = latest_joints_.position[1] +
                                 base_joint_offsets_[1] +
                                 active_profile_offsets[1];
    real_joint_msg.position[2] = latest_joints_.position[2] +
                                 base_joint_offsets_[2] +
                                 active_profile_offsets[2];
    real_joint_msg.position[3] = latest_joints_.position[3] +
                                 base_joint_offsets_[3] +
                                 active_profile_offsets[3];
    real_joint_msg.position[4] = latest_joints_.position[4] +
                                 base_joint_offsets_[4] +
                                 active_profile_offsets[4];
    real_joint_msg.position[5] = latest_joints_.position[5] +
                                 base_joint_offsets_[5] +
                                 active_profile_offsets[5];
    real_joint_msg.position[6] = latest_joints_.position[6] +
                                 base_joint_offsets_[6] +
                                 active_profile_offsets[6];
    real_joint_msg.position[7] = latest_joints_.position[7] +
                                 base_joint_offsets_[7] +
                                 active_profile_offsets[7];
    real_joint_msg.position[8] = latest_joints_.position[8] +
                                 base_joint_offsets_[8] +
                                 active_profile_offsets[8];
    real_joint_msg.position[9] = latest_joints_.position[9] +
                                 base_joint_offsets_[9] +
                                 active_profile_offsets[9];
    real_joint_msg.position[10] = latest_joints_.position[10] +
                                  base_joint_offsets_[10] +
                                  active_profile_offsets[10];
    real_joint_msg.position[11] = latest_joints_.position[11] +
                                  base_joint_offsets_[11] +
                                  active_profile_offsets[11];

    char buf[256];
    int len = snprintf(
        buf, sizeof(buf),
        "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
        real_joint_msg.position[0], real_joint_msg.position[1],
        real_joint_msg.position[2], real_joint_msg.position[3],
        real_joint_msg.position[4], real_joint_msg.position[5],
        real_joint_msg.position[6], real_joint_msg.position[7],
        real_joint_msg.position[8], real_joint_msg.position[9],
        real_joint_msg.position[10], real_joint_msg.position[11]);

    real_joint_pub_->publish(real_joint_msg);

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
      85.0, 35.0, 172.0, // LF
      98.0, 55.0, 180.0, // RF
      90.0, -5.0, 190.0, // LH
      95.0, 25.0, 182.0  // RH
  };
  std::array<double, 12> static_offsets_ = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                            0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  std::array<double, 12> moving_offsets_ = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                            0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  champ_msgs::msg::Joints prev_joints_;
  bool has_prev_joints_ = false;
  bool is_moving_state_ = false;
  double moving_threshold_ = 0.5;

  std::atomic<bool> running_;
  std::thread read_thread_;

  rclcpp::Publisher<champ_msgs::msg::Imu>::SharedPtr imu_pub_;
  rclcpp::Publisher<champ_msgs::msg::Joints>::SharedPtr real_joint_pub_;
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
