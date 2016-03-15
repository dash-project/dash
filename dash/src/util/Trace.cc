#include <dash/util/Trace.h>

#include <map>
#include <vector>
#include <string>

namespace dash {
namespace util {

std::map<std::string, TraceStore::trace_events_t> TraceStore::_traces
  = {{ }};

bool TraceStore::_trace_enabled
  = false;

} // namespace util
} // namespace dash
