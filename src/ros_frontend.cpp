#include <boost/algorithm/string.hpp>
#include <functional>
#include <thread>

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

void RosModule::LogInfo(string msg) {std::cout << msg << std::endl;}

void RosModule::LogWarning(string msg) {std::cout << msg << std::endl;}

void RosModule::LogError(string msg) {std::cout << msg << std::endl;}

std::shared_ptr<FrontendBoardHandler> RosModule::NewBoard(BoardInstance inst)
{
  return std::make_shared<RosBoardHandler>(inst, this);
}

std::vector<std::string> RosModule::GetCansNames() {return cans_names;}

void RosModule::Spin()
{

}

bool RosModule::Quit() {return false;}


void RosModule::ReportCansUtilization(std::vector<float> util)
{

}
