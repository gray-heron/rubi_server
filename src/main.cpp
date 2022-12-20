#include <cstring>
#include <vector>

#include "rubi_server/board.hpp"
#include "rubi_server/exceptions.hpp"
#include "rubi_server/ros_frontend.hpp"
#include "rubi_server/socketcan.hpp"

using std::vector;
using std::string;

int main(int argc, char ** argv)
{
  sptr<RosModule> frontend = std::make_shared<RosModule>();
  BoardManager::inst().frontend = frontend;

  frontend->Init(argc, argv);
  BoardManager::inst().Init(frontend->GetCansNames());

  while (!frontend->Quit()) {
    auto time_now = std::chrono::system_clock::now();
    BoardManager::inst().Spin(time_now);
    frontend->Spin();
  }
}
