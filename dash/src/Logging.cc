#include <dash/internal/Logging.h>

#include <dash/internal/Macro.h>
#include <dash/internal/StreamConversion.h>

#include <array>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cstring>

#include <unistd.h>

#ifdef __GNUG__
#  include <cxxabi.h>
#  include <cstdlib>
#  include <memory>
#endif


namespace dash {
namespace internal {
namespace logging {

#if defined(__GNUG__) || \
    defined(HAVE_CXA_DEMANGLE)
std::string demangle(const char * typeid_name) {
  int status = -4; // to avoid compiler warning
  std::unique_ptr<char, void(*)(void*)> res {
      abi::__cxa_demangle(typeid_name, NULL, NULL, &status),
      std::free
  };
  return (status==0) ? res.get() : typeid_name ;
}
#else
std::string demangle(const char * typeid_name) {
  return typeid_name;
}
#endif

bool _log_enabled = true;

void Log_Recursive(
  const char* level,
  const char* file,
  int line,
  const char* context_tag,
  std::ostringstream & msg)
{
  std::string msg_str = msg.str();
  if (msg_str.empty()) {
    Log_Line(level, file, line, "", context_tag);
  } else {
    std::istringstream ss(msg_str);
    std::string item;
    while (std::getline(ss, item)) {
      Log_Line(level, file, line, context_tag, item);
    }
  }
  (DASH_LOG_OUTPUT_TARGET).flush();
}

} // namespace logging
} // namespace internal
} // namespace dash
