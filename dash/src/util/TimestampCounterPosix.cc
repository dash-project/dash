#if defined(DASH__UTIL__TIMER_POSIX) || \
    defined(DASH__UTIL__TIMER_UX)

#include <dash/util/Timer.h>
#include <dash/util/internal/TimestampCounterPosix.h>
#include <dash/internal/Logging.h>

namespace dash {
namespace util {
namespace internal {

void TimestampCounterPosix::Calibrate(unsigned int freq) {
  frequencyScaling = freq == 0
                     ? 1900.0f
                     : static_cast<double>(freq);
  DASH_LOG_DEBUG("TimestampCounterPosix::Calibrate(freq)", freq);
  DASH_LOG_DEBUG("TimestampCounterPosix::Calibrate",
                 "timer:", TimerName());
  DASH_LOG_DEBUG("TimestampCounterPosix::Calibrate",
                 "fscale:", frequencyScaling);
}

Timestamp::counter_t TimestampCounterPosix::frequencyScaling = 1;

} // namespace internal
} // namespace util
} // namespace dash

#endif // DASH__UTIL__TIMER_POSIX || DASH__UTIL__TIMER_UX
