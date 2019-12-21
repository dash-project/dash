#ifndef DASH__UTIL__TIMER_H_
#define DASH__UTIL__TIMER_H_

#include <dash/internal/Config.h>

// Timestamp interfaces:
#include <dash/util/Timestamp.h>
#include <dash/util/TimeMeasure.h>

#include <climits>

// Timestamp implementations:
#if defined(DASH__UTIL__TIMER_PAPI)
#  include <dash/util/internal/TimestampPAPI.h>
#else
#  include <dash/util/internal/TimestampCounterPosix.h>
#  include <dash/util/internal/TimestampClockPosix.h>
#endif

#include <dash/internal/Logging.h>

namespace dash {
namespace util {

template<TimeMeasure::MeasureMode TimerType>
class Timer;

//////////////////////////////////////////////////////////////////////////////
// Specialization for clock-based timer

template<>
class Timer<TimeMeasure::Clock> {
public:
  typedef Timestamp::counter_t timestamp_t;

private:
  typedef Timer self_t;

private:
  timestamp_t timestampStart;

private:
#if defined(DASH__UTIL__TIMER_PAPI)
  // PAPI support, use measurements from PAPI
  typedef dash::util::internal::TimestampPAPI<TimeMeasure::Clock>
    Timestamp_t;
#elif defined(DASH__UTIL__TIMER_POSIX)
  // POSIX platform
  typedef dash::util::internal::TimestampClockPosix
    Timestamp_t;
#else
  // No PAPI, no POSIX
  #error "dash::util::Timer requires POSIX platform or PAPI"
#endif

public:
  typedef Timestamp_t timestamp_type;
  typedef timestamp_t timestamp;

public:
  inline Timer()
    : timestampStart(Timer::Now())
  {
  }

  inline Timer(const self_t& other) = default;

  inline Timer & operator=(const self_t & other)
  {
    if (this != &other) {
      timestampStart = other.timestampStart;
    }
    return *this;
  }

  /**
   * Microseconds elapsed since instantiation of this Timer object.
   */
  inline double Elapsed() const
  {
    timestamp_t now;
    Timestamp_t timestamp;
    now = timestamp.Value();
    return (static_cast<double>(now - timestampStart) *
      static_cast<double>(Timestamp_t::FrequencyPrescale())) /
      static_cast<double>(Timestamp_t::FrequencyScaling());
  }

  /**
   * Returns timestamp from instantiation of this Timer.
   */
  inline const timestamp_t & Start() const
  {
    return timestampStart;
  }

  /**
   * Microseconds elapsed since given timestamp.
   */
  inline static double ElapsedSince(timestamp_t timestamp)
  {
    Timestamp_t now;
    return (static_cast<double>(now.Value() - timestamp) *
      static_cast<double>(Timestamp_t::FrequencyPrescale())) /
      static_cast<double>(Timestamp_t::FrequencyScaling());
  }

  /**
   * Produces current timestamp.
   */
  inline static timestamp_t Now()
  {
    Timestamp_t timestamp;
    return timestamp.Value();
  }

  /**
   * Convert interval of two timestamp values to microseconds.
   */
  inline static double FromInterval(
    const timestamp_t & start,
    const timestamp_t & end)
  {
    return (static_cast<double>(end - start) *
      static_cast<double>(Timestamp_t::FrequencyPrescale())) /
      static_cast<double>(Timestamp_t::FrequencyScaling());
  }

  /**
   * Convert interval of two timestamp values to mircoseconds.
   */
  inline static double FromInterval(
    const double & start,
    const double & end)
  {
    return ((end - start) *
      Timestamp_t::FrequencyPrescale() /
      Timestamp_t::FrequencyScaling());
  }

  inline static void Calibrate(
    unsigned int freq = 0)
  {
    DASH_LOG_DEBUG("Timer<Clock>::Calibrate(freq)", freq);
    Timestamp_t::Calibrate(freq);
  }

  inline static const char * TimerName()
  {
    return Timestamp_t::TimerName();
  }

