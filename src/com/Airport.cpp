#include "Airport.h"

#include "log/Logger.h"

using namespace std::chrono_literals;
using namespace vacdm::com;

/// @brief Manages all flights and data for one airport. Updates data on specified interval
/// @param airportIcao The airport name in ICAO format
Airport::Airport(const std::string &airportIcao) : m_airportIcao(airportIcao), m_worker(), m_pause(false),
                                                   m_stop(false),
                                                   m_asyncMessagesLock(),
                                                   m_asyncMessages(),
                                                   m_pilots(),
                                                   m_pilotsLock()
{
  this->m_worker = std::thread(&Airport::run, this);
}

Airport::~Airport()
{
  this->m_stop = true;
  this->m_worker.join();
}

void Airport::pause()
{
  this->m_pause = true;
}

void Airport::resume()
{
  this->m_pause = false;
}

void Airport::resetData()
{
  std::lock_guard guard(this->m_pilotsLock);
  this->m_pilots.clear();
}

void Airport::run()
{
  std::size_t counter = 1;
  while (true)
  {
    std::this_thread::sleep_for(1s);
    if (this->m_stop == true)
      return;
    if (this->m_pause == true)
      return;

    // run every five seconds
    if (counter++ % 5 != 0)
    {
      continue;
    }

    logging::Logger::instance().log("Airport " + this->m_airportIcao, " run", logging::Logger::Level::Info);
  }
}

const std::string Airport::getAirportIcao() const
{
  return this->m_airportIcao;
}