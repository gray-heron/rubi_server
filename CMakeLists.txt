cmake_minimum_required(VERSION 3.5)
project(rubi_server)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(Boost REQUIRED)

add_library(rubi_server
  src/board.cpp
  src/can_handler.cpp
  src/communication.cpp
  src/descriptors.cpp
  src/logger.cpp
  src/protocol.cpp
  src/rubi_autodefs.cpp
  src/socketcan.cpp
)
target_include_directories(rubi_server
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(rubi_server
  rclcpp::rclcpp
)

add_executable(rubi_ros_server
  src/main.cpp
  src/ros_frontend.cpp
)
target_link_libraries(rubi_ros_server
  rubi_server
)

install(
  TARGETS
    rubi_server
  DESTINATION lib
)

install(
  TARGETS
    rubi_ros_server
  DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
