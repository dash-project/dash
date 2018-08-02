#ifndef DASH__UTIL__INTERNAL__TIMESTAMP_COUNTER_POSIX_H_
#define DASH__UTIL__INTERNAL__TIMESTAMP_COUNTER_POSIX_H_

#include <dash/util/Timer.h>
#include <dash/util/Timestamp.h>

#include <stdint.h>
#include <iostream>

#include <dash/internal/Config.h>
#if ((defined(DASH__ARCH__ARCH_X86_64) || \
      defined(DASH__ARCH__ARCH_X86_32)) && \
     !defined(DASH__ARCH__HAS_RDTSC)) \
    || \
    (!defined(DASH__ARCH__ARCH_X86_64) && \
     !defined(DASH__ARCH__ARCH_X86_32) && \
     !defined(DASH__ARCH__ARCH_ARMV6))
#include <sys/time.h>
#endif // ARM <= V3 or MIPS

namespace dash {
namespace util {
namespace internal {

/**
 * Timestamp counter (RDTSC or PMC) for POSIX platforms.
 */
class TimestampCounterPosix : public Timestamp
{
private:
  Timestamp::counter_t value;

public:
  static Timestamp::counter_t frequencyScaling;

  /**
   * Serialized RDTSCP (x86, x64) or PMC,PMU (arm6+)
   *
   * Prevents out-of-order execution to affect timestamps.
   *
   */
  static inline uint64_t ArchCycleCount() {
#if defined(DASH__ARCH__HAS_RDTSC) && defined(DASH__ARCH__ARCH_X86_64)
    uint64_t rax, rdx;
    uint32_t tsc_aux;
    __asm__ volatile ("rdtscp\n" : "=a" (rax), "=d" (rdx), "=c" (tsc_aux) : : );
    return (rdx << 32) + rax;
#elif defined(DASH__ARCH__HAS_RDTSC) && defined(DASH__ARCH__ARCH_X86_32)
    int64_t ret;
    __asm__ volatile ("rdtsc" : "=A" (ret) );
    return ret;
#elif defined(DASH__ARCH__ARCH_ARMV6)
    uint32_t pmccntr;
    uint32_t pmuseren = 1;
    uint32_t pmcntenset;

    // Read the user mode perf monitor counter access permissions.
    asm volatile ("mrc p15, 0, %0, c9, c14, 0" : "=r" (pmuseren));
    // Set permission flag:
    // pmuseren &= 0x01;  // Set E bit
    // asm volatile ("mcr p15, 0, %0, c9, c14, 0" : "=r" (pmuseren));
    if (pmuseren & 1) {  // Allows reading perfmon counters for user mode code.
      asm volatile ("mrc p15, 0, %0, c9, c12, 1" : "=r" (pmcntenset));
      if (pmcntenset & 0x80000000ul) {  // Is it counting?
        asm volatile ("mrc p15, 0, %0, c9, c13, 0" : "=r" (pmccntr));
        // The counter is set up to count every 64th cycle
        return static_cast<int64_t>(pmccntr) * 64;  // Should optimize to << 6
      }
      else {
        return 1;
      }
    }
    else {
      return 2;
    }
#else // Fallback for architectures that do not provide any low-level counter:
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return static_cast<uint64_t>(
        (tv.tv_sec + tv.tv_usec * 0.000001) *
        FrequencyScaling());
#endif
    // Undefined value if perfmon is unavailable:
    return 0;
  }

public:

  /**
   * Calibrates counts per microscecond.
   *
   * Used for frequency scaling of RDSTD.
   */
  static void Calibrate(unsigned int = 0);

public:

  inline TimestampCounterPosix()
  : value(static_cast<counter_t>(ArchCycleCount()))
  { }

  inline TimestampCounterPosix(
    const TimestampCounterPosix & other)
  : value(other.value)
  { }

  inline TimestampCounterPosix(
    const counter_t & counterValue)
  : value(counterValue)
  { }

  inline TimestampCounterPosix & operator=(
    const TimestampCounterPosix rhs)
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
    return static_cast<double>(TimestampCounterPosix::frequencyScaling);
  }

  inline static double FrequencyPrescale()
  {
    return 1.0f;
  }

  inline static const char * TimerName()
  {
#if defined(DASH__ARCH__ARCH_X86_64)
    return "POSIX:X64:RDTSC";
#elif defined(DASH__ARCH__ARCH_X86_32)
    return "POSIX:386:RDTSC";
#elif defined(DASH__ARCH__ARCH_ARMV6)
    return "POSIX:ARM:PMCNT";
#else
    return "POSIX:GENERIC";
#endif
  }

};

} // namespace internal
} // namespace util
} // namespace dash

#endif // DASH__UTIL__INTERNAL__TIMESTAMP_COUNTER_POSIX_H_
