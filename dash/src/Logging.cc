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
  std::istringstream ss(msg.str());
  std::string item;
  while (std::getline(ss, item)) {
    Log_Line(level, file, line, context_tag, item);
  }
}

} // namespace logging
} // namespace internal
} // namespace dash
