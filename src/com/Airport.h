#pragma once

#include <string>
#include <thread>
#include <mutex>
#include <list>

#include "types/Pilot.h"

namespace vacdm::com
{
  class Airport
  {
  private:
    enum class MessageType
    {
      InitialPilotData,
      TDPIn,
      TDPIt,
      TDPIs,
      ADPI,
      XDPIt,
      XDPIr,
    };

    struct AsyncMessage
    {
      MessageType type;
      types::Pilot pilot;
    };

    std::string m_airportIcao;
    std::thread m_worker;
    bool m_pause;
    bool m_stop;

    std::mutex m_asyncMessagesLock;
    std::list<AsyncMessage> m_asyncMessages;

    void run();

  public:
    Airport(const std::string &airport);
    ~Airport();

    void pause();
    void resume();

    const std::string getAirportIcao() const;
  };
}