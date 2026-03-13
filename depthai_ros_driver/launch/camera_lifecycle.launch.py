import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, EmitEvent, OpaqueFunction, RegisterEventHandler, TimerAction
from launch.conditions import IfCondition
from launch.events import matches_action
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import LifecycleNode, Node
from launch_ros.event_handlers import OnStateTransition
from launch_ros.events.lifecycle import ChangeState
from lifecycle_msgs.msg import Transition


def _arg_or_default(context, arg_name, default_value):
    value = LaunchConfiguration(arg_name).perform(context)
    return value if value else default_value


def launch_setup(context, *args, **kwargs):
    name = LaunchConfiguration("name")
    name_value = name.perform(context)
    namespace = LaunchConfiguration("namespace")
    params_file = LaunchConfiguration("params_file")
    autostart = LaunchConfiguration("autostart")
    camera_ip = LaunchConfiguration("camera_ip").perform(context)

    rgb_image_topic = _arg_or_default(context, "rgb_image_topic", f"{name_value}/rgb/image_raw")
    rgb_camera_info_topic = _arg_or_default(context, "rgb_camera_info_topic", f"{name_value}/rgb/camera_info")
    depth_image_topic = _arg_or_default(context, "depth_image_topic", f"{name_value}/stereo/image_raw")
    depth_camera_info_topic = _arg_or_default(context, "depth_camera_info_topic", f"{name_value}/stereo/camera_info")

    rgb_frame_id = LaunchConfiguration("rgb_frame_id").perform(context)
    depth_frame_id = LaunchConfiguration("depth_frame_id").perform(context)

    parameter_overrides = {"camera.i_auto_start": False}
    if camera_ip:
        parameter_overrides["camera.i_ip"] = camera_ip
    if rgb_frame_id:
        parameter_overrides["rgb.i_frame_id"] = rgb_frame_id
    if depth_frame_id:
        parameter_overrides["stereo.i_frame_id"] = depth_frame_id

    camera_node = Node(
        package="depthai_ros_driver",
        executable="camera_node",
        name=name,
        namespace=namespace,
        output="screen",
        parameters=[params_file, parameter_overrides],
        remappings=[
            ("~/rgb/image_raw", rgb_image_topic),
            ("~/rgb/camera_info", rgb_camera_info_topic),
            ("~/stereo/image_raw", depth_image_topic),
            ("~/stereo/camera_info", depth_camera_info_topic),
        ],
    )

    lifecycle_manager = LifecycleNode(
        package="depthai_ros_driver",
        executable="camera_lifecycle_node",
        name=[name, "_lifecycle_manager"],
        namespace=namespace,
        output="screen",
        parameters=[
            {"camera_name": name},
            {"service_wait_timeout_sec": 10.0},
            {"diagnostic_period_sec": 1.0},
            {"stale_timeout_sec": 3.0},
        ],
    )

    configure_event = EmitEvent(
        event=ChangeState(
            lifecycle_node_matcher=matches_action(lifecycle_manager),
            transition_id=Transition.TRANSITION_CONFIGURE,
        ),
        condition=IfCondition(autostart),
    )

    activate_event = EmitEvent(
        event=ChangeState(
            lifecycle_node_matcher=matches_action(lifecycle_manager),
            transition_id=Transition.TRANSITION_ACTIVATE,
        ),
        condition=IfCondition(autostart),
    )

    activate_on_inactive = RegisterEventHandler(
        OnStateTransition(
            target_lifecycle_node=lifecycle_manager,
            start_state="configuring",
            goal_state="inactive",
            entities=[activate_event],
        ),
        condition=IfCondition(autostart),
    )

    delayed_configure = TimerAction(period=2.0, actions=[configure_event], condition=IfCondition(autostart))

    return [
        camera_node,
        lifecycle_manager,
        activate_on_inactive,
        delayed_configure,
    ]


def generate_launch_description():
    depthai_prefix = get_package_share_directory("depthai_ros_driver")

    return LaunchDescription(
        [
            DeclareLaunchArgument("name", default_value="fork_camera"),
            DeclareLaunchArgument("namespace", default_value=""),
            DeclareLaunchArgument("camera_ip", default_value="169.254.1.222"),
            DeclareLaunchArgument(
                "params_file",
                default_value=os.path.join(depthai_prefix, "config", "camera.yaml"),
            ),
            DeclareLaunchArgument("autostart", default_value="false"),
            DeclareLaunchArgument("rgb_image_topic", default_value="/fork_camera/rgb"),
            DeclareLaunchArgument("rgb_camera_info_topic", default_value="/fork_camera/rgb_info"),
            DeclareLaunchArgument("depth_image_topic", default_value="/fork_camera/depth"),
            DeclareLaunchArgument("depth_camera_info_topic", default_value="/fork_camera/depth_info"),
            DeclareLaunchArgument("rgb_frame_id", default_value="fork_camera_link"),
            DeclareLaunchArgument("depth_frame_id", default_value="fork_camera_link"),
            OpaqueFunction(function=launch_setup),
        ]
    )
