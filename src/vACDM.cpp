#include "vACDM.h"
#include "Version.h"

#include <numeric>
#include <Windows.h>
#include <shlwapi.h>
#include <algorithm>

#include "com/Server.h"
#include "config/ConfigParser.h"
#include "core/TagItems.h"
#include "utils/String.h"

#include "log/Logger.h"

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

namespace vacdm
{
  vACDM::vACDM() : CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE, PLUGIN_NAME, PLUGIN_VERSION, PLUGIN_AUTHOR, PLUGIN_LICENSE)
  {
#ifdef DEBUG_BUILD
    AllocConsole();
#pragma warning(push)
#pragma warning(disable : 6031)
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
#pragma warning(pop)
#endif
    logging::Logger::instance().log("Initialisation", std::string(PLUGIN_VERSION), logging::Logger::Level::Info);
    DisplayMessage("Version " + std::string(PLUGIN_VERSION) + " loaded",
                   "Initialisation");

    /* get DLL path */
    char path[MAX_PATH + 1] = {0};
    GetModuleFileNameA((HINSTANCE)&__ImageBase, path, MAX_PATH);
    PathRemoveFileSpecA(path);
    this->m_dllPath = std::string(path);

    this->reloadConfiguration();
    this->RegisterTagItems();
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

  bool vACDM::OnCompileCommand(const char *sCommandLine)
  {
    std::string command(sCommandLine);

#pragma warning(push)
#pragma warning(disable : 4244)
    std::transform(command.begin(), command.end(), command.begin(), ::toupper);
#pragma warning(pop)

    // only handle commands containing ".vacdm"
    if (0 != command.find(".VACDM"))
      return false;

    // master command
    if (std::string::npos != command.find("MASTER"))
    {
      bool userIsConnected = this->GetConnectionType() != EuroScopePlugIn::CONNECTION_TYPE_NO;
      bool userIsInSweatbox = this->GetConnectionType() == EuroScopePlugIn::CONNECTION_TYPE_SWEATBOX;
      bool userIsObserver = std::string_view(this->ControllerMyself().GetCallsign()).ends_with("_OBS") == true || this->ControllerMyself().GetFacility() == 0;
      bool serverAllowsObsAsMaster = com::Server::instance().getServerConfig().allowMasterAsObserver;
      bool serverAllowsSweatboxAsMaster = com::Server::instance().getServerConfig().allowMasterInSweatbox;

      std::string userIsNotEligibleMessage;

      if (!userIsConnected)
      {
        userIsNotEligibleMessage = "You are not logged in to the VATSIM network";
      }
      else if (userIsObserver && !serverAllowsObsAsMaster)
      {
        userIsNotEligibleMessage = "You are logged in as Observer and Server does not allow Observers to be Master";
      }
      else if (userIsInSweatbox && !serverAllowsSweatboxAsMaster)
      {
        userIsNotEligibleMessage = "You are logged in on a Sweatbox Server and Server does not allow Sweatbox connections";
      }
      else
      {
        DisplayMessage("Executing vACDM as the MASTER");
        logging::Logger::instance().log("vACDM", "Switched to MASTER", logging::Logger::Level::Info);
        com::Server::instance().setMaster(true);

        return true;
      }

      DisplayMessage("Cannot upgrade to Master");
      DisplayMessage(userIsNotEligibleMessage);
      return true;
    }
    else if (std::string::npos != command.find("SLAVE"))
    {
      DisplayMessage("Executing vACDM as the SLAVE");
      logging::Logger::instance().log("vACDM", "Switched to SLAVE", logging::Logger::Level::Info);
      com::Server::instance().setMaster(false);
      return true;
    }
    else if (std::string::npos != command.find("RELOAD"))
    {
      this->reloadConfiguration();
      return true;
    }
    else if (std::string::npos != command.find("LOGLEVEL"))
    {
      const auto elements = vacdm::utils::String::splitString(command, " ");
      if (elements.size() == 3)
      {
        DisplayMessage(logging::Logger::instance().handleLogLevelCommand(elements[2]));
        return true;
      }
    }
    return false;
  }

  void vACDM::OnGetTagItem(EuroScopePlugIn::CFlightPlan FlightPlan, EuroScopePlugIn::CRadarTarget RadarTarget, int ItemCode, int TagData,
                           char sItemString[16], int *pColorCode, COLORREF *pRGB, double *pFontSize)
  {
    std::ignore = RadarTarget;
    std::ignore = TagData;
    std::ignore = pFontSize;

    *pColorCode = EuroScopePlugIn::TAG_COLOR_RGB_DEFINED;

    if (nullptr == FlightPlan.GetFlightPlanData().GetPlanType() || 0 == std::strlen(FlightPlan.GetFlightPlanData().GetPlanType()))
      return;
    // filter VFR flights
    if (std::string_view("I") != FlightPlan.GetFlightPlanData().GetPlanType())
    {
      *pRGB = (190 << 16) | (190 << 8) | 190;
      std::strcpy(sItemString, "----");
      return;
    }

    std::string_view departureAirport(FlightPlan.GetFlightPlanData().GetOrigin());
    std::lock_guard guard(this->m_airportLock);
    for (auto &airport : this->m_airports)
    {
      if (airport->getAirportIcao() != departureAirport)
        continue;

      if (airport->pilotExists(FlightPlan.GetCallsign()) != true)
        continue;

      const auto &pilot = airport->getPilot(FlightPlan.GetCallsign());
      std::stringstream outputText;

      tagitems::displayTagItem(ItemCode, pilot, outputText, pRGB);

      std::strcpy(sItemString, outputText.str().c_str());
      break;
    }
  }

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

  // Tag Items / Functions

  void vACDM::RegisterTagItems()
  {
    RegisterTagItemType("EOBT", tagitems::EOBT);
    RegisterTagItemType("TOBT", tagitems::TOBT);
    RegisterTagItemType("TSAT", tagitems::TSAT);
    RegisterTagItemType("TTOT", tagitems::TTOT);
    RegisterTagItemType("EXOT", tagitems::EXOT);
    RegisterTagItemType("ASAT", tagitems::ASAT);
    RegisterTagItemType("AOBT", tagitems::AOBT);
    RegisterTagItemType("ATOT", tagitems::ATOT);
    RegisterTagItemType("ASRT", tagitems::ASRT);
    RegisterTagItemType("AORT", tagitems::AORT);
    RegisterTagItemType("CTOT", tagitems::CTOT);
    RegisterTagItemType("ECFMP Measures", tagitems::ECFMP_MEASURES);
    RegisterTagItemType("Event Booking", tagitems::EVENT_BOOKING);
  }
}