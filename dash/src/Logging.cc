#include <dash/internal/Logging.h>

#include <dash/internal/Macro.h>
#include <dash/internal/StreamConversion.h>

#include <array>
#include <vector>
#include <map>
#include <set>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <iterator>
#include <cstring>

#include <sys/types.h>
#include <unistd.h>


namespace dash {
namespace internal {
namespace logging {

// Terminator
void Log_Recursive(
  const char* level,
  const char* file,
  int line,
  const char* context_tag,
  std::ostringstream & msg)
{
  pid_t pid = getpid();
  std::stringstream buf;
  buf << "[ "
      << std::setw(4) << dash::myid()
      << " "
      << level
      << " ] [ "
      << std::right << std::setw(5) << pid
      << " ] "
      << std::left << std::setw(25)
      << file << ":"
      << std::left << std::setw(4)
      << line << " | "
      << std::left << std::setw(35)
      << context_tag
      << msg.str()
      << std::endl;
  DASH_LOG_OUTPUT_TARGET << buf.str();
}


} // namespace logging
} // namespace internal
} // namespace dash
