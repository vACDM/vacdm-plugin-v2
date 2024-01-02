#include "Airport.h"

#include "log/Logger.h"

using namespace std::chrono_literals;
using namespace vacdm;
using namespace vacdm::com;

static constexpr std::size_t PilotConsolidated = 0;
static constexpr std::size_t PilotEuroscope = 1;
static constexpr std::size_t PilotServer = 2;

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

void Airport::updateFromEuroscope(types::Pilot &pilot)
{
  if (this->m_pause == true)
    return;

  std::lock_guard guard(this->m_pilotsLock);

  bool found = false;
  for (auto &pair : this->m_pilots)
  {
    if (pilot.callsign == pair.first)
    {
      const auto oldUpdateTime = pair.second[PilotEuroscope].lastUpdate;
      pair.second[PilotEuroscope] = pilot;
      pair.second[PilotEuroscope].lastUpdate = oldUpdateTime;
      found = true;
      break;
    }
  }

  if (found == false)
  {
    pilot.lastUpdate = std::chrono::utc_clock::now();
    this->m_pilots.insert({pilot.callsign, {pilot, pilot, types::Pilot()}});
  }
}

const types::Pilot &Airport::getPilot(const std::string &callsign)
{
  std::lock_guard guard(this->m_pilotsLock);
  return this->m_pilots.find(callsign)->second[PilotConsolidated];
}

bool Airport::pilotExists(const std::string &callsign)
{
  if (true == this->m_pause)
    return false;

  std::lock_guard guard(this->m_pilotsLock);
  return this->m_pilots.cend() != this->m_pilots.find(callsign);
}