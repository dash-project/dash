#ifndef DASH__UTIL__TIMESTAMP_H_
#define DASH__UTIL__TIMESTAMP_H_

#include <dash/internal/Config.h>
#include <limits.h>

// OS X
#if defined(DASH__PLATFORM__OSX)
#define DASH__UTIL__TIMER_OSX
#endif
// HPUX / Sun
#if defined(DASH__PLATFORM__UX)
#define DASH__UTIL__TIMER_UX
#endif
// POSIX
#if defined(DASH__PLATFORM__POSIX)
#define DASH__UTIL__TIMER_POSIX
#endif
// Linux
#if defined(__linux__)
#define DASH__UTIL__TIMER_LINUX
#endif
// FreeBSD
#if defined(__FreeBSD__)
#define DASH__UTIL__TIMER_FREEBSD
#endif

#if defined(DASH_ENABLE_PAPI)
#define DASH__UTIL__TIMER_PAPI
#endif

namespace dash {
namespace util {

class Timestamp {
 public:
  typedef unsigned long long counter_t;

 public:
  virtual ~Timestamp() { };
  virtual const counter_t & Value() const = 0;

  static double FrequencyScaling();
  static double FrequencyPrescale();
  static const char * VariantName();
  inline static counter_t TimestampInfinity() {
    return LLONG_MAX;
  }
  inline static counter_t TimestampNegInfinity() {
    return 0;
  }
};

} // namespace util
} // namespace dash

#endif // DASH__UTIL__TIMESTAMP_H_
