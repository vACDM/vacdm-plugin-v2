#include "TagItems.h"
#include "TagItemsColor.h"

namespace vacdm::tagitems
{
  std::string displayTime(const std::chrono::utc_clock::time_point timepoint)
  {
    if (timepoint.time_since_epoch().count() > 0)
      return std::format("{:%H%M}", timepoint);
    else
      return "";
  }

  void displayTagItem(int ItemCode, const types::Pilot &pilot, std::stringstream &displayedText, COLORREF *textColor)
  {
    switch (static_cast<itemType>(ItemCode))
    {
    case itemType::EOBT:
      displayedText << displayTime(pilot.eobt);
      *textColor = Color().instance().colorizeEobt(pilot);
      break;
    case itemType::TOBT:
      displayedText << displayTime(pilot.tobt);
      *textColor = Color().instance().colorizeTobt(pilot);
      break;
    case itemType::TSAT:
      displayedText << displayTime(pilot.tsat);
      *textColor = Color().instance().colorizeTsat(pilot);
      break;
    case itemType::TTOT:
      displayedText << displayTime(pilot.ttot);
      *textColor = Color().instance().colorizeTtot(pilot);
      break;
    case itemType::EXOT:
      displayedText << displayTime(pilot.exot);
      *textColor = Color().instance().colorizeExot(pilot);
      break;
    case itemType::ASAT:
      displayedText << displayTime(pilot.asat);
      *textColor = Color().instance().colorizeAsat(pilot);
      break;
    case itemType::AOBT:
      displayedText << displayTime(pilot.aobt);
      *textColor = Color().instance().colorizeAobt(pilot);
      break;
    case itemType::ATOT:
      displayedText << displayTime(pilot.atot);
      *textColor = Color().instance().colorizeAtot(pilot);
      break;
    case itemType::ASRT:
      displayedText << displayTime(pilot.asrt);
      *textColor = Color().instance().colorizeAsrt(pilot);
      break;
    case itemType::AORT:
      displayedText << displayTime(pilot.aort);
      *textColor = Color().instance().colorizeAort(pilot);
      break;
    case itemType::CTOT:
      displayedText << displayTime(pilot.ctot);
      *textColor = Color().instance().colorizeCtot(pilot);
      break;
    case itemType::ECFMP_MEASURES:
      displayedText << "";
      *textColor = Color().instance().colorizeEcfmpMeasure(pilot);
      break;
    case itemType::EVENT_BOOKING:
      displayedText << (pilot.hasBooking ? "B" : "");
      *textColor = Color().instance().colorizeEventBooking(pilot);
      break;
    default:
      break;
    }
  }
}