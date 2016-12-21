#ifndef DASH__INTERNAL__LOGGING_H_
#define DASH__INTERNAL__LOGGING_H_

#include <dash/internal/Macro.h>
#include <dash/internal/StreamConversion.h>
#include <dash/Types.h>

#include <dash/dart/if/dart_config.h>

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
  // forward-declaration
  global_unit_t myid();
}

#ifdef DASH_LOG_OUTPUT_STDOUT
#  define DASH_LOG_OUTPUT_TARGET std::cout
#else
#  define DASH_LOG_OUTPUT_TARGET std::clog
#endif

//
// Enable logging if trace logging is enabled:
//
#if defined(DASH_ENABLE_TRACE_LOGGING) && \
    !defined(DASH_ENABLE_LOGGING)
#define DASH_ENABLE_LOGGING
#endif

//
// Always log error and warning messages:
//
#define DASH_LOG_ERROR(...) \
  dash::internal::logging::LogWrapper(\
    "ERROR", __FILE__, __LINE__, __VA_ARGS__)

#define DASH_LOG_ERROR_VAR(context, var) \
  dash::internal::logging::LogVarWrapper(\
    "ERROR", __FILE__, __LINE__, context, #var, (var))

#define DASH_LOG_WARN(...) \
  dash::internal::logging::LogWrapper(\
    "WARN ", __FILE__, __LINE__, __VA_ARGS__)

#define DASH_LOG_WARN_VAR(context, var) \
  dash::internal::logging::LogVarWrapper(\
    "WARN ", __FILE__, __LINE__, context, #var, (var))

//
// Debug and trace log messages:
//
#if defined(DASH_ENABLE_LOGGING)
#  define DASH_LOG_DEBUG(...) \
     dash::internal::logging::LogWrapper(\
       "DEBUG", __FILE__, __LINE__, __VA_ARGS__)

#  define DASH_LOG_DEBUG_VAR(context, var) \
     dash::internal::logging::LogVarWrapper(\
       "DEBUG", __FILE__, __LINE__, context, #var, (var))

#  if defined(DASH_ENABLE_TRACE_LOGGING)

#    define DASH_LOG_TRACE(...) \
       dash::internal::logging::LogWrapper(\
         "TRACE", __FILE__, __LINE__, __VA_ARGS__)

#    define DASH_LOG_TRACE_VAR(context, var) \
       dash::internal::logging::LogVarWrapper(\
         "TRACE", __FILE__, __LINE__, context, #var, (var))

#  else  // DASH_ENABLE_TRACE_LOGGING
#      define DASH_LOG_TRACE(...) do {  } while(0)
#      define DASH_LOG_TRACE_VAR(context, var) do { \
                dash__unused(var); \
              } while(0)

#  endif // DASH_ENABLE_TRACE_LOGGING
#else  // DASH_ENABLE_LOGGING

#  define DASH_LOG_TRACE(...) do {  } while(0)
#  define DASH_LOG_TRACE_VAR(context, var) do { \
            dash__unused(var); \
          } while(0)
#  define DASH_LOG_DEBUG(...) do {  } while(0)
#  define DASH_LOG_DEBUG_VAR(context, var) do { \
            dash__unused(var); \
          } while(0)

#endif // DASH_ENABLE_LOGGING

namespace dash {
namespace internal {
namespace logging {

extern bool _log_enabled;


enum term_color_code {
  TCOL_DEFAULT = 0,
  TCOL_WHITE,
  TCOL_RED,
  TCOL_GREEN,
  TCOL_YELLOW,
  TCOL_BLUE,
  TCOL_MAGENTA,
  TCOL_CYAN,

  TCOL_NUM_CODES
};

const int term_colors[TCOL_NUM_CODES] = {
  39, // default
  37, // white
  31, // red
  32, // green
  33, // yellow
  34, // blue
  35, // magenta
  36  // cyan
};

const term_color_code unit_term_colors[TCOL_NUM_CODES-1] = {
  TCOL_CYAN,
  TCOL_YELLOW,
  TCOL_MAGENTA,
  TCOL_WHITE,
  TCOL_GREEN,
  TCOL_RED,
  TCOL_BLUE
};

class TermColorMod {
  term_color_code tcol;

public:
  TermColorMod(term_color_code code)
  : tcol(code)
  { }

  friend std::ostream & operator<<(
    std::ostream & os, const TermColorMod & mod) {
    return os << "\033[" << term_colors[mod.tcol] << "m";
  }
};


static inline bool log_enabled()
{
  return _log_enabled;
}

static inline void enable_log()
{
  _log_enabled = true;

  dart_config_t * dart_cfg;
  dart_config(&dart_cfg);
  dart_cfg->log_enabled = 1;
}

static inline void disable_log()
{
  _log_enabled = false;

  dart_config_t * dart_cfg;
  dart_config(&dart_cfg);
  dart_cfg->log_enabled = 0;
}

// Terminator
void Log_Recursive(
  const char* level,
  const char* file,
  int line,
  const char* context_tag,
  std::ostringstream & msg);

inline void Log_Line(
  const char* level,
  const char* file,
  int line,
  const char* context_tag,
  const std::string & msg)
{
  pid_t pid = getpid();
  dash::global_unit_t uid = dash::myid();
  std::stringstream buf;
  
//  buf << TermColorMod(uid < 0 ? TCOL_DEFAULT : unit_term_colors[uid.id % 7]);

  buf << "[ "
      << std::setw(4) << uid.id
      << " "
      << level
      << " ] [ "
      << std::right << std::setw(5) << pid
      << " ] "
      << std::left << std::setw(25)
      << file << ":"
      << std::left << std::setw(4)
      << line << " | "
      << std::left << std::setw(45)
      << context_tag << "| "
      << msg;

//  buf << TermColorMod(TCOL_DEFAULT);
  
  buf << "\n";

  DASH_LOG_OUTPUT_TARGET << buf.str();
}

// "Recursive" variadic function
template<typename T, typename... Args>
inline void Log_Recursive(
  const char         * level,
  const char         * file,
  int                  line,
  const char         * context_tag,
  std::ostringstream & msg,
  T                    value,
  const Args & ...     args)
{
  msg << value << " ";
  Log_Recursive(level, file, line, context_tag, msg, args...);
}

// Log_Recursive wrapper that creates the ostringstream
template<typename... Args>
inline void LogWrapper(
  const char *     level,
  const char *     filepath,
  int              line,
  const char *     context_tag,
  const Args & ... args)
{
  if (!dash::internal::logging::log_enabled()) {
    return;
  }

  std::ostringstream msg;
  // Extract file name from path
  const char * filebase = strrchr(filepath, '/');
  const char * filename = (filebase != 0) ? filebase + 1 : filepath;
  Log_Recursive(
    level,
    filename,
    line,
    context_tag,
    msg, args...);
}

// Log_Recursive wrapper that creates the ostringstream
template<typename T, typename... Args>
inline void LogVarWrapper(
  const char* level,
  const char* filepath,
  int         line,
  const char* context_tag,
  const char* var_name,
  const T &   var_value,
  const Args & ... args)
{
  if (!dash::internal::logging::log_enabled()) {
    return;
  }

  std::ostringstream msg;
  msg << "|- " << var_name << ": " << var_value;
  // Extract file name from path
  const char * filebase = strrchr(filepath, '/');
  const char * filename = (filebase != 0) ? filebase + 1 : filepath;
  Log_Recursive(
    level,
    filename,
    line,
    context_tag,
    msg);
}

} // namespace logging
} // namespace internal
} // namespace dash

#endif // DASH__INTERNAL__LOGGING_H_
