#ifndef DASH__UTIL__INTERNAL__TIMESTAMP_PAPI_INL_H_
#define DASH__UTIL__INTERNAL__TIMESTAMP_PAPI_INL_H_

#if defined(DASH__UTIL__TIMER_PAPI)

#include <dash/util/internal/TimestampPAPI.h>

namespace dash {
namespace util {
namespace internal {

template<TimeMeasure::MeasureMode TimerType>
int TimestampPAPI<TimerType>::timer_mode = 0; 

} // namespace internal
} // namespace util
} // namespace dash

#endif // DASH__UTIL__TIMER_PAPI

#endif // DASH__UTIL__INTERNAL__TIMESTAMP_PAPI_INL_H_
