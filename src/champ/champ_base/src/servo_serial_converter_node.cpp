// serial_joint_sender_node.cpp
#include <rclcpp/rclcpp.hpp>
#include <champ_msgs/msg/joints.hpp>

// Serial
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>

class SerialJointSender : public rclcpp::Node
{
public:
    SerialJointSender() : Node("serial_joint_sender_node")
    {
        // Mở serial port
        serial_fd_ = open("/dev/ttyACM0", O_RDWR | O_NOCTTY | O_SYNC);
        if (serial_fd_ < 0) {
            RCLCPP_ERROR(this->get_logger(), "Không mở được /dev/ttyACM0");
            return;
        }

        
        configureSerial_(serial_fd_, B115200);
        RCLCPP_INFO(this->get_logger(), "Đã mở /dev/ttyACM0 @ 115200");

        // Sub /joints_debug
        subscription_ = this->create_subscription<champ_msgs::msg::Joints>(
            "joints_debug", 10,
            std::bind(&SerialJointSender::jointsCallback_, this, std::placeholders::_1));

        // Timer 0.1s để gửi giá trị mới nhất
        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(100),
            std::bind(&SerialJointSender::timerCallback_, this));
    }

    ~SerialJointSender()
    {
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

        char buf[64];
        int offset = snprintf(buf, sizeof(buf), "%.2f\n", latest_joints_.position[1]);

        ssize_t written = write(serial_fd_, buf, offset);
        if (written < 0) {
            RCLCPP_WARN(this->get_logger(), "Lỗi ghi serial");
        } else {
            RCLCPP_INFO(this->get_logger(), "Sent: %.2f", latest_joints_.position[1]);
        }
    }

    void configureSerial_(int fd, speed_t baud)
    {
        struct termios tty;
        memset(&tty, 0, sizeof tty);
        tcgetattr(fd, &tty);

        cfsetospeed(&tty, baud);
        cfsetispeed(&tty, baud);

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8-bit
        tty.c_cflag |= (CLOCAL | CREAD);
        tty.c_cflag &= ~(PARENB | PARODD);           // no parity
        tty.c_cflag &= ~CSTOPB;                      // 1 stop bit
        tty.c_cflag &= ~CRTSCTS;                     // no flow control

        tty.c_iflag &= ~IGNBRK;
        tty.c_iflag &= ~(IXON | IXOFF | IXANY);
        tty.c_lflag = 0;
        tty.c_oflag = 0;

        tty.c_cc[VMIN]  = 0;
        tty.c_cc[VTIME] = 5; // 0.5s read timeout

        tcsetattr(fd, TCSANOW, &tty);
    }

    int serial_fd_ = -1;
    bool has_data_ = false;
    champ_msgs::msg::Joints latest_joints_;

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