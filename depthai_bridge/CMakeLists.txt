cmake_minimum_required(VERSION 3.10.2)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project(depthai_bridge VERSION 2.12.2 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(POLICY CMP0057)
    cmake_policy(SET CMP0057 NEW)
endif()

set(opencv_version 4)
find_package(OpenCV ${opencv_version} QUIET COMPONENTS imgproc highgui calib3d)
if(NOT OpenCV_FOUND)
  message(STATUS "----------------Did not find OpenCV 4, trying OpenCV 3--------------")
  set(opencv_version 3)
  find_package(OpenCV ${opencv_version} REQUIRED COMPONENTS imgproc highgui calib3d)
endif()

message(STATUS "------------------------------------------")
message(STATUS "Depthai Bridge is being built using AMENT.")
message(STATUS "------------------------------------------")

set(BUILD_TOOL_INCLUDE_DIRS ${ament_INCLUDE_DIRS})
find_package(ament_cmake REQUIRED)

find_package(cv_bridge REQUIRED)
find_package(depthai CONFIG REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(ffmpeg_image_transport_msgs REQUIRED)

set(dependencies
  cv_bridge
  rclcpp
  sensor_msgs
  std_msgs
  ffmpeg_image_transport_msgs
)

include_directories(
    include
    ${BUILD_TOOL_INCLUDE_DIRS}
)

add_library(${PROJECT_NAME} SHARED
  src/ImageConverter.cpp
)

ament_target_dependencies(${PROJECT_NAME} ${dependencies})
target_compile_definitions(${PROJECT_NAME} PUBLIC IS_ROS2)

if($ENV{ROS_DISTRO} STREQUAL "humble")
  target_compile_definitions(${PROJECT_NAME} PRIVATE IS_HUMBLE)
endif()

target_link_libraries(${PROJECT_NAME}
                      depthai::core
                      opencv_imgproc
                      opencv_highgui
                      opencv_calib3d)

ament_export_targets(depthai_bridgeTargets HAS_LIBRARY_TARGET)

install(DIRECTORY include/
        DESTINATION include/
)

install(TARGETS depthai_bridge
      EXPORT depthai_bridgeTargets
              ARCHIVE DESTINATION lib
              LIBRARY DESTINATION lib
              RUNTIME DESTINATION bin
              INCLUDES DESTINATION include
)
install(EXPORT depthai_bridgeTargets
      DESTINATION share/${PROJECT_NAME}/cmake)

ament_export_include_directories(include)
ament_export_libraries(depthai_bridge)
ament_export_dependencies(${dependencies})

ament_package()
