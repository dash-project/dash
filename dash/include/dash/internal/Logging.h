#ifndef DASH__INTERNAL__LOGGING_H_
#define DASH__INTERNAL__LOGGING_H_

#include <dash/internal/Macro.h>

#include <array>
#include <vector>
#include <map>
#include <set>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <iterator>
#include <cstring>

namespace dash {
  // forward-declaration
  int myid();
}

//
// Enable logging if trace logging is enabled:
//
#if defined(DASH_ENABLE_TRACE_LOGGING) && \
    !defined(DASH_ENABLE_LOGGING)
#define DASH_ENABLE_LOGGING
#endif

//
// Always log error messages:
//
#define DASH_LOG_ERROR(...) \
  dash::internal::logging::LogWrapper(\
    "ERROR", __FILE__, __LINE__, __VA_ARGS__)

#define DASH_LOG_ERROR_VAR(context, var) \
  dash::internal::logging::LogVarWrapper(\
    "ERROR", __FILE__, __LINE__, context, #var, (var))

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
#      define DASH_LOG_TRACE_VAR(...) do {  } while(0)

#  endif // DASH_ENABLE_TRACE_LOGGING
#else  // DASH_ENABLE_LOGGING

#  define DASH_LOG_TRACE(...) do {  } while(0)
#  define DASH_LOG_TRACE_VAR(...) do {  } while(0)
#  define DASH_LOG_DEBUG(...) do {  } while(0)
#  define DASH_LOG_DEBUG_VAR(...) do {  } while(0)

#endif // DASH_ENABLE_LOGGING

namespace dash {
namespace internal {
namespace logging {

// To print std::array to ostream
template <typename T, std::size_t N>
std::ostream & operator<<(
  std::ostream & o,
  const std::array<T, N> & arr)
{
  std::ostringstream ss;
  auto nelem = arr.size();
  ss << "{ ";
  int i = 1;
  for (auto e : arr) {
    ss << e
       << (i++ < nelem ? "," : "");
  }
  ss << " }";
  operator<<(o, ss.str());
  return o;
}
// To print std::vector to ostream
template <typename T>
std::ostream & operator<<(
  std::ostream & o,
  const std::vector<T> & vec)
{
  std::ostringstream ss;
  auto nelem = vec.size();
  ss << "{ ";
  int i = 1;
  for (auto e : vec) {
    ss << e
       << (i++ < nelem ? "," : "");
  }
  ss << " }";
  operator<<(o, ss.str());
  return o;
}
// To print std::set to ostream
template <typename T>
std::ostream & operator<<(
  std::ostream & o,
  const std::set<T> & set)
{
  std::ostringstream ss;
  auto nelem = set.size();
  ss << "{ ";
  int i = 1;
  for (auto e : set) {
    ss << e
       << (i++ < nelem ? "," : "");
  }
  ss << " }";
  operator<<(o, ss.str());
  return o;
}
// To print std::map to ostream
template <typename T1, typename T2>
std::ostream & operator<<(
  std::ostream & o,
  const std::map<T1,T2> & map)
{
  std::ostringstream ss;
  auto nelem = map.size();
  ss << "{ ";
  int i = 1;
  for (auto kv : map) {
    ss << "(" << kv.first
       << ":" << kv.second << ")"
       << (i++ < nelem ? "," : "");
  }
  ss << " }";
  operator<<(o, ss.str());
  return o;
}

// Terminator
static void Log_Recursive(
  const char* level,
  const char* file,
  int line,
  const char* context_tag,
  std::ostringstream & msg) {
  std::stringstream buf;
  buf << "[ "
      << std::setw(4) << dash::myid()
      << " "
      << level
      << " ] "
      << std::left << std::setw(25)
      << file << ":"
      << std::left << std::setw(4)
      << line << " | "
      << std::left << std::setw(35)
      << context_tag
      << msg.str()
      << std::endl;
  std::cout << buf.str();
}

// "Recursive" variadic function
template<typename T, typename... Args>
static void Log_Recursive(
  const char* level,
  const char* file,
  int line,
  const char* context_tag,
  std::ostringstream & msg,
  T value,
  const Args & ... args) {
  msg << value << " ";
  Log_Recursive(level, file, line, context_tag, msg, args...);
}

// Log_Recursive wrapper that creates the ostringstream
template<typename... Args>
static void LogWrapper(
  const char* level,
  const char* filepath,
  int line,
  const char* context_tag,
  const Args & ... args) {
  std::ostringstream msg;
  msg << "| ";
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
static void LogVarWrapper(
  const char* level,
  const char* filepath,
  int line,
  const char* context_tag,
  const char* var_name,
  const T & var_value,
  const Args & ... args) {
  std::ostringstream msg;
  msg << "| |- " << var_name << ": " << var_value;
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
