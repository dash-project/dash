#ifndef DASH__EXCEPTION_H_
#define DASH__EXCEPTION_H_

#include <dash/exception/RuntimeError.h>
#include <dash/exception/InvalidArgument.h>
#include <dash/exception/OutOfRange.h>
#include <dash/exception/NotImplemented.h>
#include <dash/exception/AssertionFailed.h>
#include <sstream>

#define DASH_THROW(excep_type, msg_stream) do { \
    ::std::ostringstream os; \
    os << msg_stream; \
    throw(excep_type(os.str())); \
  } while(0)

#define DASH_ASSERT_RETURNS(expr, exp_value) do { \
  if ((expr) != (exp_value)) { \
    DASH_THROW(dash::exception::AssertionFailed, \
               "Assertion failed: Expected " << (exp_value) << \
               __FILE__ << ":" << __LINE__); \
  }\
} while(0)

#define DASH_ASSERT(expr) do { \
  if (!(expr)) { \
    DASH_THROW(dash::exception::AssertionFailed, \
               "Assertion failed: " << \
               __FILE__ << ":" << __LINE__); \
  }\
} while(0)

#endif // DASH__EXCEPTION_H_
