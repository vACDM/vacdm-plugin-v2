#pragma once

#pragma warning(push, 0)
#include "EuroScopePlugIn.h"
#pragma warning(pop)

#include <string>
#include <map>
#include <vector>
#include <list>
#include <mutex>

#include "com/Airport.h"
#include "config/PluginConfig.h"
#include "utils/date.h"

namespace vacdm
{
  class vACDM : public EuroScopePlugIn::CPlugIn
  {
  public:
    vACDM();
    ~vACDM();

    void DisplayMessage(const std::string &message,
                        const std::string &sender = "vACDM");

  private:
    std::map<std::string, std::vector<std::string>> m_activeRunways;
    std::mutex m_airportLock;
    std::list<std::shared_ptr<com::Airport>> m_airports;

    // config:

    std::string m_dllPath;
    std::string m_configFileName = "\\vacdm.txt";
    PluginConfig m_pluginConfig;

    void changeServerUrl(const std::string &url);

    void checkServerConfiguration();
    void reloadConfiguration();

    void runEuroscopeUpdateCycle();
    void updatePilotData(const EuroScopePlugIn::CFlightPlan &flightplan);

    void RegisterTagItems();

  public:
    // Euroscope Events
    bool OnCompileCommand(const char *sCommandLine) override;
    void OnGetTagItem(EuroScopePlugIn::CFlightPlan FlightPlan, EuroScopePlugIn::CRadarTarget RadarTarget, int ItemCode, int TagData, char sItemString[16], int *pColorCode, COLORREF *pRGB, double *pFontSize) override;
    void OnTimer(const int Counter) override;
    void OnAirportRunwayActivityChanged() override;
    void OnFlightPlanControllerAssignedDataUpdate(EuroScopePlugIn::CFlightPlan flightplan, const int dataType) override;
  };
}