#ifndef DASH__UTIL__INTERNAL__TIMESTAMP_PAPI_H_
#define DASH__UTIL__INTERNAL__TIMESTAMP_PAPI_H_

#include <dash/internal/Config.h>

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
class TimestampPAPI;


/**
 * Timestamp counter based on PAPI, specialized for
 * clock-based time measurement.
 */
template<>
class TimestampPAPI<TimeMeasure::Clock> : public Timestamp
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
    if (timer_mode == 0) {
      value = static_cast<counter_t>(PAPI_get_real_usec());
    }
    else {
      value = static_cast<counter_t>(PAPI_get_virt_usec());
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
    return 1.0f;
  }

  inline static double FrequencyPrescale()
  {
    return 1.0f;
  }

  inline static const char * TimerName()
  {
    return "PAPI<Clock>";
  }

};

/**
 * Timestamp counter based on PAPI, specialized for
 * counter-based time measurement.
 */
template<>
class TimestampPAPI<TimeMeasure::Counter> : public Timestamp
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
  static void Calibrate(
    unsigned int mode   = 0,
    double       fscale = 1.0f)
  {
    timer_mode = mode;
    int retval = PAPI_library_init(PAPI_VER_CURRENT);
    if (retval != PAPI_VER_CURRENT && retval > 0) {
      throw ::std::runtime_error("PAPI version mismatch");
    }
    else if (retval < 0) {
      throw ::std::runtime_error("PAPI init failed");
    }
    const PAPI_hw_info_t * hwinfo = PAPI_get_hardware_info();
    if (hwinfo == NULL) {
      throw ::std::runtime_error("PAPI get hardware info failed");
    }
    frequencyScaling = hwinfo->cpu_max_mhz;
  }

public:
  TimestampPAPI() {
    if (timer_mode == 0) {
      value = static_cast<counter_t>(PAPI_get_real_cyc());
    }
    else {
      value = static_cast<counter_t>(PAPI_get_virt_cyc());
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
    return frequencyScaling;
  }

  inline static double FrequencyPrescale()
  {
    return 1.0f;
  }

  inline static const char * TimerName()
  {
    return "PAPI<Counter>";
  }

};

} // namespace internal
} // namespace util
} // namespace dash

#endif // DASH__UTIL__INTERNAL__TIMESTAMP_PAPI_H_
