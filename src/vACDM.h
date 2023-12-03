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

  public:
    // Euroscope Events
    void OnAirportRunwayActivityChanged() override;
  };
}