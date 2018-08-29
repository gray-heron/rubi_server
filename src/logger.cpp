#include "logger.h"
#include "board.h"

int Logger::longest_module_name = 0;

Logger::Logger(std::string module_name) : module(module_name)
{
    longest_module_name =
        std::max(longest_module_name, (int)module_name.length());
}

std::string Logger::GetSpacing()
{
    return std::string(Logger::longest_module_name - module.length() + 1, ' ');
}

void Logger::Info(std::string msg)
{

    BoardManager::inst().frontend->LogInfo("[" + module + "]" + GetSpacing() +
                                           msg);
}

void Logger::Warning(std::string msg)
{
    BoardManager::inst().frontend->LogWarning("[" + module + "]" +
                                              GetSpacing() + msg);
}

void Logger::Error(std::string msg)
{
    BoardManager::inst().frontend->LogError("[" + module + "]" + GetSpacing() +
                                            msg);
}