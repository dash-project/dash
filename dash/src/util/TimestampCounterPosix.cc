#include <dash/util/Timer.h>

#if defined(DASH__UTIL__TIMER_POSIX) || \
    defined(DASH__UTIL__TIMER_UX)

#include <dash/util/internal/TimestampCounterPosix.h>

namespace dash {
namespace util {
namespace internal {

void TimestampCounterPosix::Calibrate(unsigned int freq) {
  frequencyScaling = freq == 0
                     ? 1900.0f 
                     : static_cast<double>(freq); 

  ::std::cout << "   RDTSC timer | " << TimerName() << std::endl;
  ::std::cout << "   RDTSC scale | F:" << frequencyScaling << std::endl;
}

Timestamp::counter_t TimestampCounterPosix::frequencyScaling = 1; 

} // namespace internal
} // namespace util
} // namespace dash

#endif // DASH__UTIL__TIMER_POSIX || DASH__UTIL__TIMER_UX
