#include "types/Pilot.h"
#include "config/PluginConfig.h"

namespace vacdm::tagitems
{
  class Color
  {
  public:
    Color();

    // times:

    COLORREF colorizeEobt(const types::Pilot &pilot) const;
    COLORREF colorizeTobt(const types::Pilot &pilot) const;
    COLORREF colorizeTsat(const types::Pilot &pilot) const;
    COLORREF colorizeTtot(const types::Pilot &pilot) const;
    COLORREF colorizeExot(const types::Pilot &pilot) const;
    COLORREF colorizeAsat(const types::Pilot &pilot) const;
    COLORREF colorizeAobt(const types::Pilot &pilot) const;
    COLORREF colorizeAtot(const types::Pilot &pilot) const;
    COLORREF colorizeAsrt(const types::Pilot &pilot) const;
    COLORREF colorizeAort(const types::Pilot &pilot) const;
    COLORREF colorizeCtot(const types::Pilot &pilot) const;

    // timers:

    COLORREF colorizeCtotTimer(const types::Pilot &pilot) const;
    COLORREF colorizeAsatTimer(const types::Pilot &pilot) const;

    // other:

    COLORREF colorizeEcfmpMeasure(const types::Pilot &pilot) const;
    COLORREF colorizeEventBooking(const types::Pilot &pilot) const;

    PluginConfig m_pluginConfig;
    void changePluginConfig(const PluginConfig newPluginConfig);

    Color(const Color &) = delete;
    Color(Color &&) = delete;

    Color &operator=(const Color &) = delete;
    Color &operator=(Color &&) = delete;

    static Color &instance();

  private:
    COLORREF colorizeEobtAndTobt(const types::Pilot &pilot) const;
    COLORREF colorizeCtotandCtottimer(const types::Pilot &pilot) const;
  };
}