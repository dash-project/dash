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


namespace dash {
namespace internal {
namespace logging {

void Log_Recursive(
  const char* level,
  const char* file,
  int line,
  const char* context_tag,
  std::ostringstream & msg)
{
  std::string msg_str = msg.str();

  if (msg_str.length() > 40) {
    if (msg_str.find('\n') != std::string::npos) {
      std::stringstream ss(msg_str);
      std::string item;
      while (std::getline(ss, item, '\n')) {
        Log_Line(level, file, line, context_tag, item);
      }
    }
  } else {
    Log_Line(level, file, line, context_tag, msg_str);
  }
}

} // namespace logging
} // namespace internal
} // namespace dash
