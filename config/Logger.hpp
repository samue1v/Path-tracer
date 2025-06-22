#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <chrono>
#include "config.hpp"

class Logger {
public:
  enum class LogLevel { DEBUG, INFO, WARNING, ERROR };

public:

  static void log(LogLevel lvl, std::string );
  static void log(LogLevel lvl, std::vector<std::string>);
  static Logger & getInstance();

private: 
  Logger();
  static void getOutputFormat(const LogLevel & lvl, std::string & levelStr, std::string &colorStr);
};

#endif
