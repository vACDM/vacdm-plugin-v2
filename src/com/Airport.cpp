#include "Airport.h"

using namespace std::chrono_literals;
using namespace vacdm::com;

Airport::Airport(const std::string &airportIcao) : m_airportIcao(airportIcao), m_worker(), m_pause(false),
                                                   m_stop(false),
                                                   m_asyncMessagesLock(),
                                                   m_asyncMessages()
{
  this->m_worker = std::thread(&Airport::run, this);
}

Airport::~Airport()
{
  this->m_stop = true;
  this->m_worker.join();
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
  }
}

const std::string Airport::getAirportIcao() const
{
  return this->m_airportIcao;
}