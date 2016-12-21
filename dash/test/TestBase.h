#ifndef DASH__TEST__TEST_BASE_H_
#define DASH__TEST__TEST_BASE_H_

#include <gtest/gtest.h>
#include <dash/internal/Logging.h>

#include "TestGlobals.h"
#include "TestPrinter.h"
#include "TestLogHelpers.h"


namespace testing {
namespace internal {

#define ASSERT_FAIL() EXPECT_EQ(0, 1) << "ASSERT_FAIL"

#define ASSERT_TRUE_U(b)  EXPECT_TRUE(b)  << "Unit " << dash::myid().id
#define ASSERT_FALSE_U(b) EXPECT_FALSE(b) << "Unit " << dash::myid().id
#define ASSERT_EQ_U(e,a)  EXPECT_EQ(e,a)  << "Unit " << dash::myid().id
#define ASSERT_NE_U(e,a)  EXPECT_NE(e,a)  << "Unit " << dash::myid().id
#define ASSERT_LT_U(e,a)  EXPECT_LT(e,a)  << "Unit " << dash::myid().id
#define ASSERT_GT_U(e,a)  EXPECT_GT(e,a)  << "Unit " << dash::myid().id
#define ASSERT_LE_U(e,a)  EXPECT_LE(e,a)  << "Unit " << dash::myid().id
#define ASSERT_GE_U(e,a)  EXPECT_GE(e,a)  << "Unit " << dash::myid().id
#define ASSERT_DOUBLE_EQ_U(e,a) \
  EXPECT_DOUBLE_EQ(e,a) << "Unit " << dash::myid().id
#define ASSERT_FLOAT_EQ_U(e,a)  \
  EXPECT_FLOAT_EQ(e,a)  << "Unit " << dash::myid().id

#define EXPECT_TRUE_U(b)  EXPECT_TRUE(b)  << "Unit " << dash::myid().id
#define EXPECT_FALSE_U(b) EXPECT_FALSE(b) << "Unit " << dash::myid().id
#define EXPECT_EQ_U(e,a)  EXPECT_EQ(e,a)  << "Unit " << dash::myid().id
#define EXPECT_NE_U(e,a)  EXPECT_NE(e,a)  << "Unit " << dash::myid().id
#define EXPECT_LT_U(e,a)  EXPECT_LT(e,a)  << "Unit " << dash::myid().id
#define EXPECT_GT_U(e,a)  EXPECT_GT(e,a)  << "Unit " << dash::myid().id
#define EXPECT_LE_U(e,a)  EXPECT_LE(e,a)  << "Unit " << dash::myid().id
#define EXPECT_GE_U(e,a)  EXPECT_GE(e,a)  << "Unit " << dash::myid().id
#define EXPECT_DOUBLE_EQ_U(e,a) \
  EXPECT_DOUBLE_EQ(e,a) << "Unit " << dash::myid().id
#define EXPECT_FLOAT_EQ_U(e,a)  \
  EXPECT_FLOAT_EQ(e,a)  << "Unit " << dash::myid().id

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

#if defined(DASH_ENABLE_TEST_LOGGING)

#define LOG_MESSAGE(...) do { \
  char buffer[300]; \
  const char * filepath = __FILE__; \
  const char * filebase = strrchr(filepath, '/'); \
  const char * filename = (filebase != 0) ? filebase + 1 : filepath; \
  sprintf(buffer, __VA_ARGS__); \
  testing::internal::ColoredPrintf( \
    testing::internal::COLOR_YELLOW, \
    "[= %*d LOG =] %*s :%*d | %s \n", \
    2, dash::myid().id, 24, filename, 4, __LINE__, \
    buffer); \
} while(0)

#else  // DASH_ENABLE_TEST_LOGGING

#define LOG_MESSAGE(...) do {  } while(0)

#endif // DASH_ENABLE_TEST_LOGGING

#define DASH_TEST_LOCAL_ONLY() do { \
  if (dash::myid() > 0) { \
    return; \
  } \
} while(0)

#define SCOPED_TRACE_MSG(msg) do { \
  SCOPED_TRACE(::testing::Message() << msg); \
} while(0)

#define SKIP_TEST()\
    if(dash::myid() == 0) {\
      std::cout << TEST_SKIPPED << "Warning: test skipped" \
                << std::endl;\
    }\
    return

#define SKIP_TEST_MSG(msg)\
    if(dash::myid() == 0) {\
      std::cout << TEST_SKIPPED << "Warning: test skipped: " << msg \
                << std::endl;\
    }\
    return


namespace dash {
namespace test {

class TestBase : public ::testing::Test {

 protected:

  virtual void SetUp() {
    LOG_MESSAGE("===> Running test case with %d units ...", dash::size());
    dash::init(&TESTENV.argc, &TESTENV.argv);
    LOG_MESSAGE("-==- DASH initialized");
    dash::barrier();
  }

  virtual void TearDown() {
    LOG_MESSAGE("-==- Test case finished at unit %d",       dash::myid().id);
    dash::Team::All().barrier();
    LOG_MESSAGE("-==- Finalize DASH at unit %d",            dash::myid().id);
    dash::finalize();
    LOG_MESSAGE("<=== Finished test case with %d units",    dash::size());
  }
};

} // namespace test
} // namespace dash


#endif // DASH__TEST__TEST_BASE_H_
