import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import ComposableNodeContainer, LoadComposableNodes
from launch_ros.descriptions import ComposableNode, ParameterFile


def _arg_or_default(context, arg_name, default_value):
    value = LaunchConfiguration(arg_name).perform(context)
    return value if value else default_value


def launch_setup(context, *args, **kwargs):
    params_file = ParameterFile(LaunchConfiguration("params_file"), allow_substs=True)
    namespace = LaunchConfiguration("namespace").perform(context)
    name = LaunchConfiguration("name").perform(context)
    rectify_rgb = LaunchConfiguration("rectify_rgb")
    pointcloud_enable = LaunchConfiguration("pointcloud_enable")
    target_container = f"{namespace}/{name}_container" if namespace else f"{name}_container"

    rgb_image_topic = _arg_or_default(context, "rgb_image_topic", f"{name}/rgb/image_raw")
    rgb_camera_info_topic = _arg_or_default(context, "rgb_camera_info_topic", f"{name}/rgb/camera_info")
    rgb_rect_topic = _arg_or_default(context, "rgb_rect_topic", f"{name}/rgb/image_rect")
    depth_image_topic = _arg_or_default(context, "depth_image_topic", f"{name}/stereo/image_raw")
    depth_camera_info_topic = _arg_or_default(context, "depth_camera_info_topic", f"{name}/stereo/camera_info")
    pointcloud_topic = _arg_or_default(context, "pointcloud_topic", f"{name}/points")

    rgb_frame_id = LaunchConfiguration("rgb_frame_id").perform(context)
    depth_frame_id = LaunchConfiguration("depth_frame_id").perform(context)

    parameter_overrides = {}
    if pointcloud_enable.perform(context) == "true":
        parameter_overrides = {
            "pipeline_gen": {"i_enable_sync": True},
            "rgb": {"i_synced": True},
            "stereo": {"i_synced": True},
        }

    if rgb_frame_id:
        parameter_overrides.setdefault("rgb", {})["i_frame_id"] = rgb_frame_id
    if depth_frame_id:
        parameter_overrides.setdefault("stereo", {})["i_frame_id"] = depth_frame_id

    return [
        ComposableNodeContainer(
            name=f"{name}_container",
            namespace=namespace,
            package="rclcpp_components",
            executable="component_container",
            composable_node_descriptions=[
                ComposableNode(
                    package="depthai_ros_driver",
                    plugin="depthai_ros_driver::Camera",
                    name=name,
                    namespace=namespace,
                    parameters=[params_file, parameter_overrides],
                    remappings=[
                        ("~/rgb/image_raw", rgb_image_topic),
                        ("~/rgb/camera_info", rgb_camera_info_topic),
                        ("~/stereo/image_raw", depth_image_topic),
                        ("~/stereo/camera_info", depth_camera_info_topic),
                    ],
                )
            ],
            output="both",
        ),
        LoadComposableNodes(
            condition=IfCondition(rectify_rgb),
            target_container=target_container,
            composable_node_descriptions=[
                ComposableNode(
                    package="image_proc",
                    plugin="image_proc::RectifyNode",
                    name="rectify_color_node",
                    namespace=namespace,
                    remappings=[
                        ("image", rgb_image_topic),
                        ("camera_info", rgb_camera_info_topic),
                        ("image_rect", rgb_rect_topic),
                    ],
                )
            ],
        ),
        LoadComposableNodes(
            condition=IfCondition(pointcloud_enable),
            target_container=target_container,
            composable_node_descriptions=[
                ComposableNode(
                    package="depth_image_proc",
                    plugin="depth_image_proc::PointCloudXyzrgbNode",
                    name="point_cloud_xyzrgb_node",
                    namespace=namespace,
                    remappings=[
                        ("depth_registered/image_rect", depth_image_topic),
                        ("rgb/image_rect_color", rgb_rect_topic),
                        ("rgb/camera_info", rgb_camera_info_topic),
                        ("points", pointcloud_topic),
                    ],
                ),
            ],
        ),
    ]


def generate_launch_description():
    depthai_prefix = get_package_share_directory("depthai_ros_driver")

    declared_arguments = [
        DeclareLaunchArgument("name", default_value="camera"),
        DeclareLaunchArgument("namespace", default_value=""),
        DeclareLaunchArgument(
            "params_file",
            default_value=os.path.join(depthai_prefix, "config", "camera.yaml"),
        ),
        DeclareLaunchArgument("rectify_rgb", default_value="true"),
        DeclareLaunchArgument("pointcloud_enable", default_value="false"),
        DeclareLaunchArgument("rgb_image_topic", default_value=""),
        DeclareLaunchArgument("rgb_camera_info_topic", default_value=""),
        DeclareLaunchArgument("rgb_rect_topic", default_value=""),
        DeclareLaunchArgument("depth_image_topic", default_value=""),
        DeclareLaunchArgument("depth_camera_info_topic", default_value=""),
        DeclareLaunchArgument("pointcloud_topic", default_value=""),
        DeclareLaunchArgument("rgb_frame_id", default_value=""),
        DeclareLaunchArgument("depth_frame_id", default_value=""),
    ]

    return LaunchDescription(declared_arguments + [OpaqueFunction(function=launch_setup)])
