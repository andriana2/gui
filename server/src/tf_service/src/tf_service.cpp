#include <rclcpp/rclcpp.hpp>
#include <tf2_msgs/msg/tf_message.hpp>
#include <geometry_msgs/msg/transform_stamped.hpp>
#include "interface_srv/srv/get_robot_position.hpp"  // Interfaz personalizada
#include <unordered_map>
#include <string>
#include <vector>
#include <cmath>

class TFServiceNode : public rclcpp::Node
{
public:
    struct GetPosition
    {
        float x;
        float y;
        float yaw;
    };
    TFServiceNode() : Node("tf_service_node")
    {
        // Suscripción al topic /tf
        subscription_ = this->create_subscription<tf2_msgs::msg::TFMessage>(
            "/tf", 10, std::bind(&TFServiceNode::tfCallback, this, std::placeholders::_1));

        // Servicio para obtener transformaciones
        service_ = this->create_service<interface_srv::srv::GetRobotPosition>(
            "get_transform", std::bind(&TFServiceNode::handleServiceRequest, this, std::placeholders::_1, std::placeholders::_2));

        RCLCPP_INFO(this->get_logger(), "TF Service Node has been started.");
    }

private:
    // Callback para procesar datos del topic /tf
    void tfCallback(const tf2_msgs::msg::TFMessage::SharedPtr msg)
    {
        // std::cout << "tfCallback" << std::endl;

        for (const auto &transform : msg->transforms)
        {
            std::string key = transform.header.frame_id + "->" + transform.child_frame_id;
            transform_cache_[key] = transform;
        }
    }

    // Encontrar la transformación acumulada entre dos frames
    bool findTransform(const std::string &parent_frame, const std::string &child_frame, geometry_msgs::msg::Transform &result)
    {
        std::cout << "encontrando transformacion" << std::endl;

        std::string key = parent_frame + "->" + child_frame;

        // Caso base: transformación directa
        if (transform_cache_.find(key) != transform_cache_.end())
        {
            const auto &transform = transform_cache_[key];
            result = transform.transform;
            return true;
        }

        // Buscar frame intermedio
        for (const auto &[cached_key, transform] : transform_cache_)
        {
            if (cached_key.find(parent_frame + "->") == 0)
            {
                std::string intermediate_frame = cached_key.substr(parent_frame.size() + 2); // Extraer el frame hijo

                geometry_msgs::msg::Transform intermediate_transform;
                if (findTransform(intermediate_frame, child_frame, intermediate_transform))
                {
                    // Multiplicar transformaciones (parent_frame -> intermediate_frame -> child_frame)
                    combineTransforms(transform.transform, intermediate_transform, result);
                    return true;
                }
            }
        }

        return false;
    }

    // Combinar dos transformaciones
    void combineTransforms(const geometry_msgs::msg::Transform& t1, const geometry_msgs::msg::Transform& t2, geometry_msgs::msg::Transform& result) {
        // Combinar traslaciones
        std::cout << "transformando" << std::endl;
        
        result.translation.x = t1.translation.x + t2.translation.x;
        result.translation.y = t1.translation.y + t2.translation.y;
        result.translation.z = t1.translation.z + t2.translation.z;

        // Combinar rotaciones (multiplicación de cuaterniones)
        result.rotation.x = t1.rotation.w * t2.rotation.x + t1.rotation.x * t2.rotation.w + t1.rotation.y * t2.rotation.z - t1.rotation.z * t2.rotation.y;
        result.rotation.y = t1.rotation.w * t2.rotation.y - t1.rotation.x * t2.rotation.z + t1.rotation.y * t2.rotation.w + t1.rotation.z * t2.rotation.x;
        result.rotation.z = t1.rotation.w * t2.rotation.z + t1.rotation.x * t2.rotation.y - t1.rotation.y * t2.rotation.x + t1.rotation.z * t2.rotation.w;
        result.rotation.w = t1.rotation.w * t2.rotation.w - t1.rotation.x * t2.rotation.x - t1.rotation.y * t2.rotation.y - t1.rotation.z * t2.rotation.z;
    }

    void transform2GetPosition(const geometry_msgs::msg::Transform &result, struct GetPosition &result_struct)
    {
        std::cout << "En las transformaciones" << std::endl;
        
        result_struct.x = result.translation.x;
        result_struct.y = result.translation.y;

        float siny_cosp = 2.0 * (result.rotation.w * result.rotation.z + result.rotation.x * result.rotation.y);
        float cosy_cosp = 1.0 - 2.0 * (result.rotation.y * result.rotation.y + result.rotation.z * result.rotation.z);
        result_struct.yaw = std::atan2(siny_cosp, cosy_cosp);
    }

