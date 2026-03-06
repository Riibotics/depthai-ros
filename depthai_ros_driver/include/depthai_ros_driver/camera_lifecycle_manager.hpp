#pragma once

#include <memory>
#include <mutex>
#include <string>

#include "diagnostic_updater/diagnostic_status_wrapper.hpp"
#include "lifecycle_msgs/msg/state.hpp"
#include "rclcpp/client.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/lifecycle_node.hpp"
#include "rii_common_utils/diagnostic_updater.h"
#include "std_srvs/srv/trigger.hpp"

namespace depthai_ros_driver {

class CameraLifecycleManager : public rclcpp_lifecycle::LifecycleNode {
   public:
    explicit CameraLifecycleManager(const rclcpp::NodeOptions& options = rclcpp::NodeOptions());

    using CallbackReturn = rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;

    CallbackReturn on_configure(const rclcpp_lifecycle::State& state) override;
    CallbackReturn on_activate(const rclcpp_lifecycle::State& state) override;
    CallbackReturn on_deactivate(const rclcpp_lifecycle::State& state) override;
    CallbackReturn on_cleanup(const rclcpp_lifecycle::State& state) override;
    CallbackReturn on_shutdown(const rclcpp_lifecycle::State& state) override;

   private:
    using Trigger = std_srvs::srv::Trigger;

    void updateServiceNames();
    void sendTriggerRequest(const rclcpp::Client<Trigger>::SharedPtr& client, bool start_request);
    void lifecycleDiagnostics(diagnostic_updater::DiagnosticStatusWrapper& status);

    std::string cameraName_;
    std::string cameraStartService_;
    std::string cameraStopService_;
    double serviceWaitTimeoutSec_{2.0};
    double diagnosticPeriodSec_{1.0};
    double staleTimeoutSec_{3.0};

    rclcpp::Client<Trigger>::SharedPtr startClient_;
    rclcpp::Client<Trigger>::SharedPtr stopClient_;

    std::unique_ptr<rii_common_utils::DiagnosticUpdater> diagnosticUpdater_;

    std::mutex stateMutex_;
    bool desiredActive_{false};
    bool cameraRunning_{false};
    bool lastCommandSuccess_{false};
    std::string lastCommandMessage_{"No command sent"};
    rclcpp::Time lastCommandResponseStamp_{0, 0, RCL_ROS_TIME};
};

}  // namespace depthai_ros_driver
