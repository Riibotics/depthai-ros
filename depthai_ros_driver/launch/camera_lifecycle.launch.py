import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, EmitEvent, RegisterEventHandler, TimerAction
from launch.conditions import IfCondition
from launch.events import matches_action
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import LifecycleNode, Node
from launch_ros.event_handlers import OnStateTransition
from launch_ros.events.lifecycle import ChangeState
from lifecycle_msgs.msg import Transition


def generate_launch_description():
    depthai_prefix = get_package_share_directory("depthai_ros_driver")

    name = LaunchConfiguration("name")
    namespace = LaunchConfiguration("namespace")
    params_file = LaunchConfiguration("params_file")
    autostart = LaunchConfiguration("autostart")

    camera_node = Node(
        package="depthai_ros_driver",
        executable="camera_node",
        name=name,
        namespace=namespace,
        output="screen",
        parameters=[params_file, {"camera.i_auto_start": False}],
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

    return LaunchDescription(
        [
            DeclareLaunchArgument("name", default_value="camera"),
            DeclareLaunchArgument("namespace", default_value=""),
            DeclareLaunchArgument(
                "params_file",
                default_value=os.path.join(depthai_prefix, "config", "camera.yaml"),
            ),
            DeclareLaunchArgument("autostart", default_value="true"),
            camera_node,
            lifecycle_manager,
            activate_on_inactive,
            delayed_configure,
        ]
    )
