#ifndef DASH__UTIL__CONFIG_H__
#define DASH__UTIL__CONFIG_H__

#include <string>
#include <sstream>
#include <map>

namespace dash {
namespace util {

namespace internal {

extern std::map<std::string, std::string> __config_values;

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
  static std::string get_str(const std::string & key)
  {
    auto kv = internal::__config_values.find(key);
    if (kv == internal::__config_values.end()) {
      return std::string("");
    }
    return std::string(kv->second);
  }

public:

  template<typename ValueT>
  static
  typename std::enable_if<!std::is_integral<ValueT>::value, ValueT>::type
  get(const std::string & key);

  template<typename ValueT>
  static
  typename std::enable_if<std::is_integral<ValueT>::value, ValueT>::type
  get(const std::string & key)
  {
    return atoi(get_str(key).c_str());
  }

  template<typename ValueT>
  static void set(
    const std::string & key,
    const ValueT      & setting_value)
  {
    std::ostringstream ss;
    ss << setting_value;
    internal::__config_values[key] = ss.str();
  }

  static typename std::map<std::string, std::string>::iterator
  begin() {
    return internal::__config_values.begin();
  }

  static typename std::map<std::string, std::string>::iterator
  end() {
    return internal::__config_values.end();
  }

  static bool is_set(const std::string & key)
  {
    auto kv = internal::__config_values.find(key);
    if (kv == internal::__config_values.end()) {
      return false;
    }
    return true;
  }

  static void init();
};

} // namespace util
} // namespace dash

#endif // DASH__UTIL__CONFIG_H__
