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

namespace dash {
namespace util {

class Timer {
 public: 
  typedef Timestamp::counter_t timestamp_t;

 private:
  typedef Timer self_t;
  static dash::util::TimeMeasure::MeasureMode Type;
  timestamp_t timestampStart;

 private: 
#if defined(DASH__UTIL__TIMER_PAPI)
// PAPI support, use measurements from PAPI 
  typedef dash::util::internal::TimestampPAPI<TimeMeasure::Clock>
    TimestampClockBased;
  typedef dash::util::internal::TimestampPAPI<TimeMeasure::Counter>
    TimestampCounterBased;
#elif defined(DASH__UTIL__TIMER_POSIX)
// POSIX platform
  typedef dash::util::internal::TimestampCounterPosix 
    TimestampCounterBased;
  typedef dash::util::internal::TimestampClockPosix 
    TimestampClockBased;
# else
// No PAPI, no POSIX
#  pragma error "dash::util::Timer requires POSIX platform or PAPI"
#endif

 public:
  inline Timer() {
    timestampStart = Timer::Now(); 
  }

  inline Timer(const self_t & other) : timestampStart(other.timestampStart)
  { }

  inline Timer & operator=(const self_t & other) {
    if (this != &other) {
      timestampStart = other.timestampStart;
    }
    return *this;
  }

  /**
   * Microseconds elapsed since instantiation of this Timer object.
   */
  inline double Elapsed() const {
    timestamp_t now;

    if (Timer::Type == TimeMeasure::Counter) {
      TimestampCounterBased timestamp;
      now = timestamp.Value();
      return (static_cast<double>(now - timestampStart) *
        static_cast<double>(TimestampCounterBased::FrequencyPrescale())) /
        static_cast<double>(TimestampCounterBased::FrequencyScaling());
    }
    if (Timer::Type == TimeMeasure::Clock) {
      TimestampClockBased timestamp;
      now = timestamp.Value();
      return (static_cast<double>(now - timestampStart) *
        static_cast<double>(TimestampClockBased::FrequencyPrescale())) /
        static_cast<double>(TimestampClockBased::FrequencyScaling());
    }
    return 0.0f; 
  }

  /**
   * Returns timestamp from instantiation of this Timer.
   */
  inline const timestamp_t & Start() const {
    return timestampStart;
  }

  /**
   * Microseconds elapsed since given timestamp.
   */
  inline static double ElapsedSince(timestamp_t timestamp) {
    if (Timer::Type == TimeMeasure::Counter) {
      TimestampCounterBased now;
      return (static_cast<double>(now.Value() - timestamp) *
        static_cast<double>(TimestampCounterBased::FrequencyPrescale())) /
        static_cast<double>(TimestampCounterBased::FrequencyScaling());
    }
    if (Timer::Type == TimeMeasure::Clock) {
      TimestampClockBased now;
      return (static_cast<double>(now.Value() - timestamp) *
        static_cast<double>(TimestampClockBased::FrequencyPrescale())) /
        static_cast<double>(TimestampClockBased::FrequencyScaling());
    }
    return 0.0f; 
  }

  /**
   * Produces current timestamp.
   */
  inline static timestamp_t Now() {
    if (Timer::Type == TimeMeasure::Counter) {
      TimestampCounterBased timestamp;
      return timestamp.Value();
    }
    if (Timer::Type == TimeMeasure::Clock) {
      TimestampClockBased timestamp;
      return timestamp.Value();
    }
    return 0; 
  }

  /**
   * Convert interval of two timestamp values to mircoseconds.
   */
  inline static double FromInterval(
    const timestamp_t & start,
    const timestamp_t & end) 
  {
    if (Timer::Type == TimeMeasure::Counter) {
      return (static_cast<double>(end - start) *
        static_cast<double>(TimestampCounterBased::FrequencyPrescale())) /
        static_cast<double>(TimestampCounterBased::FrequencyScaling());
    }
    if (Timer::Type == TimeMeasure::Clock) {
      return (static_cast<double>(end - start) *
        static_cast<double>(TimestampClockBased::FrequencyPrescale())) /
        static_cast<double>(TimestampClockBased::FrequencyScaling());
    }
    return -1.0f; 
  }

  /**
   * Convert interval of two timestamp values to mircoseconds.
   */
  inline static double FromInterval(
    const double & start,
    const double & end)
  {
    if (Timer::Type == TimeMeasure::Counter) {
      return ((end - start) *
        TimestampCounterBased::FrequencyPrescale() /
        TimestampCounterBased::FrequencyScaling());
    }
    if (Timer::Type == TimeMeasure::Clock) {
      return ((end - start) *
        TimestampClockBased::FrequencyPrescale() /
        TimestampClockBased::FrequencyScaling());
    }
    return -1.0f; 
  }

  inline static void Calibrate(
      TimeMeasure::MeasureMode mode, 
      unsigned int freq = 0) {
    Timer::Type = mode; 
    if (Timer::Type == TimeMeasure::Counter) {
      Timer::TimestampCounterBased::Calibrate(freq);
    }
    else if (Timer::Type == TimeMeasure::Clock) {
      Timer::TimestampClockBased::Calibrate(freq);
    }
  }

  inline static const char * TimerName() {
    if (Timer::Type == TimeMeasure::Counter) {
      return TimestampCounterBased::TimerName();
    }
    if (Timer::Type == TimeMeasure::Clock) {
      return TimestampClockBased::TimerName();
    }
    return "Undefined";
  }

  inline static Timestamp::counter_t TimestampInfinity() {
    if (Timer::Type == TimeMeasure::Counter) {
      return TimestampCounterBased::TimestampInfinity();
    }
    if (Timer::Type == TimeMeasure::Clock) {
      return TimestampClockBased::TimestampInfinity();
    }
    return LLONG_MAX; 
  }

  inline static Timestamp::counter_t TimestampNegInfinity() {
    if (Timer::Type == TimeMeasure::Counter) {
      return Timer::TimestampCounterBased::TimestampNegInfinity();
    }
    if (Timer::Type == TimeMeasure::Clock) {
      return Timer::TimestampClockBased::TimestampNegInfinity();
    }
    return 0; 
  }

  inline static double FrequencyScaling() {
    if (Timer::Type == TimeMeasure::Counter) {
      return Timer::TimestampCounterBased::FrequencyScaling(); 
    }
    if (Timer::Type == TimeMeasure::Clock) {
      return Timer::TimestampClockBased::FrequencyScaling(); 
    }
    return 1.0f; 
  }
};

} // namespace util
} // namespace dash

#endif // DASH__UTIL__TIMER_H_
