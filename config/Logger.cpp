#include "Logger.hpp"

Logger::Logger() {
  Logger::log(LogLevel::INFO, "LOG START");
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  std::tm tm = *std::localtime(&time);

  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

  Logger::log(LogLevel::INFO, oss.str());

}

Logger & Logger::getInstance(){
  static Logger log;
  return log;
}

void Logger::log(LogLevel lvl, std::string message) {
  if (!enableValidationLayers && lvl == LogLevel::DEBUG) {
    return;
  }

  std::string colorCode, levelStr;
  Logger::getOutputFormat(lvl, levelStr, colorCode);
  std::cout << colorCode << "[" << levelStr << "]: " << message << "\033[0m"
            << std::endl;
}

void Logger::log(LogLevel lvl, std::vector<std::string> messages) {
  if (!enableValidationLayers && lvl == LogLevel::DEBUG) {
    return;
  }
  std::string colorCode, levelStr;
  Logger::getOutputFormat(lvl, levelStr, colorCode);
  for (std::string &message : messages) {
    std::cout << colorCode << "[" << levelStr << "]: " << message << "\033[0m"
              << std::endl;
  }
}

void Logger::getOutputFormat(const LogLevel &lvl, std::string &levelStr,
                             std::string &colorCode) {
  switch (lvl) {
  case LogLevel::DEBUG:
    levelStr = "DEBUG";
    colorCode = "\033[36m"; // Cyan
    break;
  case LogLevel::INFO:
    levelStr = "INFO";
    colorCode = "\033[32m"; // Green
    break;
  case LogLevel::WARNING:
    levelStr = "WARNING";
    colorCode = "\033[33m"; // Yellow
    break;
  case LogLevel::ERROR:
    levelStr = "ERROR";
    colorCode = "\033[31m"; // Red
    break;
  }
}
