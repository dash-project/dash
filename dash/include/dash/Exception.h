#ifndef DASH__EXCEPTION_H_
#define DASH__EXCEPTION_H_

#include <dash/exception/RuntimeError.h>
#include <dash/exception/InvalidArgument.h>
#include <dash/exception/OutOfRange.h>
#include <dash/exception/NotImplemented.h>
#include <dash/exception/AssertionFailed.h>
#include <dash/exception/StackTrace.h>

#include <dash/internal/Macro.h>
#include <dash/internal/Logging.h>

#include <sstream>

#define DASH_STACK_TRACE() do { \
    dash__print_stacktrace(); \
  } while(0)

#define DASH_THROW(excep_type, msg_stream) do { \
    ::std::ostringstream os; \
    os << "[ Unit " << dash::myid() << " ] "; \
    os << msg_stream; \
    DASH_LOG_ERROR(dash__toxstr(excep_type), os.str()); \
    throw(excep_type(os.str())); \
  } while(0)

#define DASH_ASSERT_ALWAYS(expr) do { \
  if (!(expr)) { \
    DASH_THROW(dash::exception::AssertionFailed, \
               "Assertion failed: " \
               << " " << __FILE__ << ":" << __LINE__); \
  }\
} while(0)

#define DASH_ASSERT_MSG_ALWAYS(expr, msg) do { \
  if (!(expr)) { \
    DASH_THROW(dash::exception::AssertionFailed, \
               "Assertion failed: " \
               << " " << __FILE__ << ":" << __LINE__); \
  }\
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

// Using (value+1) < (lower+1) to avoid compiler warning for unsigned.
#define DASH_ASSERT_RANGE(lower, value, upper, message) do { \
  if ((value) > (upper) || (value+1) < (lower+1)) { \
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

#define DASH_ASSERT_NE(val_a, val_b, message) do { \
  if ((val_a) == (val_b)) { \
    DASH_THROW(dash::exception::AssertionFailed, \
               "Assertion " \
               << val_a << " != " << val_b \
               << " failed: " \
               << message << " " \
               << __FILE__ << ":" << __LINE__); \
  } \
} while(0)

// Using (value+1) < (min+1) to avoid compiler warning for unsigned.
#define DASH_ASSERT_GT(value, min, message) do { \
  if (((value)+1) <= ((min)+1)) { \
    DASH_THROW(dash::exception::OutOfRange, \
               "Range assertion " \
               << value << " > " << min \
               << " failed: " \
               << message << " " \
               << __FILE__ << ":" << __LINE__); \
  } \
} while(0)

// Using (value+1) < (min+1) to avoid compiler warning for unsigned.
#define DASH_ASSERT_GE(value, min, message) do { \
  if (((value)+1) < ((min)+1)) { \
    DASH_THROW(dash::exception::OutOfRange, \
               "Range assertion " \
               << value << " >= " << min \
               << " failed: " \
               << message << " " \
               << __FILE__ << ":" << __LINE__); \
  } \
} while(0)

// Using (value+1) >= (max+1) to avoid compiler warning for unsigned.
#define DASH_ASSERT_LT(value, max, message) do { \
  if (((value)+1) >= ((max)+1)) { \
    DASH_THROW(dash::exception::OutOfRange, \
               "Range assertion " \
               << value << " < " << max \
               << " failed: " \
               << message << " "\
               << __FILE__ << ":" << __LINE__); \
  } \
} while(0)

// Using (value+1) > (max+1) to avoid compiler warning for unsigned.
#define DASH_ASSERT_LE(value, max, message) do { \
  if (((value)+1) > ((max)+1)) { \
    DASH_THROW(dash::exception::OutOfRange, \
               "Range assertion " \
               << value << " <= " << max \
               << " failed: " \
               << message << " "\
               << __FILE__ << ":" << __LINE__); \
  } \
} while(0)

#define DASH_ASSERT_NOEXCEPT

#else  // DASH_ENABLE_ASSERTIONS

#define DASH_ASSERT(expr) do { } while (0)
#define DASH_ASSERT_MSG(expr, msg) do { } while (0)
#define DASH_ASSERT_RETURNS(expr, exp_value) do { \
          (expr); \
          dash__unused(exp_value); \
        } while(0)
#define DASH_ASSERT_RANGE(lower, value, upper, message) do { \
          dash__unused(lower);   \
          dash__unused(value);   \
          dash__unused(upper);   \
        } while(0)
#define DASH_ASSERT_EQ(val, exp, message) do { \
          dash__unused(val); \
          dash__unused(exp); \
        } while (0)
#define DASH_ASSERT_NE(val, exp, message) do { \
          dash__unused(val); \
          dash__unused(exp); \
        } while (0)
#define DASH_ASSERT_GT(val, min, message) do { \
          dash__unused(val); \
          dash__unused(min); \
        } while (0)
#define DASH_ASSERT_GE(val, min, message) do { \
          dash__unused(val); \
          dash__unused(min); \
        } while (0)
#define DASH_ASSERT_LT(val, max, message) do { \
          dash__unused(val); \
          dash__unused(max); \
        } while (0)
#define DASH_ASSERT_LE(val, max, message) do { \
          dash__unused(val); \
          dash__unused(max); \
        } while (0)

#define DASH_ASSERT_NOEXCEPT noexcept

#endif // DASH_ENABLE_ASSERTIONS

#endif // DASH__EXCEPTION_H_
