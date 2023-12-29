#include "vACDM.h"
#include "Version.h"

#include <numeric>
#include <Windows.h>
#include <shlwapi.h>

#include "com/Server.h"
#include "config/ConfigParser.h"
#include "utils/String.h"

#include "log/Logger.h"

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

namespace vacdm
{
  vACDM::vACDM() : CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE, PLUGIN_NAME, PLUGIN_VERSION, PLUGIN_AUTHOR, PLUGIN_LICENSE)
  {
#ifdef DEBUG_BUILD
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
#endif
    DisplayMessage("Version " + std::string(PLUGIN_VERSION) + " loaded",
                   "Initialisation");

    /* get DLL path */
    char path[MAX_PATH + 1] = {0};
    GetModuleFileNameA((HINSTANCE)&__ImageBase, path, MAX_PATH);
    PathRemoveFileSpecA(path);
    this->m_dllPath = std::string(path);

    this->reloadConfiguration();

    logging::Logger::instance().log("Initialisation", std::string(PLUGIN_VERSION), logging::Logger::Level::Info);
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

  void vACDM::reloadConfiguration()
  {
    PluginConfig newConfig;
    ConfigParser parser;

    if (false == parser.parse(this->m_dllPath + this->m_configFileName, newConfig) || false == newConfig.valid)
    {
      std::string message = "vacdm.txt:" + std::to_string(parser.errorLine()) + ": " + parser.errorMessage();
      DisplayMessage(message, "Config");
    }
    else
    {
      DisplayMessage("Reloaded the config", "Config");
      if (this->m_pluginConfig.serverUrl != newConfig.serverUrl)
        this->changeServerUrl(newConfig.serverUrl);
      this->m_pluginConfig = newConfig;
    }
  }

  void vACDM::changeServerUrl(const std::string &url)
  {
    // pause airport updates and reset internal data
    this->m_airportLock.lock();
    for (auto &airport : this->m_airports)
    {
      airport->pause();
      airport->resetData();
    }
    this->m_airportLock.unlock();

    com::Server::instance().changeServerAddress(url);
    this->checkServerConfiguration();

    // reenable airports
    this->m_airportLock.lock();
    for (auto &airport : this->m_airports)
      airport->resume();
    this->m_airportLock.unlock();
  }

  void vACDM::runEuroscopeUpdateCycle()
  {
    for (EuroScopePlugIn::CFlightPlan flightplan = FlightPlanSelectFirst(); flightplan.IsValid(); flightplan = FlightPlanSelectNext(flightplan))
      this->updatePilotData(flightplan);
  }

  void vACDM::updatePilotData(const EuroScopePlugIn::CFlightPlan &flightplan)
  {
    // skip all non-IFR flights
    if (flightplan.GetFlightPlanData().GetPlanType() != std::string_view("I"))
      return;

    // check if the aircraft is departing from an active airport, return if not
    bool foundAirport = false;
    for (auto &airport : this->m_airports)
    {
      if (airport->getAirportIcao() == flightplan.GetFlightPlanData().GetOrigin())
      {
        foundAirport = true;
        break;
      }
    }
    if (!foundAirport)
      return;

    types::Pilot pilot;

    // required post message items:
    pilot.callsign = flightplan.GetCallsign();
    pilot.latitude = flightplan.GetFPTrackPosition().GetPosition().m_Latitude;
    pilot.longitude = flightplan.GetFPTrackPosition().GetPosition().m_Longitude;
    pilot.origin = flightplan.GetFlightPlanData().GetOrigin();
    pilot.destination = flightplan.GetFlightPlanData().GetDestination();
    pilot.eobt = vacdm::utils::convertEuroscopeDepartureTime(flightplan);
    pilot.tobt = pilot.eobt;
    pilot.runway = flightplan.GetFlightPlanData().GetDepartureRwy();
    pilot.sid = flightplan.GetFlightPlanData().GetSidName();

    std::lock_guard guard(this->m_airportLock);
    for (auto &airport : this->m_airports)
    {
      if (airport->getAirportIcao() == pilot.origin)
      {
        airport->updateFromEuroscope(pilot);
        logging::Logger::instance().log("ES-UpdateCycle", "Updated: " + pilot.callsign, logging::Logger::Level::Info);
        return;
      }
    }
  }

  // Euroscope Events:

  void vACDM::OnTimer(const int Counter)
  {
    // run update cycle every 5 seconds
    if (Counter % 5 == 0)
      this->runEuroscopeUpdateCycle();
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
      std::string airportIcao = vacdm::utils::String::trim(rwy.GetAirportName());

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