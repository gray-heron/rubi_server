#ifndef H_LOGGER
#define H_LOGGER

#include "types.h"
#include <string>

class Logger
{
  static int longest_module_name;
  std::string module;
  std::string GetSpacing();

public:
  Logger(std::string module_name);
  void Info(std::string);
  void Warning(std::string);
  void Error(std::string);
};

#endif