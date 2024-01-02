#pragma once

#include <string>
#include <thread>
#include <mutex>
#include <list>
#include <map>

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

    std::mutex m_pilotsLock;
    std::map<std::string, std::array<types::Pilot_t, 3>> m_pilots;

    void run();

  public:
    Airport(const std::string &airport);
    ~Airport();

    void pause();
    void resume();
    void resetData();

    const std::string getAirportIcao() const;

    void updateFromEuroscope(types::Pilot &pilot);
    /// @brief finds the pilot object using the callsign
    /// @param callsign
    /// @return the pilot data if found, a nullptr if the data could not be found
    const types::Pilot &getPilot(const std::string &callsign);
    bool pilotExists(const std::string &callsign);
  };
}