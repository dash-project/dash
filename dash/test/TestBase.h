#ifndef DASH__TEST__TEST_BASE_H_
#define DASH__TEST__TEST_BASE_H_

#include <gtest/gtest.h>
#include <dash/internal/Logging.h>

namespace testing {
namespace internal {

#define ASSERT_FAIL() ASSERT_EQ(0, 1) << "ASSERT_FAIL"

#define ASSERT_EQ_U(e,a) ASSERT_EQ(e,a) << "Unit " << dash::myid()
#define ASSERT_NE_U(e,a) ASSERT_NE(e,a) << "Unit " << dash::myid()
#define ASSERT_LT_U(e,a) ASSERT_LT(e,a) << "Unit " << dash::myid()
#define ASSERT_LE_U(e,a) ASSERT_LE(e,a) << "Unit " << dash::myid()
#define ASSERT_GT_U(e,a) ASSERT_GT(e,a) << "Unit " << dash::myid()
#define ASSERT_GE_U(e,a) ASSERT_GE(e,a) << "Unit " << dash::myid()
#define EXPECT_EQ_U(e,a) EXPECT_EQ(e,a) << "Unit " << dash::myid()
#define EXPECT_NE_U(e,a) EXPECT_NE(e,a) << "Unit " << dash::myid()
#define EXPECT_LT_U(e,a) EXPECT_LT(e,a) << "Unit " << dash::myid()
#define EXPECT_LE_U(e,a) EXPECT_LE(e,a) << "Unit " << dash::myid()
#define EXPECT_GT_U(e,a) EXPECT_GT(e,a) << "Unit " << dash::myid()
#define EXPECT_GE_U(e,a) EXPECT_GE(e,a) << "Unit " << dash::myid()

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
  char buffer[300]; \
  sprintf(buffer, __VA_ARGS__); \
  testing::internal::ColoredPrintf( \
    testing::internal::COLOR_YELLOW, \
    "[ %d LOG    ] %s \n", \
    dash::myid(),\
    buffer); \
} while(0)

#else  // DASH_ENABLE_LOGGING

#define LOG_MESSAGE(...) do {  } while(0)

#endif // DASH_ENABLE_LOGGING

#define DASH_TEST_LOCAL_ONLY() do { \
  if (dash::myid() > 0) { \
    return; \
  } \
} while(0)

#define SCOPED_TRACE_MSG(msg) do { \
  SCOPED_TRACE(::testing::Message() << msg); \
} while(0)

#endif // DASH__TEST__TEST_BASE_H_
