#ifndef DASH__EXCEPTION_H_
#define DASH__EXCEPTION_H_

#include <dash/exception/RuntimeError.h>
#include <dash/exception/InvalidArgument.h>
#include <dash/exception/OutOfRange.h>
#include <dash/exception/NotImplemented.h>
#include <dash/exception/AssertionFailed.h>
#include <dash/exception/StackTrace.h>
#include <sstream>

#define DASH_STACK_TRACE() do { \
    dash__print_stacktrace(); \
  } while(0)

#define DASH_THROW(excep_type, msg_stream) do { \
    ::std::ostringstream os; \
    os << "[ Unit " << dash::myid() << " ] "; \
    os << msg_stream; \
    throw(excep_type(os.str())); \
  } while(0)

#if defined(DASH_ENABLE_ASSERTIONS)

#define DASH_ASSERT(expr) do { \
  if (!(expr)) { \
    DASH_THROW(dash::exception::AssertionFailed, \
               "Assertion failed: " \
               << " " << __FILE__ << ":" << __LINE__); \
  }\
} while(0)

#define DASH_ASSERT_MSG(expr, msg) do { \
  if (!(expr)) { \
    DASH_THROW(dash::exception::AssertionFailed, \
               "Assertion failed: " << msg \
               << " " << __FILE__ << ":" << __LINE__); \
  }\
} while(0)

#define DASH_ASSERT_RETURNS(expr, exp_value) do { \
  if ((expr) != (exp_value)) { \
    DASH_THROW(dash::exception::AssertionFailed, \
               "Assertion failed: Expected " << (exp_value) \
               << " " << __FILE__ << ":" << __LINE__); \
  }\
} while(0)

#define DASH_ASSERT_RANGE(lower, value, upper, message) do { \
  if ((value) > (upper) || (value) < (lower)) { \
    DASH_THROW(dash::exception::OutOfRange, \
               "Range assertion " \
               << lower << " <= " << value << " <= " << upper \
               << " failed: " \
               << message << " "\
               << __FILE__ << ":" << __LINE__); \
  }\
} while(0)

#define DASH_ASSERT_EQ(val_a, val_b, message) do { \
  if ((val_a) != (val_b)) { \
    DASH_THROW(dash::exception::AssertionFailed, \
               "Assertion " \
               << val_a << " == " << val_b \
               << " failed: " \
               << message << " " \
               << __FILE__ << ":" << __LINE__); \
  } \
} while(0)

#define DASH_ASSERT_GT(value, min, message) do { \
  if ((value+1) <= (min+1)) { \
    DASH_THROW(dash::exception::OutOfRange, \
               "Range assertion " \
               << value << " > " << min \
               << " failed: " \
               << message << " " \
               << __FILE__ << ":" << __LINE__); \
  } \
} while(0)

#define DASH_ASSERT_LT(value, max, message) do { \
  if ((value) >= (max)) { \
    DASH_THROW(dash::exception::OutOfRange, \
               "Range assertion " \
               << value << " < " << max \
               << " failed: " \
               << message << " "\
               << __FILE__ << ":" << __LINE__); \
  } \
} while(0)

#else  // DASH_ENABLE_ASSERTIONS

#define DASH_ASSERT(expr) do { } while (expr)
#define DASH_ASSERT_MSG(expr, msg) do { } while (expr)
#define DASH_ASSERT_RETURNS(expr, exp_value) (expr)
#define DASH_ASSERT_RANGE(lower, value, upper, message) do { } while(0)
#define DASH_ASSERT_EQ(val, min, message) do { } while (0)
#define DASH_ASSERT_GT(val, min, message) do { } while (0)
#define DASH_ASSERT_LT(val, max, message) do { } while (0)

#endif // DASH_ENABLE_ASSERTIONS

#endif // DASH__EXCEPTION_H_
