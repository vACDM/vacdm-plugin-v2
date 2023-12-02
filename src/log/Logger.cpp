#include "Logger.h"

#ifdef DEBUG_BUILD
#include <iostream>
#endif

using namespace vacdm::logging;

Logger::Logger() : m_minimumLevel(Logger::Level::Info) {}

Logger::~Logger() {}

void Logger::setMinimumLevel(Logger::Level level)
{
  this->m_minimumLevel = level;
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