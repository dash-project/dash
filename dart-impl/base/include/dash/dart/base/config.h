#ifndef DART__BASE__CONFIG_H_
#define DART__BASE__CONFIG_H_

/**
 * Input for configuration file generated during build.
 * Provides platform-specific definitions.
 */

#ifdef DOXYGEN

/**
 * \defgroup{Config}
 *
 * \ingroup{Config}
 *
 * \par{Architecture-specific Definitions}
 *
 * Definition                                | Defined for                                |
 * ----------------------------------------- | ------------------------------------------ |
 * <tt>DART__ARCH__ARCH_32</tt>              | Any 32-bit architecture.                   |
 * <tt>DART__ARCH__ARCH_64</tt>              | Any 64-bit architecture.                   |
 * <tt>DART__ARCH__ARCH_X86_32</tt>          | Intel x86 compatible 32-bit architecture.  |
 * <tt>DART__ARCH__ARCH_X86_64</tt>          | Intel x86 compatible 64-bit architecture.  |
 * <tt>DART__ARCH__ARCH_ARM</tt>             | Any ARM architecture.                      |
 * <tt>DART__ARCH__ARCH_ARMV<i>X</i></tt>    | ARM architecture version <i>X</i>.         |
 * <tt>DART__ARCH__ARCH_UNKNOWN<i>X</i></tt> | Unknown architecture.                      |
 * &nbsp;                                    | e.g. <tt>DART__ARCH__ARMV7</tt> for ARMv7. |
 * <tt>DART__ARCH__CACHE_LINE_SIZE</tt>      | Width of a single cache line, in bytes.    |
 * <tt>DART__ARCH__PAGE_SIZE</tt>            | Width of a single memory page, in bytes.   |
 * <tt>DART__ARCH__HAS_CAS</tt>              | Atomic Compare-And-Swap supported.         |
 * <tt>DART__ARCH__HAS_CAS_64</tt>           | CAS on 64-bit wide values supported.       |
 * <tt>DART__ARCH__HAS_CAS_32</tt>           | CAS on 32-bit wide values supported.       |
 * <tt>DART__ARCH__HAS_LLSC</tt>             | Load-Linked/Store-Conditional supported.   |
 * <tt>DART__ARCH__HAS_LLSC_32</tt>          | LL/SC on 32-bit wide values supported.     |
 * <tt>DART__ARCH__HAS_LLSC_64</tt>          | LL/SC on 64-bit wide values supported.     |
 *
 * \par{OS-specific Definitions}
 *
 * Definition                             | Defined for                                |
 * -------------------------------------- | ------------------------------------------ |
 * <tt>DART__PLATFORM__POSIX</tt>         | POSIX-compatible platform.                 |
 * <tt>DART__PLATFORM__LINUX</tt>         | Linux platform.                            |
 * <tt>DART__PLATFORM__FREEBSD</tt>       | FreeBSD platform.                          |
 * <tt>DART__PLATFORM__OSX</tt>           | Apple OSX platform.                        |
 * <tt>DART__PLATFORM__UX</tt>            | HP-UX/Sun platform.                        |
 *
 */

#else // !DOXYGEN

// Architecture defines

#if defined(__x86_64__)
#  define DART__ARCH__ARCH_X86_64
#  define DART__ARCH__ARCH_X86
#  define DART__ARCH__ARCH_64
#  define DART__ARCH__HAS_CAS_64
#elif defined(__i386)
#  define DART__ARCH__ARCH_X86_32
#  define DART__ARCH__ARCH_X86
#  define DART__ARCH__ARCH_32
#  define DART__ARCH__HAS_CAS_32
#elif defined(__arm__)
#  define DART__ARCH__ARCH_ARM
// ARM versions consolidated to major architecture version.
// See: https://wiki.edubuntu.org/ARM/Thumb2PortingHowto
#  if defined(__ARM_ARCH_7__) || \
      defined(__ARM_ARCH_7R__) || \
      defined(__ARM_ARCH_7A__)
