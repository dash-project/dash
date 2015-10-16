#ifndef DASH__UTIL__INTERNAL__TIMESTAMP_PAPI_H_
#define DASH__UTIL__INTERNAL__TIMESTAMP_PAPI_H_

#include <dash/internal/Config.h>

#include <dash/util/Timer.h>
#include <dash/util/TimeMeasure.h>
#include <dash/util/Timestamp.h>

#include <papi.h>

#include <stdint.h>
#include <iostream>
#include <stdexcept>

namespace dash {
namespace util {
namespace internal {

/**
 * Timestamp counter based on PAPI
 */
template<TimeMeasure::MeasureMode TimerType>
class TimestampPAPI : public Timestamp
{
private: 
  Timestamp::counter_t         value; 
  static int                   timer_mode; 

public: 
  static Timestamp::counter_t  frequencyScaling; 

public: 
  /** 
   * Initializes the PAPI library.
   *
   * \param  arg  0 for real (usec), 1 for virt (usec)
   */
  static void Calibrate(unsigned int arg = 0) { 
    timer_mode = arg; 
    int retval = PAPI_library_init(PAPI_VER_CURRENT); 
    if (retval != PAPI_VER_CURRENT && retval > 0) {
      throw ::std::runtime_error("PAPI version mismatch");
    }
    else if (retval < 0) { 
      throw ::std::runtime_error("PAPI init failed");
    }
  } 

public:
  TimestampPAPI() {
    if (TimerType == TimeMeasure::Clock) {
      if (timer_mode == 0) { 
        value = static_cast<counter_t>(PAPI_get_real_usec());
      }
      else {
        value = static_cast<counter_t>(PAPI_get_virt_usec());
      }
    }
    else if (TimerType == TimeMeasure::Counter) {
      if (timer_mode == 0) { 
        value = static_cast<counter_t>(PAPI_get_real_cyc());
      }
      else {
        value = static_cast<counter_t>(PAPI_get_virt_cyc());
      }
    }
  }

  inline TimestampPAPI(
    const TimestampPAPI & other)
  : value(other.value)
  { }

  inline TimestampPAPI(
    const counter_t & counterValue)
  : value(counterValue)
  { }

  inline TimestampPAPI & operator=(
    const TimestampPAPI rhs)
  {
    if (this != &rhs) {
      value = rhs.value; 
    }
    return *this; 
  }

  inline const counter_t & Value() const
  {
    return value;
  }

  inline static double FrequencyScaling()
  {
    if (TimerType == TimeMeasure::Counter) { 
      return 996.0f; // clock speed on Wandboard
    }
    return 1.0f; 
  }

  inline static double FrequencyPrescale()
  {
    return 1.0f;
  }

  inline static const char * TimerName()
  {
    return "PAPI"; 
  }

};

} // namespace internal
} // namespace util
} // namespace dash

#include <dash/util/internal/TimestampPAPI-inl.h>

#endif // DASH__UTIL__INTERNAL__TIMESTAMP_PAPI_H_
