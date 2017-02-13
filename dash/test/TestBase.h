#ifndef DASH__TEST__TEST_BASE_H_
#define DASH__TEST__TEST_BASE_H_

#include <type_traits>

#include <gtest/gtest.h>

#include <dash/Init.h>
#include <dash/Team.h>
#include <dash/internal/Logging.h>

#include "TestGlobals.h"
#include "TestPrinter.h"
#include "TestLogHelpers.h"


namespace testing {
namespace internal {

#define ASSERT_FAIL() EXPECT_EQ(0, 1) << "ASSERT_FAIL"

#define ASSERT_TRUE_U(b)  EXPECT_TRUE(b)  << "Unit " << dash::myid().id
#define ASSERT_FALSE_U(b) EXPECT_FALSE(b) << "Unit " << dash::myid().id
#define ASSERT_NE_U(e,a)  EXPECT_NE(e,a)  << "Unit " << dash::myid().id
#define ASSERT_LT_U(e,a)  EXPECT_LT(e,a)  << "Unit " << dash::myid().id
#define ASSERT_GT_U(e,a)  EXPECT_GT(e,a)  << "Unit " << dash::myid().id
#define ASSERT_LE_U(e,a)  EXPECT_LE(e,a)  << "Unit " << dash::myid().id
#define ASSERT_GE_U(e,a)  EXPECT_GE(e,a)  << "Unit " << dash::myid().id

#define EXPECT_TRUE_U(b)  EXPECT_TRUE(b)  << "Unit " << dash::myid().id
#define EXPECT_FALSE_U(b) EXPECT_FALSE(b) << "Unit " << dash::myid().id
#define EXPECT_NE_U(e,a)  EXPECT_NE(e,a)  << "Unit " << dash::myid().id
#define EXPECT_LT_U(e,a)  EXPECT_LT(e,a)  << "Unit " << dash::myid().id
#define EXPECT_GT_U(e,a)  EXPECT_GT(e,a)  << "Unit " << dash::myid().id
#define EXPECT_LE_U(e,a)  EXPECT_LE(e,a)  << "Unit " << dash::myid().id
#define EXPECT_GE_U(e,a)  EXPECT_GE(e,a)  << "Unit " << dash::myid().id

/**
 * This is used in the general case. It evaluates to a failing test,
 * which is OK because assert_float_eq is only called for floating point
 * types. It's a wrapper around CmpHelperFloatingPointEQ, which cannot
 * be called on arbitrary types.
 *
 * TODO: A warning is issued if the expected value is literal NULL.
 *       GTest seems to have a workaround for that case, which we might
 *       adopt.
 */
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wconversion-null"
#endif // defined(__GNUC__)
template<typename T, typename S>
typename std::enable_if<!std::is_floating_point<T>::value, ::testing::AssertionResult>::type
assert_float_eq(
  const char *exp_e,
  const char *exp_a,
  const T& val_e,
  const S& val_a)
{
  // return success for types other than floats
  return ::testing::AssertionFailure() << "Wrong type for assert_float_eq()";
}

/**
 * Internally calls CmpHelperFloatingPointEQ provided by GTest.
 */
template<typename T>
typename std::enable_if<std::is_floating_point<T>::value,
              ::testing::AssertionResult>::type
assert_float_eq(
  const char *exp_e,
  const char *exp_a,
  T val_e,
  T val_a)
{
  return ::testing::internal::CmpHelperFloatingPointEQ<T>(
                                  exp_e, exp_a, val_e, val_a);
}

/**
 * float_type_helper is used to pick double over float.
 */
template<typename T, typename S>
struct float_type_helper {
  using type = T;
};

template<typename S>
struct float_type_helper<double, S> {
  using type = double;
};

template<typename S>
struct float_type_helper<S, double> {
  using type = double;
};

template<>
struct float_type_helper<double, double> {
  using type = double;
};

#define ASSERT_EQ_U(e,a)                                         \
  do {                                                           \
    if (std::is_floating_point<decltype(e)>::value               \
     || std::is_floating_point<decltype(a)>::value) {            \
      using value_type =                                         \
              typename ::testing::internal::float_type_helper<   \
                                decltype(e), decltype(a)>::type; \
      EXPECT_PRED_FORMAT2(                                       \
        ::testing::internal::assert_float_eq<value_type>, e, a)  \
            << "Unit " << dash::myid().id;                       \
    }                                                            \
    else {                                                       \
      EXPECT_EQ(e,a)        << "Unit " << dash::myid().id;       \
    }                                                            \
  } while(0)

#define EXPECT_EQ_U(e,a) ASSERT_EQ_U(e,a)


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
    const ::testing::TestInfo* const test_info =
      ::testing::UnitTest::GetInstance()->current_test_info();
    LOG_MESSAGE("===> Running test case %s:%s with %lu units ...",
                test_info->name(), test_info->test_case_name(),
                dash::size());
    dash::init(&TESTENV.argc, &TESTENV.argv);
    LOG_MESSAGE("-==- DASH initialized with %lu units", dash::size());
    dash::barrier();
  }

  virtual void TearDown() {
    const ::testing::TestInfo* const test_info =
      ::testing::UnitTest::GetInstance()->current_test_info();
    LOG_MESSAGE("-==- Test case finished at unit %d", dash::myid().id);

    dash::Team::All().barrier();
    LOG_MESSAGE("-==- Finalize DASH at unit %d", dash::myid().id);
    dash::finalize();

    size_t size = dash::size();
    LOG_MESSAGE("<=== Finished test case %s:%s with %lu units",
                test_info->name(), test_info->test_case_name(),
                size);
  }

  std::string _hostname() const {
    char hostname[100];
    gethostname(hostname, 100);
    return std::string(hostname);
  }

  int _pid() const {
    return static_cast<int>(getpid());
  }
};

} // namespace test
} // namespace dash


#endif // DASH__TEST__TEST_BASE_H_