#      define DART__ARCH__ARCH_ARMV7 1
#  endif
#  if defined(DART__ARCH__ARCH_ARMV7) || \
      defined(__ARM_ARCH_6__) || \
      defined(__ARM_ARCH_6J__) || \
      defined(__ARM_ARCH_6K__) || \
      defined(__ARM_ARCH_6Z__) || \
      defined(__ARM_ARCH_6T2__) || \
      defined(__ARM_ARCH_6ZK__)
#      define DART__ARCH__ARCH_ARMV6 1
#  endif
#  if defined(DART__ARCH__ARCH_ARMV6) || \
      defined(__ARM_ARCH_5T__) || \
      defined(__ARM_ARCH_5E__) || \
      defined(__ARM_ARCH_5TE__) || \
      defined(__ARM_ARCH_5TEJ__)
#      define DART__ARCH__ARCH_ARMV5 1
#  endif
#  if defined(DART__ARCH__ARCH_ARMV5) || \
      defined(__ARM_ARCH_4__) || \
      defined(__ARM_ARCH_4T__)
#      define DART__ARCH__ARCH_ARMV4 1
#  endif
#  if defined(DART__ARCH__ARCH_ARMV4) || \
      defined(__ARM_ARCH_3__) || \
      defined(__ARM_ARCH_3M__)
#      define DART__ARCH__ARCH_ARMV3 1
#  endif
#  if defined(DART__ARCH__ARCH_ARMV3) || \
      defined(__ARM_ARCH_2__)
#    define DART__ARCH__ARCH_ARMV2 1
#    define DART__ARCH__ARCH_ARM 1
#  endif

#else
#  define DART__ARCH__ARCH_UNKNOWN
#endif

// Intel(R) Many Integrated Core (MIC, Xeon Phi)
#if defined(__MIC__)
#  define DART__ARCH__IS_MIC
#endif


// RDTSC support:
//
#if defined(DART__ARCH__ARCH_X86_64) && \
    !defined(DART__ARCH__IS_MIC)
#  define DART__ARCH__HAS_RDTSC
#endif


// Atomic instructions:
//
// LL/SC:
#if defined(__ARM_ARCH_7A__)
#define DART__ARCH__HAS_LLSC
#define DART__ARCH__HAS_LLSC_64
#endif
// CAS:
#if defined(DART__ARCH__HAS_CAS_64) || \
    defined(DART__ARCH__HAS_CAS_32)
#  define DART__ARCH__HAS_CAS
#endif
#if defined(DART__ARCH__HAS_LLSC_64) || \
    defined(DART__ARCH__HAS_LLSC_32)
#  define DART__ARCH__HAS_LLSC
#endif

#if defined(DART__ARCH__ARCH_ARM)
// Assuming 32-bit architecture for ARM:
#  define DART__ARCH__ARCH_32
#endif

// Default cache line and page size, in bytes
#if defined(DART__ARCH__ARCH_64)
#  define DART__ARCH__CACHE_LINE_SIZE 64
#  define DART__ARCH__PAGE_SIZE 0x1000
#else
#  define DART__ARCH__CACHE_LINE_SIZE 32
#  define DART__ARCH__PAGE_SIZE 0x1000
#endif

// Platform defines

// OSX
#if defined(__MACH__) && defined(__APPLE__)
#  define DART__PLATFORM__OSX
#endif
// UX
#if (defined(__hpux) || defined(hpux)) || \
     ((defined(__sun__) || defined(__sun) || defined(sun)) && \
      (defined(__SVR4) || defined(__svr4__)))
#  define DART__PLATFORM__UX
#endif
// Linux
#if defined(__linux__)
#  define DART__PLATFORM__LINUX
#  define DART__PLATFORM__POSIX
#endif
// FreeBSD
#if defined(__FreeBSD__)
#  define DART__PLATFORM__FREEBSD
#  define DART__PLATFORM__POSIX
#endif

#endif // DOXYGEN


#endif /* DART__BASE__CONFIG_H_ */
