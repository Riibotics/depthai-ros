#include "depthai_ros_driver/pipeline/base_types.hpp"

#include "depthai/device/Device.hpp"
#include "depthai/pipeline/Pipeline.hpp"
#include "depthai_ros_driver/dai_nodes/base_node.hpp"
#include "depthai_ros_driver/dai_nodes/sensors/sensor_helpers.hpp"
#include "depthai_ros_driver/dai_nodes/sensors/sensor_wrapper.hpp"
#include "depthai_ros_driver/dai_nodes/sensors/stereo.hpp"
#include "depthai_ros_driver/pipeline/base_pipeline.hpp"
#include "rclcpp/node.hpp"

namespace depthai_ros_driver {
namespace pipeline_gen {

std::vector<std::unique_ptr<dai_nodes::BaseNode>> RGB::createPipeline(std::shared_ptr<rclcpp::Node> node,
                                                                      std::shared_ptr<dai::Device> device,
                                                                      std::shared_ptr<dai::Pipeline> pipeline) {
    using namespace dai_nodes::sensor_helpers;
    std::vector<std::unique_ptr<dai_nodes::BaseNode>> daiNodes;
    auto rgb = std::make_unique<dai_nodes::SensorWrapper>(getNodeName(node, NodeNameEnum::RGB), node, pipeline, device, dai::CameraBoardSocket::CAM_A);
    daiNodes.push_back(std::move(rgb));
    return daiNodes;
}

std::vector<std::unique_ptr<dai_nodes::BaseNode>> RGBD::createPipeline(std::shared_ptr<rclcpp::Node> node,
                                                                       std::shared_ptr<dai::Device> device,
                                                                       std::shared_ptr<dai::Pipeline> pipeline) {
    using namespace dai_nodes::sensor_helpers;
    std::vector<std::unique_ptr<dai_nodes::BaseNode>> daiNodes;
    auto rgb = std::make_unique<dai_nodes::SensorWrapper>(getNodeName(node, NodeNameEnum::RGB), node, pipeline, device, dai::CameraBoardSocket::CAM_A);
    auto stereo = std::make_unique<dai_nodes::Stereo>(getNodeName(node, NodeNameEnum::Stereo), node, pipeline, device);
    daiNodes.push_back(std::move(rgb));
    daiNodes.push_back(std::move(stereo));
    return daiNodes;
}

std::vector<std::unique_ptr<dai_nodes::BaseNode>> Stereo::createPipeline(std::shared_ptr<rclcpp::Node> node,
                                                                         std::shared_ptr<dai::Device> device,
                                                                         std::shared_ptr<dai::Pipeline> pipeline) {
    using namespace dai_nodes::sensor_helpers;
    std::vector<std::unique_ptr<dai_nodes::BaseNode>> daiNodes;
    auto left = std::make_unique<dai_nodes::SensorWrapper>(getNodeName(node, NodeNameEnum::Left), node, pipeline, device, dai::CameraBoardSocket::CAM_B);
    auto right = std::make_unique<dai_nodes::SensorWrapper>(getNodeName(node, NodeNameEnum::Right), node, pipeline, device, dai::CameraBoardSocket::CAM_C);
    daiNodes.push_back(std::move(left));
    daiNodes.push_back(std::move(right));
    return daiNodes;
}

std::vector<std::unique_ptr<dai_nodes::BaseNode>> Depth::createPipeline(std::shared_ptr<rclcpp::Node> node,
                                                                        std::shared_ptr<dai::Device> device,
                                                                        std::shared_ptr<dai::Pipeline> pipeline) {
    using namespace dai_nodes::sensor_helpers;
    std::vector<std::unique_ptr<dai_nodes::BaseNode>> daiNodes;
    auto stereo = std::make_unique<dai_nodes::Stereo>(getNodeName(node, NodeNameEnum::Stereo), node, pipeline, device);
    daiNodes.push_back(std::move(stereo));
    return daiNodes;
}

}  // namespace pipeline_gen
}  // namespace depthai_ros_driver

#include <pluginlib/class_list_macros.hpp>

PLUGINLIB_EXPORT_CLASS(depthai_ros_driver::pipeline_gen::RGB, depthai_ros_driver::pipeline_gen::BasePipeline)
PLUGINLIB_EXPORT_CLASS(depthai_ros_driver::pipeline_gen::RGBD, depthai_ros_driver::pipeline_gen::BasePipeline)
PLUGINLIB_EXPORT_CLASS(depthai_ros_driver::pipeline_gen::Stereo, depthai_ros_driver::pipeline_gen::BasePipeline)
PLUGINLIB_EXPORT_CLASS(depthai_ros_driver::pipeline_gen::Depth, depthai_ros_driver::pipeline_gen::BasePipeline)
