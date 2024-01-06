#include "Logger.h"

#ifdef DEBUG_BUILD
#include <iostream>
#endif

#include <algorithm>

using namespace vacdm::logging;

Logger::Logger() : m_minimumLevel(Logger::Level::Info) {}

Logger::~Logger() {}

void Logger::setMinimumLevel(Logger::Level level)
{
  this->m_minimumLevel = level;
}

std::string Logger::handleLogLevelCommand(std::string level)
{
#pragma warning(push)
#pragma warning(disable : 4244)
  std::transform(level.begin(), level.end(), level.begin(), ::toupper);
#pragma warning(pop)

  std::string message = "";

  if (level == "INFO")
  {
    message = "Changed loglevel to INFO";
    setMinimumLevel(Logger::Level::Info);
  }
  else if (level == "UTILS")
  {
    message = "Changed loglevel to UTILS";
    setMinimumLevel(Logger::Level::Utils);
  }
  else if (level == "DISABLED" || level == "OFF")
  {
    message = "Changed loglevel to DISABLED";
    setMinimumLevel(Logger::Level::Disabled);
  }
  else
  {
    message = "Could not change loglevel, unknown loglevel";
  }

  return message;
}

void Logger::log(const std::string &sender, const std::string &message, Level level)
{
  if (sender.length() == 0 || message.length() == 0 || level < this->m_minimumLevel)
    return;

  std::lock_guard guard(this->m_mutex);

#ifdef DEBUG_BUILD
  std::cout << sender << ": " << message << "\n";
#endif
}

Logger &Logger::instance()
{
  static Logger __instance;
  return __instance;
}