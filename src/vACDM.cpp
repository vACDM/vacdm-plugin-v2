#include "vACDM.h"
#include "Version.h"

#include <numeric>

#include "com/Server.h"
#include "utils/trimString.h"

#ifdef DEBUG_BUILD
#include <iostream>
#endif

namespace vacdm
{
  vACDM::vACDM() : CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE, PLUGIN_NAME, PLUGIN_VERSION, PLUGIN_AUTHOR, PLUGIN_LICENSE)
  {
    DisplayMessage("Version " + std::string(PLUGIN_VERSION) + " loaded", "Initialisation");
    this->checkServerConfiguration();

    #ifdef DEBUG_BUILD
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    #endif
  }
  vACDM::~vACDM()
  {
  }

  void vACDM::checkServerConfiguration()
  {
    if (com::Server::instance().checkWebApi() == false)
    {
      DisplayMessage("Connection failed.", "Server");
      DisplayMessage(com::Server::instance().errorMessage().c_str(), "Server");
    }
    else
    {
      std::string serverName = com::Server::instance().getServerConfig().name;
      std::list<std::string> supportedAirports = com::Server::instance().getServerConfig().supportedAirports;
      DisplayMessage(("Connected to " + serverName), "Server");
      DisplayMessage("ACDM available for: " + std::accumulate(
                                                  std::next(supportedAirports.begin()), supportedAirports.end(),
                                                  supportedAirports.front(),
                                                  [](const std::string &acc, const std::string &str)
                                                  {
                                                    return acc + " " + str;
                                                  }),
                     "Server");
      // set active airports and runways
      this->OnAirportRunwayActivityChanged();
    }
  }

  void vACDM::DisplayMessage(const std::string &message, const std::string &sender)
  {
    DisplayUserMessage("vACDM", sender.c_str(), message.c_str(), true, false, false, false, false);
  }

  void vACDM::OnAirportRunwayActivityChanged()
  {
    std::list<std::string> activeAirports;
    m_activeRunways.clear();

    // loop through all defined runways, get active airports and runways
    EuroScopePlugIn::CSectorElement rwy;
    for (rwy = this->SectorFileElementSelectFirst(EuroScopePlugIn::SECTOR_ELEMENT_RUNWAY); rwy.IsValid() == true; rwy = this->SectorFileElementSelectNext(rwy, EuroScopePlugIn::SECTOR_ELEMENT_RUNWAY))
    {
      // trimmed airport ICAO, as GetAirportName() returns names with empty spaces at the end
      std::string airportIcao = vacdm::utils::trim(rwy.GetAirportName());

      // if one of the selected runways is active as departure runway
      if (rwy.IsElementActive(true, 0) || rwy.IsElementActive(true, 1))
      {
        // add airport to m_activeRunways map if it does not exist
        if (m_activeRunways.find(airportIcao) == m_activeRunways.end())
        {
          m_activeRunways.insert({airportIcao, {}});
        }
        // add the active runway
        if (rwy.IsElementActive(true, 0))
          m_activeRunways[airportIcao].push_back(rwy.GetRunwayName(0));
        if (rwy.IsElementActive(true, 1))
          m_activeRunways[airportIcao].push_back(rwy.GetRunwayName(1));

        // add mark as active airport
        if (std::find(activeAirports.begin(), activeAirports.end(), airportIcao) == activeAirports.end())
          activeAirports.push_back(airportIcao);
      }
    }

    // update m_airports map
    std::lock_guard guard(this->m_airportLock);
    for (auto it = this->m_airports.begin(); this->m_airports.end() != it;)
    {
      bool isDeselectedAirport = std::find(activeAirports.begin(), activeAirports.end(), (*it)->getAirportIcao()) == activeAirports.end();
      if (isDeselectedAirport)
      {
        // remove airport if it has been deselected
        it = this->m_airports.erase(it);
      }
      else
      {
        // remove airport from activeAirports list if it already exists, so the list can be used to create missing airport objects
        activeAirports.remove_if([it](const std::string &airport)
                                 { return (*it)->getAirportIcao() == airport; });
        ++it;
      }
    }

    // create missing airport objects
    std::list<std::string> supportedAirports = com::Server::instance().getServerConfig().supportedAirports;
    for (const auto &icao : std::as_const(activeAirports))
    {
      // check if backend supports airport
      for (const std::string &supportedAirport : supportedAirports)
      {
        if (supportedAirport == icao)
        {
          this->m_airports.push_back(std::unique_ptr<com::Airport>(new com::Airport(icao)));
          break;
        }
      }
    }
  }
}