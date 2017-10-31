#ifndef DASH__TEST__TEST_BASE_H_
#define DASH__TEST__TEST_BASE_H_

#include <type_traits>

#include <gtest/gtest.h>

#include <dash/dart/if/dart_tasking.h>

#include <dash/internal/Config.h>
#include <dash/internal/Logging.h>
#include <dash/internal/StreamConversion.h>

#include <dash/Types.h>
#include <dash/Init.h>

#include <dash/view/IndexSet.h>
#include <dash/view/ViewMod.h>
#include <dash/view/Sub.h>

#include <dash/Team.h>

#include "TestGlobals.h"
#include "TestPrinter.h"
#include "TestLogHelpers.h"

#include <sstream>
#include <iomanip>
#include <string>



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
typename std::enable_if<
           !std::is_floating_point<T>::value,
           ::testing::AssertionResult
         >::type
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

#define ASSERT_EQ_U(_e,_a)                                          \
  do {                                                              \
    if (std::is_floating_point<decltype(_e)>::value                 \
     || std::is_floating_point<decltype(_a)>::value) {              \
      using value_type =                                            \
              typename ::testing::internal::float_type_helper<      \
                                decltype(_e), decltype(_a)>::type;  \
      EXPECT_PRED_FORMAT2(                                          \
        ::testing::internal::assert_float_eq<value_type>,(_e),(_a)) \
            << "Unit " << dash::myid().id;                          \
    }                                                               \
    else {                                                          \
      EXPECT_EQ(_e,_a)        << "Unit " << dash::myid().id;        \
    }                                                               \
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
    "[= %3d:%-2d LOG =] %*s :%*d | %s \n", \
    dash::myid().id, \
    dart_task_thread_num ? dart_task_thread_num() : 0, \
    24, filename, 4, __LINE__, \
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

template <class NViewType>
std::string nview_str(
  const NViewType   & nview) {
  using value_t   = typename NViewType::value_type;
  auto view_nrows = nview.extents()[0];
  auto view_ncols = nview.extents()[1];
  auto nindex     = dash::index(nview);
  std::ostringstream ss;
  for (int r = 0; r < view_nrows; ++r) {
    for (int c = 0; c < view_ncols; ++c) {
      int offset = r * view_ncols + c;
      ss << std::fixed << std::setw(3)
         << nindex[offset]
         << ":"
         << std::fixed << std::setprecision(5)
         << static_cast<const value_t>(nview[offset])
         << " ";
    }
    ss << '\n';
  }
  return ss.str();
}

template <class NViewType>
std::string nrange_str(
  const NViewType   & nview) {
  using value_t   = typename NViewType::value_type;
  auto view_nrows = nview.extents()[0];
  auto view_ncols = nview.extents()[1];
  std::ostringstream ss;
  for (int r = 0; r < view_nrows; ++r) {
    auto row_view = dash::sub<0>(r, r+1, nview);
    for (int c = 0; c < row_view.size(); ++c) {
      int offset = r * view_ncols + c;
      ss << std::fixed << std::setw(3)
         << offset << ":"
         << std::fixed << std::setprecision(5)
         << static_cast<const value_t &>(row_view[c])
         << " ";
    }
    ss << '\n';
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

template <class ValueT, class IteratorA, class SentinelA, class IteratorB>
static bool expect_range_values_equal(
  const IteratorA & rng_a_begin,
  const SentinelA & rng_a_end,
  const IteratorB & rng_b_begin) {
  auto       it_a  = rng_a_begin;
  auto       it_b  = rng_b_begin;
  const auto end_a = rng_a_end;
  const auto end_b = rng_b_begin + dash::distance(it_a, end_a);

  const auto rng_a = dash::make_range(it_a, end_a);
  const auto rng_b = dash::make_range(it_b, end_b);

  DASH_LOG_TRACE_VAR("TestBase.expect_range_values_equal", rng_a);
  DASH_LOG_TRACE_VAR("TestBase.expect_range_values_equal", rng_b);
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

  virtual void SetUp() {
    const ::testing::TestInfo* const test_info =
      ::testing::UnitTest::GetInstance()->current_test_info();
    LOG_MESSAGE("===> Running test case %s.%s ...",
                test_info->test_case_name(), test_info->name());
    dash::init(&TESTENV::argc, &TESTENV::argv);
    
    LOG_MESSAGE("-==- DASH initialized with %lu units", dash::size());
    dash::barrier();
  }

  virtual void TearDown() {
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