  inline static Timestamp::counter_t TimestampInfinity()
  {
    return Timestamp_t::TimestampInfinity();
  }

  inline static Timestamp::counter_t TimestampNegInfinity() {
    return Timestamp_t::TimestampNegInfinity();
  }

  inline static double FrequencyScaling()
  {
    return Timestamp_t::FrequencyScaling();
  }
};

//////////////////////////////////////////////////////////////////////////////
// Specialization for counter-based timer

template<>
class Timer<TimeMeasure::Counter> {
public:
  typedef Timestamp::counter_t timestamp_t;

private:
  typedef Timer self_t;

private:
  timestamp_t timestampStart;

private:
#if defined(DASH__UTIL__TIMER_PAPI)
  // PAPI support, use measurements from PAPI
  typedef dash::util::internal::TimestampPAPI<TimeMeasure::Counter>
    Timestamp_t;
#elif defined(DASH__UTIL__TIMER_POSIX)
  // POSIX platform
  typedef dash::util::internal::TimestampCounterPosix
    Timestamp_t;
#else
  // No PAPI, no POSIX
  #error "dash::util::Timer requires POSIX platform or PAPI"
#endif

public:
  typedef Timestamp_t timestamp_type;
  typedef timestamp_t timestamp;

public:
  inline Timer()
  : timestampStart(Timer::Now())
  { }

  inline Timer(const self_t& other) = default;

  inline Timer & operator=(const self_t & other)
  {
    if (this != &other) {
      timestampStart = other.timestampStart;
    }
    return *this;
  }

  /**
   * Microseconds elapsed since instantiation of this Timer object.
   */
  inline double Elapsed() const
  {
    timestamp_t now;
    Timestamp_t timestamp;
    now = timestamp.Value();
    return (static_cast<double>(now - timestampStart) *
      static_cast<double>(Timestamp_t::FrequencyPrescale())) /
      static_cast<double>(Timestamp_t::FrequencyScaling());
  }

  /**
   * Returns timestamp from instantiation of this Timer.
   */
  inline const timestamp_t & Start() const
  {
    return timestampStart;
  }

  /**
   * Microseconds elapsed since given timestamp.
   */
  inline static double ElapsedSince(timestamp_t timestamp)
  {
    Timestamp_t now;
    return (static_cast<double>(now.Value() - timestamp) *
      static_cast<double>(Timestamp_t::FrequencyPrescale())) /
      static_cast<double>(Timestamp_t::FrequencyScaling());
  }

  /**
   * Produces current timestamp.
   */
  inline static timestamp_t Now()
  {
    Timestamp_t timestamp;
    return timestamp.Value();
  }

  /**
   * Convert interval of two timestamp values to mircoseconds.
   */
  inline static double FromInterval(
    const timestamp_t & start,
    const timestamp_t & end)
  {
    return (static_cast<double>(end - start) *
      static_cast<double>(Timestamp_t::FrequencyPrescale())) /
      static_cast<double>(Timestamp_t::FrequencyScaling());
  }

  /**
   * Convert interval of two timestamp values to mircoseconds.
   */
  inline static double FromInterval(
    const double & start,
    const double & end)
  {
    return ((end - start) *
      Timestamp_t::FrequencyPrescale() /
      Timestamp_t::FrequencyScaling());
  }

  inline static void Calibrate(
    unsigned int freq = 0)
  {
    DASH_LOG_DEBUG("Timer<Counter>::Calibrate(freq)", freq);
    Timestamp_t::Calibrate(freq);
  }

  inline static const char * TimerName()
  {
    return Timestamp_t::TimerName();
  }

  inline static Timestamp::counter_t TimestampInfinity()
  {
    return Timestamp_t::TimestampInfinity();
  }

  inline static Timestamp::counter_t TimestampNegInfinity()
  {
    return Timestamp_t::TimestampNegInfinity();
  }

  inline static double FrequencyScaling()
  {
    return Timestamp_t::FrequencyScaling();
  }
};

} // namespace util
} // namespace dash

#endif // DASH__UTIL__TIMER_H_
