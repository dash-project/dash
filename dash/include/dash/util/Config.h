#ifndef DASH__UTIL__CONFIG_H__
#define DASH__UTIL__CONFIG_H__

#include <string>
#include <sstream>
#include <map>

namespace dash {
namespace util {

namespace internal {

static std::map<std::string, std::string> __config_values;

} // namespace internal

/**
 * Usage:
 *
 * \code
 *   dash::utils::Config::set("NCHUNKS", 1024);
 *   size_t cfg_value = dash::utils::Config::get<size_t>("NCHUNKS");
 * \endcode
 *
 */
class Config
{
private:
  static const std::string & get_str(const std::string & setting_name)
  {
    return internal::__config_values[setting_name];
  }

public:

  template<typename ValueT>
  static ValueT get(const std::string & setting_name);

  template<typename ValueT>
  static typename std::enable_if<std::is_integral<ValueT>::value, int>::type
  get(const std::string & setting_name) {
    return atoi(get_str(setting_name).c_str());
  }

  template<typename ValueT>
  static void set(
    const std::string & setting_name,
    const ValueT      & setting_value)
  {
    std::ostringstream ss;
    ss << setting_value;
    internal::__config_values[setting_name] = ss.str();
  }
};


template<>
const std::string & Config::get<const std::string &>(
  const std::string & setting_name)
{
  return get_str(setting_name);
}

template<>
bool Config::get<bool>(
  const std::string & setting_name)
{
  return atoi(get_str(setting_name).c_str()) == 1;
}

} // namespace util
} // namespace dash

#endif // DASH__UTIL__CONFIG_H__
