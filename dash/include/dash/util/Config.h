#ifndef DASH__UTIL__CONFIG_H__
#define DASH__UTIL__CONFIG_H__

#include <dash/internal/Logging.h>

#include <string>
#include <sstream>
#include <map>
#include <type_traits>
#include <cstdlib>

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
 * Environment variables starting with 'DASH_' are automatically added.
 *
 * Configuration keys ending in '_SIZE' allow to set sizes (bytes) in
 * human-readable format, e.g. "2M" -> 2048.
 * The parsed size in number of bytes is then stored in a separate
 * configuration key <key name>_BYTES.
 *
 * For example:
 *
 * \code
 *   dash::utils::Config::set("CHUNK_SIZE", "4MB");
 *   auto chunk_bytes = dash::utils::Config::get<size_t>("CHUNK_SIZE_BYTES");
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
    std::string value = kv->second;
    return value;
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
  static
  typename std::enable_if<std::is_arithmetic<ValueT>::value, void>::type
  set(
    const std::string & key,
    ValueT              value)
  {
    DASH_LOG_TRACE("util::Config::set(string,T)", key, value);
    std::ostringstream ss;
    ss << value;
    internal::__config_values[key] = ss.str();
  }

  static void
  set(
    const std::string & key,
    bool                value)
  {
    DASH_LOG_TRACE("util::Config::set(string,bool)", key, value);

    if (key == "ENABLE_LOG") {
      if (value) {
        dash::internal::logging::enable_log();
        DASH_LOG_TRACE("util::Config::set", "Log enabled");
      } else {
        DASH_LOG_TRACE("util::Config::set", "Disabling log");
        dash::internal::logging::disable_log();
      }
    }

    std::ostringstream ss;
    ss << value;
    internal::__config_values[key] = ss.str();
  }

  static void
  set(
    const std::string & key,
    std::string         value);

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
