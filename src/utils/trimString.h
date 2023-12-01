#include <string>

namespace vacdm::utils
{
  static __inline std::string ltrim(const std::string &str)
  {
    size_t start = str.find_first_not_of(" \n\r\t\f\v");
    return (start == std::string::npos) ? "" : str.substr(start);
  }

  static __inline std::string rtrim(const std::string &str)
  {
    size_t end = str.find_last_not_of(" \n\r\t\f\v");
    return (end == std::string::npos) ? "" : str.substr(0, end + 1);
  }

  static __inline std::string trim(const std::string &str)
  {
    return rtrim(ltrim(str));
  }
}