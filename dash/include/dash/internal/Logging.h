#ifndef DASH__INTERNAL__LOGGING_H_
#define DASH__INTERNAL__LOGGING_H_

#include <dash/internal/Macro.h>
// #include <dash/Init.h>

namespace dash {
  // forward-declaration
  int myid();
}

#if defined(DASH_ENABLE_LOGGING)
// {

#include <array>
#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <iterator>

#if defined(DASH_ENABLE_TRACE_LOGGING)

#  define DASH_LOG_TRACE(...) \
     dash::internal::logging::LogWrapper(\
       "TRACE", __FILE__, __LINE__, __VA_ARGS__)

#  define DASH_LOG_TRACE_VAR(context, var) \
     dash::internal::logging::LogVarWrapper(\
       "TRACE", __FILE__, __LINE__, context, #var, (var))

#else  // DASH_ENABLE_TRACE_LOGGING

#  define DASH_LOG_TRACE(...) do {  } while(0)
#  define DASH_LOG_TRACE_VAR(...) do {  } while(0)

#endif // DASH_ENABLE_TRACE_LOGGING

#define DASH_LOG_DEBUG(...) \
  dash::internal::logging::LogWrapper(\
    "DEBUG", __FILE__, __LINE__, __VA_ARGS__)

#define DASH_LOG_DEBUG_VAR(context, var) \
  dash::internal::logging::LogVarWrapper(\
    "DEBUG", __FILE__, __LINE__, context, #var, (var))

namespace dash {
namespace internal {
namespace logging {

// To print std::array to ostream
template <typename T, std::size_t N>
std::ostream & operator<<(
  std::ostream & o,
  const std::array<T, N> & arr) {
  std::copy(arr.cbegin(), arr.cend(),
            std::ostream_iterator<T>(o, ","));
  return o;
}
// To print std::vector to ostream
template <typename T>
std::ostream & operator<<(
  std::ostream & o,
  const std::vector<T> & arr) {
  std::copy(arr.cbegin(), arr.cend(),
            std::ostream_iterator<T>(o, ","));
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
  // Extract file name from path
  std::string filename(filepath);
  std::size_t offset = filename.find_last_of("/\\");
  Log_Recursive(
    level,
    filename.substr(offset+1).c_str(),
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
  msg << "| " << var_name << "(" << var_value << ")";
  // Extract file name from path
  std::string filename(filepath);
  std::size_t offset = filename.find_last_of("/\\");
  Log_Recursive(
    level,
    filename.substr(offset+1).c_str(),
    line,
    context_tag,
    msg);
}

} // namespace logging
} // namespace internal
} // namespace dash

// }
#else  // DASH_ENABLE_LOGGING

#  define DASH_LOG_TRACE(...) do {  } while(0)
#  define DASH_LOG_TRACE_VAR(...) do {  } while(0)
#  define DASH_LOG_DEBUG(...) do {  } while(0)
#  define DASH_LOG_DEBUG_VAR(...) do {  } while(0)

#endif // DASH_ENABLE_LOGGING

#endif // DASH__INTERNAL__LOGGING_H_
