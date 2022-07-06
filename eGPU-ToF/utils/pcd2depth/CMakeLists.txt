cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(pcd2depth)

find_package(PCL 1.2 REQUIRED)
find_package(OpenCV REQUIRED)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


include_directories(${PCL_INCLUDE_DIRS})
include_directories( ${OpenCV_INCLUDE_DIRS} )
link_directories(${PCL_LIBRARY_DIRS})
add_definitions(${PCL_DEFINITIONS})

add_executable (pcd2depth src/pcd2depth.cpp)
target_link_libraries (pcd2depth ${OpenCV_LIBS} ${PCL_LIBRARIES})
