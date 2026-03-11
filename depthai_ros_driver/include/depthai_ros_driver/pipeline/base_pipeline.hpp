#pragma once

#include <memory>
#include <string>
#include <vector>

#include "depthai_ros_driver/dai_nodes/base_node.hpp"

namespace dai {
class Pipeline;
class Device;
}  // namespace dai

namespace rclcpp {
class Node;
}

namespace depthai_ros_driver {
namespace pipeline_gen {
class BasePipeline {
   public:
    ~BasePipeline() = default;
    virtual std::vector<std::unique_ptr<dai_nodes::BaseNode>> createPipeline(std::shared_ptr<rclcpp::Node> node,
                                                                             std::shared_ptr<dai::Device> device,
                                                                             std::shared_ptr<dai::Pipeline> pipeline) = 0;

   protected:
    BasePipeline(){};
};
}  // namespace pipeline_gen
}  // namespace depthai_ros_driver
