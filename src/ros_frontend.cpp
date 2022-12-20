#include <functional>
#include <thread>

#include <boost/algorithm/string.hpp>

#include "rubi_server/descriptors.hpp"
#include "rubi_server/exceptions.hpp"
#include "rubi_server/ros_frontend.hpp"
#include "rubi_server/rubi_autodefs.hpp"

#include "rclcpp/rclcpp.hpp"

RosModule::RosModule() {}

bool RosModule::Init(int argc, char ** argv)
{
  return true;
}

std::vector<std::string> RosModule::GetCansNames() {return cans_names;}

void RosModule::Spin() {}

bool RosModule::Quit() {return false;}

void RosModule::LogInfo(string msg) {std::cout << msg << std::endl;}

void RosModule::LogWarning(string msg) {std::cout << msg << std::endl;}

void RosModule::LogError(string msg) {std::cout << msg << std::endl;}

void RosModule::ReportCansUtilization(std::vector<float> util) {}

std::shared_ptr<FrontendBoardHandler> RosModule::NewBoard(BoardInstance inst)
{
  return std::make_shared<RosBoardHandler>(inst, this);
}

RosBoardHandler::RosBoardHandler(BoardInstance inst, RosModule * ros_module)
: ros_module(ros_module), board(inst) {}
void RosBoardHandler::FFDataInbound(std::vector<uint8_t> & data, int ffid) {}
void RosBoardHandler::ReplaceBackendHandler(sptr<BoardCommunicationHandler>) {}
void RosBoardHandler::Shutdown() {}
void RosBoardHandler::ConnectionLost() {}
