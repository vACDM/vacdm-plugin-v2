#include "vACDM.h"
#include "Version.h"

namespace vacdm
{
  vACDM::vACDM() : CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE, PLUGIN_NAME, PLUGIN_VERSION, PLUGIN_AUTHOR, PLUGIN_LICENSE)
  {
    DisplayMessage("Version " + std::string(PLUGIN_VERSION) + " loaded", "Initialisation");
  }
  vACDM::~vACDM()
  {
  }

  void vACDM::DisplayMessage(const std::string &message, const std::string &sender)
  {
    DisplayUserMessage("vACDM", sender.c_str(), message.c_str(), true, false, false, false, false);
  }
}