#include <boost/algorithm/string.hpp>
#include <cstring>
#include <vector>

#include "board.h"
#include "exceptions.h"
#include "ros_frontend.h"
#include "socketcan.h"

using std::vector;
using std::string;

int main(int argc, char **argv)
{
    vector<string> cans;
    stringmap args;

    if (!parseArguments("--", '=', argc, argv, args))
        Log::Error("Error occured while parsing input arguments!");

    if (args.find("cans") == args.end())
        ASSERT(0, "No can interfaces specified!");

    boost::split(cans, args["cans"], boost::is_any_of(","));

    sptr<RosModule> frontend = std::make_shared<RosModule>();
    frontend->Init(args, cans);

    BoardManager::inst().frontend = frontend;
    BoardManager::inst().Init(cans);

    while (1)
    {
        auto time_now = std::chrono::system_clock::now();
        BoardManager::inst().Spin(time_now);
        frontend->Spin();
    }
}
