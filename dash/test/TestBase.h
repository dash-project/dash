#ifndef DASH__TEST__TEST_BASE_H_
#define DASH__TEST__TEST_BASE_H_

#include <type_traits>

#include <gtest/gtest.h>

#include <dash/internal/Config.h>
#include <dash/internal/Logging.h>
#include <dash/internal/StreamConversion.h>

#include <dash/Init.h>
#include <dash/Team.h>
#include <dash/Types.h>
#include <dash/view/IndexSet.h>

#include "TestGlobals.h"
#include "TestLogHelpers.h"
#include "TestPrinter.h"

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
template <typename T, typename S>
typename std::enable_if<
    !std::is_floating_point<T>::value,
    ::testing::AssertionResult>::type
assert_float_eq(
    const char* /*exp_e*/,
    const char* /*exp_a*/,
    const T& /*val_e*/,
    const S& /*val_a*/)
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
struct float_type_helper<float, S> {
  using type = float;
};

template<typename S>
struct float_type_helper<S, double> {
  using type = double;
};
template<typename S>
struct float_type_helper<S, float> {
  using type = float;
};

template<>
struct float_type_helper<double, double> {
  using type = double;
};
template<>
struct float_type_helper<float, float> {
  using type = float;
};

template <
    class T,
    class S,
    bool isAnyFP =
        std::is_floating_point<T>::value || std::is_floating_point<S>::value>
class EQAsserter {
  using lhs_t = typename std::remove_cv<T>::type;
  using rhs_t = typename std::remove_cv<S>::type;

public:
  void operator()(lhs_t const & expected,
                  rhs_t const & actual,
                  const char *_file, int line)
  {
    EXPECT_EQ(expected, actual) << "Unit " << dash::myid().id << ": "
                                << _file << ":" << line;
  }
};

template <class T, class S>
class EQAsserter<T, S, true> {
  using value_t = typename ::testing::internal::float_type_helper<
      typename std::remove_cv<T>::type,
      typename std::remove_cv<S>::type>::type;

  using lhs_t = typename std::remove_cv<T>::type;
  using rhs_t = typename std::remove_cv<S>::type;

public:
  void operator()(lhs_t const& expected,
                  rhs_t const& actual,
                  const char *_file, int line)
  {
    if (std::is_same<value_t, double>::value) {
      EXPECT_DOUBLE_EQ(expected, actual) << "Unit " << dash::myid().id << ": "
                                         << _file << ":" << line;
    }
    else if (std::is_same<value_t, float>::value) {
      EXPECT_FLOAT_EQ(expected, actual)  << "Unit " << dash::myid().id << ": "
                                         << _file << ":" << line;
    }
  }
};

#define ASSERT_EQ_U(_e, _a)                                                \
  do {                                                                     \
    ::testing::internal::EQAsserter<decltype(_e), decltype(_a)>{}(_e, _a,  \
                                                                  __FILE__,\
                                                                  __LINE__); \
  } while (0)

#define EXPECT_EQ_U(e,a) ASSERT_EQ_U(e,a)


} // namespace internal
} // namespace testing

#if defined(DASH_ENABLE_TEST_LOGGING)

#define LOG_MESSAGE(...) do { \
  char buffer[300]; \
  const char * filepath = __FILE__; \
  const char * filebase = strrchr(filepath, '/'); \
  const char * filename = (filebase != 0) ? filebase + 1 : filepath; \
  sprintf(buffer, __VA_ARGS__); \
  printf( \
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

#define SCOPED_TRACE_MSG(msg)                    \
  do {                                           \
    SCOPED_TRACE(::testing::Message() << (msg)); \
  } while (0)

#define SKIP_TEST()\
    if(dash::myid() == 0) {\
      std::cout << TEST_SKIPPED << "Warning: test skipped" \
                << std::endl;\
    }\
    return

#define SKIP_TEST_MSG(msg)                                          \
  if (dash::myid() == 0) {                                          \
    std::cout << TEST_SKIPPED << "Warning: test skipped: " << (msg) \
              << std::endl;                                         \
  }                                                                 \
  return

namespace dash {
namespace test {

template <class ValueRange>
static std::string range_str(
  const ValueRange & vrange) {
  typedef typename ValueRange::value_type value_t;
  std::ostringstream ss;
  auto idx = dash::index(vrange);
  int        i   = 0;

  // ss << "<" << dash::internal::typestr(*vrange.begin()) << "> ";
  for (const auto & v : vrange) {
    ss << std::setw(2) << *(dash::begin(idx) + i) << "|"
       << std::fixed << std::setprecision(4)
       << static_cast<const value_t>(v) << " ";
    ++i;
  }
  return ss.str();
}

template <class ValueT, class RangeA, class RangeB>
static bool expect_range_values_equal(
  const RangeA & rng_a,
  const RangeB & rng_b) {
  DASH_LOG_TRACE_VAR("TestBase.expect_range_values_equal", rng_a);
  DASH_LOG_TRACE_VAR("TestBase.expect_range_values_equal", rng_b);
  auto       it_a  = dash::begin(rng_a);
  auto       it_b  = dash::begin(rng_b);
  const auto end_a = dash::end(rng_a);
  const auto end_b = dash::end(rng_b);
  for (; it_a != end_a && it_b != end_b; ++it_a, ++it_b) {
    if (static_cast<ValueT>(*it_a) !=
        static_cast<ValueT>(*it_b)) {
      return false;
    }
  }
  return (end_a == it_a) && (end_b == it_b);
}

class TestBase : public ::testing::Test {

 protected:
   void SetUp() override
   {
     const ::testing::TestInfo* const test_info =
         ::testing::UnitTest::GetInstance()->current_test_info();
     LOG_MESSAGE(
         "===> Running test case %s.%s ...",
         test_info->test_case_name(),
         test_info->name());
     dash::init(&TESTENV::argc, &TESTENV::argv);

     LOG_MESSAGE("-==- DASH initialized with %lu units", dash::size());
     dash::barrier();
  }

  void TearDown() override
  {
    auto myid = dash::myid();
    size_t size = dash::size();
    const ::testing::TestInfo* const test_info =
      ::testing::UnitTest::GetInstance()->current_test_info();

    LOG_MESSAGE("-==- Test case finished at unit %d", myid.id);

    dash::Team::All().barrier();
    LOG_MESSAGE("-==- Finalize DASH at unit %d", myid.id);
    dash::finalize();

    LOG_MESSAGE("<=== Finished test case %s.%s with %lu units",
                test_info->test_case_name(), test_info->name(),
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
