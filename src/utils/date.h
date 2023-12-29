#pragma once

#include <chrono>
#include <string>

#pragma warning(push, 0)
#include "EuroScopePlugIn.h"
#pragma warning(pop)

#include "log/Logger.h"

namespace vacdm::utils
{
    std::string timestampToIsoString(
        const std::chrono::utc_clock::time_point &timepoint);

    /// @brief transforms the 4 char estimated departure time from the flightplan into a utc_clock::time_point
    /// @param flightplan the flightplan of the aircraft
    /// @return the departure time as utc_clock::time_point
    std::chrono::utc_clock::time_point convertEuroscopeDepartureTime(
        const EuroScopePlugIn::CFlightPlan flightplan);
}
