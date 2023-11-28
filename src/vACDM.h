#pragma once

#pragma warning(push, 0)
#include "EuroScopePlugIn.h"
#pragma warning(pop)

#include <string>

namespace vacdm
{
  class vACDM : public EuroScopePlugIn::CPlugIn
  {
  public:
    vACDM();
    ~vACDM();

    void DisplayMessage(const std::string &message,
                        const std::string &sender = "vACDM");
  };
}