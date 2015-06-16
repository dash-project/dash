#ifndef DASH__INTERNAL__LOGGING_H_
#define DASH__INTERNAL__LOGGING_H_

#include <dash/internal/Macro.h>

#ifdef DASH_ENABLE_LOGGING

#include <sstream>
#include <iostream>

#define DASH_LOG(...) dash::logging::LogWrapper(__FILE__, __LINE__, __VA_ARGS__)

namespace dash {
namespace logging {


} // namespace logging
} // namespace dash

#else 

#define DASH_LOG(...) do {  } while(0)

#endif // DASH_ENABLE_LOGGING

#endif // DASH__INTERNAL__LOGGING_H_
