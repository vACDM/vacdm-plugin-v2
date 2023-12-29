#include "Date.h"

namespace vacdm::utils
{
  std::string timestampToIsoString(
      const std::chrono::utc_clock::time_point &timepoint)
  {
    if (timepoint.time_since_epoch().count() >= 0)
    {
      std::stringstream stream;
      stream << std::format("{0:%FT%T}", timepoint);
      auto timestamp = stream.str();
      timestamp = timestamp.substr(0, timestamp.length() - 4) + "Z";
      return timestamp;
    }
    else
    {
      return "1969-12-31T23:59:59.999Z";
    }
  }

  std::chrono::utc_clock::time_point convertEuroscopeDepartureTime(
      const EuroScopePlugIn::CFlightPlan flightplan)
  {
    const std::string callsign = flightplan.GetCallsign();

    const auto now = std::chrono::utc_clock::now();
    const std::string eobt =
        flightplan.GetFlightPlanData().GetEstimatedDepartureTime();

    if (eobt.length() == 0 || eobt.length() > 4)
    {
      logging::Logger::instance().log(
          "Util Date", "Unable to convert EOBT of " + callsign + " " + eobt,
          logging::Logger::Level::Utils);
      return now;
    }

    // logging::Logger::instance().log("Util Date",
    //                                 "Converting EOBT " + eobt + " of " + callsign,
    //                                 logging::Logger::Level::Utils);

    std::stringstream stream;
    stream << std::format("{0:%Y%m%d}", now);
    std::size_t requiredLeadingZeros = 4 - eobt.length();
    while (requiredLeadingZeros != 0)
    {
      requiredLeadingZeros -= 1;
      stream << "0";
    }
    stream << eobt;

    std::chrono::utc_clock::time_point time;
    std::stringstream input(stream.str());
    std::chrono::from_stream(stream, "%Y%m%d%H%M", time);

    return time;
  }
}
