#pragma once
/// @brief Defines the Tag Item Types and Tag Item Functions used in Euroscope

#include <chrono>
#include <string>
#include <format>

#include "types/Pilot.h"
#include <wtypes.h>

namespace vacdm::tagitems
{
  enum itemType
  {
    EOBT,
    TOBT,
    TSAT,
    TTOT,
    EXOT,
    ASAT,
    AOBT,
    ATOT,
    ASRT,
    AORT,
    CTOT,
    ECFMP_MEASURES,
    EVENT_BOOKING,
  };

  /// @brief formats the time
  /// @param timepoint time_point to format
  /// @return formatted time string
  std::string displayTime(const std::chrono::utc_clock::time_point timepoint);
  /// @brief handles the display of tag items, formatting and color
  /// @param ItemCode the EuroScope ItemCode
  /// @param pilot the data of the pilot
  /// @param displayedText stream of text which will be displayed
  /// @param textColor color of the displayed text
  void displayTagItem(int ItemCode, const types::Pilot &pilot, std::stringstream &displayedText, COLORREF *textColor);
}