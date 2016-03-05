#include <dash/util/Config.h>
#include <dash/internal/Logging.h>

#include <map>
#include <string>
#include <cstring>
#include <unistd.h>

// Environment variables as array of strings, terminated by null pointer.
extern char ** environ;

namespace dash {
namespace util {

namespace internal {

std::map<std::string, std::string> __config_values;

}

void Config::init()
{
  int    i          = 1;
  char * env_var_kv = *environ;
  for (; env_var_kv != 0; ++i) {
    // Split into key and value:
    char * flag_name_cstr  = env_var_kv;
    char * flag_value_cstr = strstr(env_var_kv, "=");
    int    flag_name_len   = flag_value_cstr - flag_name_cstr;
    std::string flag_name(flag_name_cstr, flag_name_cstr + flag_name_len);
    std::string flag_value(flag_value_cstr+1);
    if (strstr(env_var_kv, "DASH_") == env_var_kv) {
      internal::__config_values[flag_name] = flag_value;
    }
    env_var_kv = *(environ + i);
  }
}

template<>
std::string Config::get<std::string>(
  const std::string & key)
{
  return get_str(key);
}

template<>
bool Config::get<bool>(
  const std::string & key)
{
  return atoi(get_str(key).c_str()) == 1;
}


} // namespace util
} // namespace dash
