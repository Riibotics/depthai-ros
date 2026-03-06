# DepthAI ROS2 Humble Minimal Workspace

This repository is trimmed for one purpose only:
- OAK-D image stream
- OAK-D depth stream
- OAK-D pointcloud stream

Kept packages:
- `depthai_bridge`
- `depthai_ros_driver`
- `depthai-ros` (metapackage)

## Build
```bash
./build.sh
```

## Run
```bash
# image + depth
ros2 launch depthai_ros_driver camera.launch.py

# image + depth + pointcloud
ros2 launch depthai_ros_driver pointcloud.launch.py
```

## Launch-Level Topic/Frame Customization
```bash
ros2 launch depthai_ros_driver pointcloud.launch.py \
  rgb_image_topic:=camera/front/color/image_raw \
  rgb_camera_info_topic:=camera/front/color/camera_info \
  rgb_rect_topic:=camera/front/color/image_rect \
  depth_image_topic:=camera/front/depth/image_raw \
  depth_camera_info_topic:=camera/front/depth/camera_info \
  pointcloud_topic:=camera/front/depth/points \
  rgb_frame_id:=front_color_optical_frame \
  depth_frame_id:=front_depth_optical_frame
```

## Lifecycle + Diagnostics
```bash
# Lifecycle mode (camera + lifecycle manager)
ros2 launch depthai_ros_driver camera_lifecycle.launch.py

# Manual transitions (if autostart:=false)
ros2 lifecycle set /camera_lifecycle_manager configure
ros2 lifecycle set /camera_lifecycle_manager activate
ros2 lifecycle set /camera_lifecycle_manager deactivate
```

Diagnostics are published on:
- `/diagnostics` (camera sys logger + lifecycle manager status)
