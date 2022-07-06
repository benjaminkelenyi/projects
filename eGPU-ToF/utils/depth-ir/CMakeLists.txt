cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(depth_ir)

find_package(PCL 1.7 REQUIRED)

find_package(catkin REQUIRED COMPONENTS
  cv_bridge
  image_transport
  pcl_conversions
  pcl_ros
  roscpp
  sensor_msgs
)

include_directories(
# include
  ${catkin_INCLUDE_DIRS}
  ${PCL_INCLUDE_DIRS}
  ${OpenCV_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}/include
)

link_directories(${PCL_LIBRARY_DIRS})
add_definitions(${PCL_DEFINITIONS})

add_executable (depth_ir ./depth_ir/src/main.cpp)
target_link_libraries (depth_ir ${PCL_LIBRARIES})