    // Manejar solicitudes del servicio
    void handleServiceRequest(
        [[maybe_unused]] const std::shared_ptr<interface_srv::srv::GetRobotPosition::Request> request,
        std::shared_ptr<interface_srv::srv::GetRobotPosition::Response> response)
    {
        std::cout << "Manejando solicitudes" << std::endl;
        geometry_msgs::msg::Transform result;

        if (findTransform("map", "base_footprint", result))
        {
            struct GetPosition gp;
            transform2GetPosition(result, gp);
            response->x = gp.x;
            response->y = gp.y;
            response->yaw = gp.yaw;
            response->success = true;
            RCLCPP_INFO(this->get_logger(), "Transform found: map -> base_footprint");
        }
        else
        {
            response->success = false;
            RCLCPP_WARN(this->get_logger(), "Transform not found: map -> base_footprint");
        }
    }

    rclcpp::Subscription<tf2_msgs::msg::TFMessage>::SharedPtr subscription_;
    rclcpp::Service<interface_srv::srv::GetRobotPosition>::SharedPtr service_;

    // Almacén para guardar las transformaciones más recientes
    std::unordered_map<std::string, geometry_msgs::msg::TransformStamped> transform_cache_;
};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<TFServiceNode>());
    rclcpp::shutdown();
    return 0;
}

// #include <rclcpp/rclcpp.hpp>
// #include <geometry_msgs/msg/transform_stamped.hpp>
// #include <tf2_ros/transform_listener.h>
// #include <tf2_ros/buffer.h>
// #include "interface_srv/srv/get_robot_position.hpp" // Cambia según tu archivo .srv
// #include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>

// class RobotPoseService : public rclcpp::Node
// {
// public:
//     RobotPoseService()
//         : Node("robot_pose_service"), tf_buffer_(std::make_shared<tf2_ros::Buffer>(this->get_clock())), tf_listener_(*tf_buffer_)
//     {
//         service_ = this->create_service<interface_srv::srv::GetRobotPosition>(
//             "get_robot_pose",
//             std::bind(&RobotPoseService::handle_pose_request, this, std::placeholders::_1, std::placeholders::_2)
//         );
//         RCLCPP_INFO(this->get_logger(), "Robot Pose Service ready.");
//     }

// private:
//     void handle_pose_request([[maybe_unused]] const std::shared_ptr<interface_srv::srv::GetRobotPosition::Request> request,
//                              std::shared_ptr<interface_srv::srv::GetRobotPosition::Response> response)
//     {
//         try
//         {
//             // Obtener la transformación de "map" a "odom"
//             auto map_to_odom = tf_buffer_->lookupTransform("map", "odom", tf2::TimePointZero);

//             // Obtener la transformación de "odom" a "base_link"
//             auto odom_to_base = tf_buffer_->lookupTransform("odom", "base_link", tf2::TimePointZero);

//             // Combinar ambas transformaciones
//             geometry_msgs::msg::TransformStamped map_to_base;
//             tf2::doTransform(odom_to_base, map_to_base, map_to_odom);

//             // Extraer posición
//             response->x = map_to_base.transform.translation.x;
//             response->y = map_to_base.transform.translation.y;

//             // Convertir cuaternión a yaw
//             tf2::Quaternion q(
//                 map_to_base.transform.rotation.x,
//                 map_to_base.transform.rotation.y,
//                 map_to_base.transform.rotation.z,
//                 map_to_base.transform.rotation.w);
//             double roll, pitch, yaw;
//             tf2::Matrix3x3(q).getRPY(roll, pitch, yaw);
//             response->yaw = yaw;

//             // RCLCPP_INFO(this->get_logger(), "Robot pose: x=%.2f, y=%.2f, yaw=%.2f", response->x, response->y, response->yaw);
//         }
//         catch (const tf2::TransformException &ex)
//         {
//             RCLCPP_ERROR(this->get_logger(), "Failed to get transform: %s", ex.what());
//             response->x = response->y = response->yaw = 0.0;
//         }
//     }

//     rclcpp::Service<interface_srv::srv::GetRobotPosition>::SharedPtr service_;
//     std::shared_ptr<tf2_ros::Buffer> tf_buffer_;
//     tf2_ros::TransformListener tf_listener_;
// };

// int main(int argc, char **argv)
// {
//     rclcpp::init(argc, argv);
//     auto node = std::make_shared<RobotPoseService>();
//     rclcpp::spin(node);
//     rclcpp::shutdown();
//     return 0;
// }
