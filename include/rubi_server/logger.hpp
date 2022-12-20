#pragma once

#include <string>

#include "rubi_server/types.hpp"

class Logger
{
  static int longest_module_name;
  std::string module;
  std::string GetSpacing();

public:
  explicit Logger(std::string module_name);
  void Info(std::string);
  void Warning(std::string);
  void Error(std::string);
};
