#include <string>
#include <chrono>

namespace vacdm::utils
{
  std::string timestampToIsoString(const std::chrono::utc_clock::time_point& timepoint) {
    if (timepoint.time_since_epoch().count() >= 0) {
      std::stringstream stream;
      stream << std::format("{0:%FT%T}", timepoint);
      auto timestamp = stream.str();
      timestamp = timestamp.substr(0, timestamp.length() - 4) + "Z";
      return timestamp;
    }
    else {
      return "1969-12-31T23:59:59.999Z";
    }
  }
}