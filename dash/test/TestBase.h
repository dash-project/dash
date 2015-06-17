#ifndef DASH__TEST__TEST_BASE_H_
#define DASH__TEST__TEST_BASE_H_

#include <gtest/gtest.h>
#include <dash/internal/Logging.h>

namespace testing {
namespace internal {

enum GTestColor {
    COLOR_DEFAULT,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_YELLOW
};

extern void ColoredPrintf(
  GTestColor color,
  const char* fmt,
  ...);

} // namespace internal
} // namespace testing

#if defined(DASH_ENABLE_LOGGING)

#define LOG_MESSAGE(...) do { \
  char buffer[200]; \
  sprintf(buffer, __VA_ARGS__); \
  testing::internal::ColoredPrintf( \
    testing::internal::COLOR_GREEN, \
    "[   LOG    ] "); \
  testing::internal::ColoredPrintf(\
    testing::internal::COLOR_YELLOW, \
    buffer); \
  testing::internal::ColoredPrintf( \
    testing::internal::COLOR_YELLOW, \
    "\n"); \
} while(0)

#else // DASH_ENABLE_LOGGING

#define LOG_MESSAGE(...) do {  } while(0)

#endif

#define DASH_TEST_LOCAL_ONLY() do { \
  if (dash::myid() > 0) { \
    return; \
  } \
} while(0)

#define SCOPED_TRACE_MSG(msg) do { \
  SCOPED_TRACE(::testing::Message() << msg); \
} while(0)

#endif // DASH__TEST__TEST_BASE_H_
