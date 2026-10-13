#include <rclcpp/rclcpp.hpp>
#include "xgo_driver/xgo_mini2s.hpp"                                                                                                                                               
#include <geometry_msgs/msg/twist.hpp>                                                                                                            
#include <sensor_msgs/msg/battery_state.hpp>                                                                                                      
#include <sensor_msgs/msg/imu.hpp>
#include <cmath>
#include <numbers>

class XgoDriverNode:public rclcpp::Node{
    xgo::XgoMini2s robot_;
    //timers
    rclcpp::TimerBase::SharedPtr sensor_timer_;                                                                                                       
    rclcpp::TimerBase::SharedPtr battery_timer_;

    //Subscribers and publishers
    rclcpp::Publisher<sensor_msgs::msg::BatteryState>::SharedPtr battery_pub_;
    rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr imu_pub_;
    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_sub_;

    // Safety Watchdog
    double timeout_s_{0.5};
    rclcpp::Time last_cmd_vel_time_{0, 0, RCL_ROS_TIME};
    bool is_moving_{false};

public :
    XgoDriverNode (): Node("xgo_driver_node"){

        std::string port= this->declare_parameter<std::string>("serial_port", "/dev/ttyAMA0");                                                                              
        int baudrate= this->declare_parameter<int>("baudrate", 115200);                                                                                                 
        double rate_hz= this->declare_parameter<double>("publish_rate_hz", 20.0);                                                                                         
        timeout_s_ = this->declare_parameter<double>("cmd_vel_timeout_sec", 0.5);
        last_cmd_vel_time_ = this->now();

        if (!robot_.connect(port, baudrate)) {
            RCLCPP_ERROR(this->get_logger(), "Failure connecting to the port: %s", port.c_str());
        } else {
            RCLCPP_INFO(this->get_logger(), "Connected to the port: %s", port.c_str());
        }

        //Subscriptors

        cmd_vel_sub_ = this->create_subscription<geometry_msgs::msg::Twist>(                                                                          
                "cmd_vel", 10,                                                                                                                            
                std::bind(&XgoDriverNode::cmd_vel_callback, this, std::placeholders::_1)                                                                  
        );

        // Publishers
        sensor_timer_ = this->create_wall_timer(std::chrono::duration<double>(1.0/rate_hz), std::bind(&XgoDriverNode::sensor_timer_callback, this));
        battery_timer_ = this->create_wall_timer(std::chrono::seconds(5),std::bind(&XgoDriverNode::battery_timer_callback, this));

        battery_pub_ = this->create_publisher<sensor_msgs::msg::BatteryState>("battery_state", 10);                                                       
        imu_pub_     = this->create_publisher<sensor_msgs::msg::Imu>("imu/data", 10); 
    }

    ~XgoDriverNode(){
        robot_.disconnect();
    }

private:
    static geometry_msgs::msg::Quaternion euler_to_quaternion(double roll_rad, double pitch_rad, double yaw_rad) {
        double cr = std::cos(roll_rad * 0.5);
        double sr = std::sin(roll_rad * 0.5);
        double cp = std::cos(pitch_rad * 0.5);
        double sp = std::sin(pitch_rad * 0.5);
        double cy = std::cos(yaw_rad * 0.5);
        double sy = std::sin(yaw_rad * 0.5);

        geometry_msgs::msg::Quaternion q;
        q.w = cr * cp * cy + sr * sp * sy;
        q.x = sr * cp * cy - cr * sp * sy;
        q.y = cr * sp * cy + sr * cp * sy;
        q.z = cr * cp * sy - sr * sp * cy;
        return q;
    }

    sensor_msgs::msg::Imu to_imu_msg(const xgo::ImuData& data) {
        sensor_msgs::msg::Imu msg;
        msg.header.stamp = this->now();
        msg.header.frame_id = "imu_link";

        constexpr double deg2rad = std::numbers::pi / 180.0;
        msg.orientation = euler_to_quaternion(
            data.roll * deg2rad,
            data.pitch * deg2rad,
            data.yaw * deg2rad
        );

        // Covarianzas en -1 indican a ROS que estos valores no están medidos
        msg.angular_velocity_covariance[0] = -1.0;
        msg.linear_acceleration_covariance[0] = -1.0;
        return msg;
    }

    void check_cmd_vel_timeout() {
        if (is_moving_ && (this->now() - last_cmd_vel_time_).seconds() > timeout_s_) {
            RCLCPP_WARN(this->get_logger(), "cmd_vel timeout (%.2f s) exceeded: stopping robot", timeout_s_);
            robot_.stop();
            is_moving_ = false;
        }
    }

    void sensor_timer_callback() {
        check_cmd_vel_timeout();

        if (auto imu = robot_.read_imu()) {
            imu_pub_->publish(to_imu_msg(*imu));
        }
    }

    void battery_timer_callback() {
        if (auto battery = robot_.read_battery()) {
            sensor_msgs::msg::BatteryState battery_msg;
            battery_msg.header.stamp = this->now();
            battery_msg.percentage = static_cast<float>(*battery) / 100.0f;
            battery_msg.present = true;
            battery_pub_->publish(battery_msg);
        }
    }

    void cmd_vel_callback(const geometry_msgs::msg::Twist::SharedPtr msg) {
        last_cmd_vel_time_ = this->now();
        bool has_velocity = (msg->linear.x != 0.0 || msg->linear.y != 0.0 || msg->angular.z != 0.0);
        is_moving_ = has_velocity;
        robot_.set_velocity(msg->linear.x, msg->linear.y, msg->angular.z);
    }
    
    


};


int main(int argc, char** argv) {
        rclcpp::init(argc, argv);
        auto node = std::make_shared<XgoDriverNode>();
        rclcpp::spin(node);
        rclcpp::shutdown();
        return 0;
    }