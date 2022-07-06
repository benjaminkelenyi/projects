cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(depth2pcd)

find_package(PCL 1.2 REQUIRED)
find_package(OpenCV REQUIRED)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


include_directories(${PCL_INCLUDE_DIRS})
include_directories( ${OpenCV_INCLUDE_DIRS} )
link_directories(${PCL_LIBRARY_DIRS})
add_definitions(${PCL_DEFINITIONS})

add_executable (depth2pcd src/depth2pcd.cpp)
target_link_libraries (depth2pcd ${OpenCV_LIBS} ${PCL_LIBRARIES})
