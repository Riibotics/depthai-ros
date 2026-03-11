import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration


def generate_launch_description():
    depthai_prefix = get_package_share_directory("depthai_ros_driver")

    return LaunchDescription(
        [
            DeclareLaunchArgument("name", default_value="camera"),
            DeclareLaunchArgument("namespace", default_value=""),
            DeclareLaunchArgument(
                "params_file",
                default_value=os.path.join(depthai_prefix, "config", "pcl.yaml"),
            ),
            DeclareLaunchArgument("rgb_image_topic", default_value=""),
            DeclareLaunchArgument("rgb_camera_info_topic", default_value=""),
            DeclareLaunchArgument("rgb_rect_topic", default_value=""),
            DeclareLaunchArgument("depth_image_topic", default_value=""),
            DeclareLaunchArgument("depth_camera_info_topic", default_value=""),
            DeclareLaunchArgument("pointcloud_topic", default_value=""),
            DeclareLaunchArgument("rgb_frame_id", default_value=""),
            DeclareLaunchArgument("depth_frame_id", default_value=""),
            IncludeLaunchDescription(
                PythonLaunchDescriptionSource(
                    os.path.join(depthai_prefix, "launch", "camera.launch.py")
                ),
                launch_arguments={
                    "name": LaunchConfiguration("name"),
                    "namespace": LaunchConfiguration("namespace"),
                    "params_file": LaunchConfiguration("params_file"),
                    "rectify_rgb": "true",
                    "pointcloud_enable": "true",
                    "rgb_image_topic": LaunchConfiguration("rgb_image_topic"),
                    "rgb_camera_info_topic": LaunchConfiguration("rgb_camera_info_topic"),
                    "rgb_rect_topic": LaunchConfiguration("rgb_rect_topic"),
                    "depth_image_topic": LaunchConfiguration("depth_image_topic"),
                    "depth_camera_info_topic": LaunchConfiguration("depth_camera_info_topic"),
                    "pointcloud_topic": LaunchConfiguration("pointcloud_topic"),
                    "rgb_frame_id": LaunchConfiguration("rgb_frame_id"),
                    "depth_frame_id": LaunchConfiguration("depth_frame_id"),
                }.items(),
            ),
        ]
    )
