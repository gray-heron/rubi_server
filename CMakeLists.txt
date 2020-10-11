cmake_minimum_required(VERSION 3.0.2)
project(rubi_server)

find_package(catkin REQUIRED COMPONENTS
  roscpp
  std_msgs
  message_generation
)

find_package(Boost REQUIRED)

add_message_files(
  DIRECTORY msg
  FILES
  BoardAnnounce.msg
  RubiInt.msg
  RubiUnsignedInt.msg
  RubiString.msg
  RubiFloat.msg
  RubiBool.msg
)

add_service_files(
  DIRECTORY srv
  FILES
  CansNames.srv
  FieldDescriptor.srv
  FuncDescriptor.srv
  BoardDescriptor.srv
  BoardInstances.srv
  ShowBoards.srv
  BoardOnline.srv
  BoardWake.srv
)

generate_messages(DEPENDENCIES std_msgs)

catkin_package()

include_directories(
  ${Boost_INCLUDE_DIRS}
  ${catkin_INCLUDE_DIRS}
)

add_executable(rubi_server
  src/board.cpp src/communication.cpp src/can_handler.cpp src/descriptors.cpp
  src/main.cpp src/protocol.cpp src/ros_frontend.cpp
  src/rubi_autodefs.cpp src/socketcan.cpp
  src/logger.cpp
)

add_executable(rubi_fake_server
  src/fake_communication.cpp src/descriptors.cpp
  src/fake_server.cpp src/ros_frontend.cpp src/rubi_autodefs.cpp
  src/logger.cpp
)

add_dependencies(rubi_server rubi_server_generate_messages_cpp)
add_dependencies(rubi_fake_server rubi_server_generate_messages_cpp)

target_link_libraries(rubi_server ${catkin_LIBRARIES})
target_link_libraries(rubi_fake_server ${catkin_LIBRARIES})

install(TARGETS
  rubi_server 
  rubi_fake_server
  ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)
