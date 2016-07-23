#ifndef DASH__UTIL__CONFIG_H__
#define DASH__UTIL__CONFIG_H__

#include <dash/internal/Logging.h>
#include <dash/util/StaticConfig.h>

#include <string>
#include <sstream>
#include <unordered_map>
#include <functional>
#include <type_traits>
#include <cstdlib>


namespace dash {
namespace util {

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

  typedef void (*callback_fun)(const std::string &);

  static std::unordered_map<std::string, callback_fun>  callbacks_;
  static std::unordered_map<std::string, std::string>   config_values_;

private:
  static std::string get_str(
    const std::string & key)
  {
    std::string value;
    auto kv = Config::config_values_.find(key);
    if (kv != Config::config_values_.end()) {
      value = kv->second;
    }
    DASH_LOG_TRACE("util::Config::get_str >", key, "->", value);
    return value;
  }

  static void set_str(
    const std::string & key,
    const std::string & value)
  {
    DASH_LOG_TRACE("util::Config::set_str >", key, "->", value);
    Config::config_values_[key] = value;
  }


public:

  ///////////////////////////////////////////////////////////////////////////
  // Config::get<T>(key)
  ///////////////////////////////////////////////////////////////////////////

  template<typename ValueT>
  static
  typename std::enable_if<
    std::is_same<ValueT, bool>::value, ValueT>::type
  get(const std::string & key)
  {
    bool value = (get_str(key)           == "1") ||
                 (get_str(key + "_BOOL") == "1");

    DASH_LOG_TRACE("util::Config::get<bool>", key, "->", value);
    return value;
  }

  template<typename ValueT>
  static
  typename std::enable_if<
    std::is_floating_point<ValueT>::value, ValueT>::type
  get(const std::string & key)
  {
    ValueT value = std::stod(get_str(key).c_str());
    DASH_LOG_TRACE("util::Config::get<fp>", key, "->", value);
    return value;
  }

  template<typename ValueT>
  static
  typename std::enable_if<
    !std::is_same<ValueT, bool>::value &&
    std::is_integral<ValueT>::value, ValueT >::type
  get(const std::string & key)
  {
    ValueT value = std::strtoll(get_str(key).c_str(), nullptr, 10);
    DASH_LOG_TRACE("util::Config::get<integral>", key, "->", value);
    return value;
  }

  template<typename ValueT>
  static
  typename std::enable_if<
    !std::is_floating_point<ValueT>::value &&
    !std::is_integral<ValueT>::value, ValueT>::type
  get(const std::string & key)
  {
    std::istringstream ss(get_str(key));
    ValueT value;
    ss >> value;

    DASH_LOG_TRACE("util::Config::get<T>", key, "->", value);
    return value;
  }

  ///////////////////////////////////////////////////////////////////////////
  // Config::set(key, T)
  ///////////////////////////////////////////////////////////////////////////

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
    std::string value_s = ss.str();

    Config::config_values_[key] = value_s;
    Config::on_change(key, value_s);
  }

  static void
  set(
    const std::string & key,
    std::string         value);

  static inline void
  set(
    const std::string & key,
    const char *        cstr)
  {
    set(key, std::string(cstr));
  }

  ///////////////////////////////////////////////////////////////////////////
  // Config::begin(), Config::end()
  ///////////////////////////////////////////////////////////////////////////

  static typename std::unordered_map<std::string, std::string>::iterator
  begin() {
    return Config::config_values_.begin();
  }

  static typename std::unordered_map<std::string, std::string>::iterator
  end() {
    return Config::config_values_.end();
  }

  static bool is_set(const std::string & key)
  {
    auto kv = Config::config_values_.find(key);
    if (kv == Config::config_values_.end()) {
      return false;
    }
    return true;
  }

  static void init();

private:

  static void on_change(
    const std::string & key,
    const std::string & value)
  {
    auto callback_it = Config::callbacks_.find(key);
    if (Config::callbacks_.end() != callback_it) {
      ((*callback_it).second)(value);
    }
  }

  static void dash_enable_logging_callback(
    const std::string & value)
  {
    if (value == "1") {
      dash::internal::logging::enable_log();
      DASH_LOG_TRACE("util::Config::set", "Log enabled");
    } else if (value == "0") {
      DASH_LOG_TRACE("util::Config::set", "Disabling log");
      dash::internal::logging::disable_log();
    }
  }

};

} // namespace util
} // namespace dash

#endif // DASH__UTIL__CONFIG_H__
