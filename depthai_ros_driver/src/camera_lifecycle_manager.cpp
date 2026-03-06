#include "depthai_ros_driver/camera_lifecycle_manager.hpp"

#include <chrono>
#include <utility>

#include "diagnostic_msgs/msg/diagnostic_status.hpp"

namespace depthai_ros_driver {

CameraLifecycleManager::CameraLifecycleManager(const rclcpp::NodeOptions& options)
    : rclcpp_lifecycle::LifecycleNode("camera_lifecycle_manager", options) {
    this->declare_parameter<std::string>("camera_name", "camera");
    this->declare_parameter<double>("service_wait_timeout_sec", serviceWaitTimeoutSec_);
    this->declare_parameter<double>("diagnostic_period_sec", diagnosticPeriodSec_);
    this->declare_parameter<double>("stale_timeout_sec", staleTimeoutSec_);
}

void CameraLifecycleManager::updateServiceNames() {
    cameraName_ = this->get_parameter("camera_name").as_string();
    const auto ns = std::string(this->get_namespace());

    std::string prefix;
    if(!ns.empty() && ns != "/") {
        prefix = ns;
    }

    cameraStartService_ = prefix + "/" + cameraName_ + "/start_camera";
    cameraStopService_ = prefix + "/" + cameraName_ + "/stop_camera";

    if(cameraStartService_.empty() || cameraStartService_[0] != '/') {
        cameraStartService_ = "/" + cameraStartService_;
    }
    if(cameraStopService_.empty() || cameraStopService_[0] != '/') {
        cameraStopService_ = "/" + cameraStopService_;
    }
}

CameraLifecycleManager::CallbackReturn CameraLifecycleManager::on_configure(const rclcpp_lifecycle::State& /*state*/) {
    serviceWaitTimeoutSec_ = this->get_parameter("service_wait_timeout_sec").as_double();
    diagnosticPeriodSec_ = this->get_parameter("diagnostic_period_sec").as_double();
    staleTimeoutSec_ = this->get_parameter("stale_timeout_sec").as_double();

    updateServiceNames();

    if(cameraName_.empty()) {
        RCLCPP_ERROR(get_logger(), "camera_name parameter is empty.");
        return CallbackReturn::FAILURE;
    }

    startClient_ = this->create_client<Trigger>(cameraStartService_);
    stopClient_ = this->create_client<Trigger>(cameraStopService_);

    const auto timeout = std::chrono::duration<double>(serviceWaitTimeoutSec_);
    const bool start_ready = startClient_->wait_for_service(timeout);
    const bool stop_ready = stopClient_->wait_for_service(timeout);

    if(!start_ready || !stop_ready) {
        RCLCPP_ERROR(get_logger(), "Camera services are not available. start=%d stop=%d", start_ready, stop_ready);
        return CallbackReturn::FAILURE;
    }

    {
        std::lock_guard<std::mutex> lock(stateMutex_);
        desiredActive_ = false;
        cameraRunning_ = false;
        lastCommandSuccess_ = false;
        lastCommandMessage_ = "Configured";
        lastCommandResponseStamp_ = this->now();
    }

    RCLCPP_INFO(get_logger(), "Lifecycle manager configured for camera '%s'.", cameraName_.c_str());
    return CallbackReturn::SUCCESS;
}

void CameraLifecycleManager::sendTriggerRequest(const rclcpp::Client<Trigger>::SharedPtr& client, bool start_request) {
    if(!client || !client->service_is_ready()) {
        std::lock_guard<std::mutex> lock(stateMutex_);
        lastCommandSuccess_ = false;
        lastCommandMessage_ = "Service not ready";
        if(start_request) {
            cameraRunning_ = false;
        }
        lastCommandResponseStamp_ = this->now();
        if(diagnosticUpdater_) {
            diagnosticUpdater_->SetStatusERROR(lastCommandMessage_);
        }
        return;
    }

    auto req = std::make_shared<Trigger::Request>();
    client->async_send_request(req, [this, start_request](rclcpp::Client<Trigger>::SharedFuture future) {
        try {
            auto response = future.get();
            std::lock_guard<std::mutex> lock(stateMutex_);
            lastCommandSuccess_ = response->success;
            lastCommandMessage_ = response->message.empty() ? (start_request ? "Start request sent" : "Stop request sent") : response->message;
            if(start_request) {
                cameraRunning_ = response->success;
            } else if(response->success) {
                cameraRunning_ = false;
            }
            lastCommandResponseStamp_ = this->now();
            if(diagnosticUpdater_) {
                if(response->success && start_request) {
                    diagnosticUpdater_->SetStatusOK("Camera start request succeeded.");
                } else if(response->success) {
                    diagnosticUpdater_->SetStatusWARN("Camera stopped.");
                } else {
                    diagnosticUpdater_->SetStatusERROR(lastCommandMessage_);
                }
            }
        } catch(const std::exception& e) {
            std::lock_guard<std::mutex> lock(stateMutex_);
            lastCommandSuccess_ = false;
            lastCommandMessage_ = e.what();
            if(start_request) {
                cameraRunning_ = false;
            }
            lastCommandResponseStamp_ = this->now();
            if(diagnosticUpdater_) {
                diagnosticUpdater_->SetStatusERROR(lastCommandMessage_);
            }
        }
    });
}

CameraLifecycleManager::CallbackReturn CameraLifecycleManager::on_activate(const rclcpp_lifecycle::State& /*state*/) {
    {
        std::lock_guard<std::mutex> lock(stateMutex_);
        desiredActive_ = true;
        lastCommandMessage_ = "Sending start request";
    }
    rii_common_utils::DiagnosticUpdaterBuilder diagnostic_updater_builder(this);
    diagnosticUpdater_ = diagnostic_updater_builder.SetPeriodInSec(diagnosticPeriodSec_)
                           .SetHardwareID(cameraName_)
                           .EnableStatusUpdate()
                           .RegisterCustomUpdaterFunction(
                               "camera_lifecycle",
                               std::bind(&CameraLifecycleManager::lifecycleDiagnostics, this, std::placeholders::_1))
                           .Build();
    diagnosticUpdater_->SetStatusWARN("Lifecycle manager activated. Waiting for camera start response.");

    sendTriggerRequest(startClient_, true);
    RCLCPP_INFO(get_logger(), "Lifecycle manager activated. Start request dispatched.");
    return CallbackReturn::SUCCESS;
}

CameraLifecycleManager::CallbackReturn CameraLifecycleManager::on_deactivate(const rclcpp_lifecycle::State& /*state*/) {
    {
        std::lock_guard<std::mutex> lock(stateMutex_);
        desiredActive_ = false;
        lastCommandMessage_ = "Sending stop request";
    }
    if(diagnosticUpdater_) {
        diagnosticUpdater_->SetStatusWARN("Lifecycle manager deactivating.");
    }
    sendTriggerRequest(stopClient_, false);
    RCLCPP_INFO(get_logger(), "Lifecycle manager deactivated. Stop request dispatched.");
    return CallbackReturn::SUCCESS;
}

CameraLifecycleManager::CallbackReturn CameraLifecycleManager::on_cleanup(const rclcpp_lifecycle::State& /*state*/) {
    diagnosticUpdater_.reset();
    startClient_.reset();
    stopClient_.reset();
    {
        std::lock_guard<std::mutex> lock(stateMutex_);
        desiredActive_ = false;
        cameraRunning_ = false;
        lastCommandSuccess_ = false;
        lastCommandMessage_ = "Cleaned up";
    }
    return CallbackReturn::SUCCESS;
}

CameraLifecycleManager::CallbackReturn CameraLifecycleManager::on_shutdown(const rclcpp_lifecycle::State& /*state*/) {
    sendTriggerRequest(stopClient_, false);
    if(diagnosticUpdater_) {
        diagnosticUpdater_->SetStatusSTALE("Lifecycle manager shutting down.");
    }
    diagnosticUpdater_.reset();
    return CallbackReturn::SUCCESS;
}

void CameraLifecycleManager::lifecycleDiagnostics(diagnostic_updater::DiagnosticStatusWrapper& status) {
    bool desired_active = false;
    bool camera_running = false;
    bool last_success = false;
    std::string last_message;
    rclcpp::Time last_response;

    {
        std::lock_guard<std::mutex> lock(stateMutex_);
        desired_active = desiredActive_;
        camera_running = cameraRunning_;
        last_success = lastCommandSuccess_;
        last_message = lastCommandMessage_;
        last_response = lastCommandResponseStamp_;
    }

    const auto current_state = this->get_current_state();
    const auto now = this->now();

    int summary_level = diagnostic_msgs::msg::DiagnosticStatus::OK;
    std::string summary_message = "Operating normally";

    if(current_state.id() != lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE) {
        summary_level = diagnostic_msgs::msg::DiagnosticStatus::WARN;
        summary_message = "Lifecycle manager is not active";
    } else if(!camera_running || !last_success) {
        summary_level = diagnostic_msgs::msg::DiagnosticStatus::ERROR;
        summary_message = "Lifecycle active but camera is not running";
    }

    if(desired_active && last_response.nanoseconds() > 0) {
        const auto age_sec = (now - last_response).seconds();
        if(age_sec > staleTimeoutSec_ && !camera_running) {
            summary_level = diagnostic_msgs::msg::DiagnosticStatus::STALE;
            summary_message = "No successful camera start response within timeout";
        }
        status.add("last_response_age_sec", age_sec);
    }

    status.summary(summary_level, summary_message);
    status.add("lifecycle_state", current_state.label());
    status.add("camera_name", cameraName_);
    status.add("camera_running", camera_running);
    status.add("desired_active", desired_active);
    status.add("start_service", cameraStartService_);
    status.add("stop_service", cameraStopService_);
    status.add("start_service_ready", startClient_ ? startClient_->service_is_ready() : false);
    status.add("stop_service_ready", stopClient_ ? stopClient_->service_is_ready() : false);
    status.add("last_command_success", last_success);
    status.add("last_command_message", last_message);
}

}  // namespace depthai_ros_driver
