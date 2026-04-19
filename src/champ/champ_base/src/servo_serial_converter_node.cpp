// serial_joint_sender_node.cpp
#include <rclcpp/rclcpp.hpp>
#include <champ_msgs/msg/joints.hpp>
#include <champ_msgs/msg/imu.hpp>

// Serial
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>

// Threading
#include <thread>
#include <atomic>
#include <sstream>

// Offset configuration (in units of positions, adjust as needed)
#define OFFSET_JOINT_1  0.0
#define OFFSET_JOINT_2 180.0

class SerialJointSender : public rclcpp::Node
{
public:
    SerialJointSender() : Node("serial_joint_sender_node"), running_(false)
    {
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
            std::bind(&SerialJointSender::jointsCallback_, this, std::placeholders::_1));

        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(100),
            std::bind(&SerialJointSender::timerCallback_, this));

        running_ = true;
        read_thread_ = std::thread(&SerialJointSender::readLoop_, this);
    }

    ~SerialJointSender()
    {
        running_ = false;
        if (read_thread_.joinable()) read_thread_.join();
        if (serial_fd_ >= 0) close(serial_fd_);
    }

private:
    void jointsCallback_(const champ_msgs::msg::Joints::SharedPtr msg)
    {
        latest_joints_ = *msg;
        has_data_ = true;
    }

    void timerCallback_()
    {
        if (!has_data_ || serial_fd_ < 0) return;
        if (latest_joints_.position.size() < 3) return;

        char buf[64];
        double val1 = latest_joints_.position[1] + OFFSET_JOINT_1;
        double val2 = latest_joints_.position[2] + OFFSET_JOINT_2;
        int len = snprintf(buf, sizeof(buf), "%.2f,%.2f\n", val1, val2);

        ssize_t written = write(serial_fd_, buf, len);
        if (written < 0) {
            RCLCPP_WARN(this->get_logger(), "Serial write error");
        } else {
            RCLCPP_INFO(this->get_logger(), "Sent: %.2f, %.2f", val1, val2);
        }
    }

    // Background thread: reads lines from Arduino and parses IMU packets
    void readLoop_()
    {
        std::string line;
        line.reserve(128);

        while (running_) {
            char c;
            ssize_t n = read(serial_fd_, &c, 1);
            if (n <= 0) continue;

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

    void parseIncoming_(const std::string& line)
    {
        // Expected format: IMU,qw,qx,qy,qz,ax,ay,az,gx,gy,gz,csum
        if (line.size() < 4 || line.substr(0, 4) != "IMU,") return;

        float vals[11] = {};
        int count = 0;

        std::stringstream ss(line.substr(4));
        std::string token;
        while (std::getline(ss, token, ',') && count < 11) {
            try { vals[count++] = std::stof(token); }
            catch (...) { return; }
        }

        if (count < 11) return;

        // Validating checksum
        int expected_csum = ((int)(vals[4] + vals[5] + vals[6] + vals[7] + vals[8] + vals[9])) & 0xFF;
        int received_csum = (int)vals[10];

        if (expected_csum != received_csum) {
            RCLCPP_WARN(this->get_logger(), "IMU checksum mismatch!");
            return;
        }

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

    void configureSerial_(int fd, speed_t baud)
    {
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

        tty.c_cc[VMIN]  = 0;   // non-blocking to allow timeout
        tty.c_cc[VTIME] = 1;   // 0.1s timeout so read thread can exit on Ctrl+C

        tcsetattr(fd, TCSANOW, &tty);
    }

    int serial_fd_ = -1;
    bool has_data_ = false;
    champ_msgs::msg::Joints latest_joints_;

    std::atomic<bool> running_;
    std::thread read_thread_;

    rclcpp::Publisher<champ_msgs::msg::Imu>::SharedPtr imu_pub_;
    rclcpp::Subscription<champ_msgs::msg::Joints>::SharedPtr subscription_;
    rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<SerialJointSender>());
    rclcpp::shutdown();
    return 0;
}
