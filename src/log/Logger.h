#pragma once

#include <string>
#include <mutex>

namespace vacdm::logging
{
  class Logger
  {
  public:
    enum class Level
    {
      Info,
      Disabled
    };

  private:
    Level m_minimumLevel;
    std::mutex m_mutex;
    Logger();

  public:
    ~Logger();
    void setMinimumLevel(Level level);
    void log(const std::string &sender, const std::string &message, Level level);
    static Logger &instance();
  };
}