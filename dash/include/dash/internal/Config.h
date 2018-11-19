#ifndef DASH__INTERNAL__CONFIG_H_
#define DASH__INTERNAL__CONFIG_H_

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
 * <tt>DASH__ARCH__ARCH_32</tt>              | Any 32-bit architecture.                   |
 * <tt>DASH__ARCH__ARCH_64</tt>              | Any 64-bit architecture.                   |
 * <tt>DASH__ARCH__ARCH_X86_32</tt>          | Intel x86 compatible 32-bit architecture.  |
 * <tt>DASH__ARCH__ARCH_X86_64</tt>          | Intel x86 compatible 64-bit architecture.  |
 * <tt>DASH__ARCH__ARCH_ARM</tt>             | Any ARM architecture.                      |
 * <tt>DASH__ARCH__ARCH_ARMV<i>X</i></tt>    | ARM architecture version <i>X</i>.         |
 * <tt>DASH__ARCH__ARCH_UNKNOWN<i>X</i></tt> | Unknown architecture.                      |
 * &nbsp;                                    | e.g. <tt>DASH__ARCH__ARMV7</tt> for ARMv7. |
 * <tt>DASH__ARCH__CACHE_LINE_SIZE</tt>      | Width of a single cache line, in bytes.    |
 * <tt>DASH__ARCH__PAGE_SIZE</tt>            | Width of a single memory page, in bytes.   |
 * <tt>DASH__ARCH__HAS_CAS</tt>              | Atomic Compare-And-Swap supported.         |
 * <tt>DASH__ARCH__HAS_CAS_64</tt>           | CAS on 64-bit wide values supported.       |
 * <tt>DASH__ARCH__HAS_CAS_32</tt>           | CAS on 32-bit wide values supported.       |
 * <tt>DASH__ARCH__HAS_LLSC</tt>             | Load-Linked/Store-Conditional supported.   |
 * <tt>DASH__ARCH__HAS_LLSC_32</tt>          | LL/SC on 32-bit wide values supported.     |
 * <tt>DASH__ARCH__HAS_LLSC_64</tt>          | LL/SC on 64-bit wide values supported.     |
 *
 * \par{OS-specific Definitions}
 *
 * Definition                             | Defined for                                |
 * -------------------------------------- | ------------------------------------------ |
 * <tt>DASH__PLATFORM__POSIX</tt>         | POSIX-compatible platform.                 |
 * <tt>DASH__PLATFORM__LINUX</tt>         | Linux platform.                            |
 * <tt>DASH__PLATFORM__FREEBSD</tt>       | FreeBSD platform.                          |
 * <tt>DASH__PLATFORM__OSX</tt>           | Apple OSX platform.                        |
 * <tt>DASH__PLATFORM__UX</tt>            | HP-UX/Sun platform.                        |
 *
 */

#else // !DOXYGEN

#include <dash/internal/Macro.h>

// Architecture defines

#if defined(__x86_64__)
#  define DASH__ARCH__ARCH_X86_64
#  define DASH__ARCH__ARCH_X86
#  define DASH__ARCH__ARCH_64
#  define DASH__ARCH__HAS_CAS_64
#elif defined(__i386)
#  define DASH__ARCH__ARCH_X86_32
#  define DASH__ARCH__ARCH_X86
#  define DASH__ARCH__ARCH_32
#  define DASH__ARCH__HAS_CAS_32
#elif defined(__arm__)
#  define DASH__ARCH__ARCH_ARM
// ARM versions consolidated to major architecture version.
// See: https://wiki.edubuntu.org/ARM/Thumb2PortingHowto
#  if defined(__ARM_ARCH_7__) || \
      defined(__ARM_ARCH_7R__) || \
      defined(__ARM_ARCH_7A__)
#      define DASH__ARCH__ARCH_ARMV7 1
#  endif
#  if defined(DASH__ARCH__ARCH_ARMV7) || \
      defined(__ARM_ARCH_6__) || \
      defined(__ARM_ARCH_6J__) || \
      defined(__ARM_ARCH_6K__) || \
      defined(__ARM_ARCH_6Z__) || \
      defined(__ARM_ARCH_6T2__) || \
      defined(__ARM_ARCH_6ZK__)
#      define DASH__ARCH__ARCH_ARMV6 1
#  endif
#  if defined(DASH__ARCH__ARCH_ARMV6) || \
      defined(__ARM_ARCH_5T__) || \
      defined(__ARM_ARCH_5E__) || \
      defined(__ARM_ARCH_5TE__) || \
      defined(__ARM_ARCH_5TEJ__)
#      define DASH__ARCH__ARCH_ARMV5 1
#  endif
#  if defined(DASH__ARCH__ARCH_ARMV5) || \
      defined(__ARM_ARCH_4__) || \
      defined(__ARM_ARCH_4T__)
#      define DASH__ARCH__ARCH_ARMV4 1
#  endif
#  if defined(DASH__ARCH__ARCH_ARMV4) || \
      defined(__ARM_ARCH_3__) || \
      defined(__ARM_ARCH_3M__)
#      define DASH__ARCH__ARCH_ARMV3 1
#  endif
#  if defined(DASH__ARCH__ARCH_ARMV3) || \
      defined(__ARM_ARCH_2__)
#    define DASH__ARCH__ARCH_ARMV2 1
#    define DASH__ARCH__ARCH_ARM 1
#  endif

#else
#  define DASH__ARCH__ARCH_UNKNOWN
#endif

// Intel(R) Many Integrated Core (MIC, Xeon Phi)
#if defined(__MIC__)
#  define DASH__ARCH__IS_MIC
#endif


// RDTSC support:
//
#if defined(DASH__ARCH__ARCH_X86_64) && \
    !defined(DASH__ARCH__IS_MIC)
#  define DASH__ARCH__HAS_RDTSC
#endif


// Atomic instructions:
//
// LL/SC:
#if defined(__ARM_ARCH_7A__)
#define DASH__ARCH__HAS_LLSC
#define DASH__ARCH__HAS_LLSC_64
#endif
// CAS:
#if defined(DASH__ARCH__HAS_CAS_64) || \
    defined(DASH__ARCH__HAS_CAS_32)
#  define DASH__ARCH__HAS_CAS
#endif
#if defined(DASH__ARCH__HAS_LLSC_64) || \
    defined(DASH__ARCH__HAS_LLSC_32)
#  define DASH__ARCH__HAS_LLSC
#endif

#if defined(DASH__ARCH__ARCH_ARM)
// Assuming 32-bit architecture for ARM:
#  define DASH__ARCH__ARCH_32
#endif

// Default cache line and page size, in bytes
#if defined(DASH__ARCH__ARCH_64)
#  define DASH__ARCH__CACHE_LINE_SIZE 64
#  define DASH__ARCH__PAGE_SIZE 0x1000
#else
#  define DASH__ARCH__CACHE_LINE_SIZE 32
#  define DASH__ARCH__PAGE_SIZE 0x1000
#endif

// Platform defines

// OSX
#if defined(__MACH__) && defined(__APPLE__)
#  define DASH__PLATFORM__OSX
#endif
// UX
#if (defined(__hpux) || defined(hpux)) || \
     ((defined(__sun__) || defined(__sun) || defined(sun)) && \
      (defined(__SVR4) || defined(__svr4__)))
#  define DASH__PLATFORM__UX
#endif
// Linux
#if defined(__linux__)
#  define DASH__PLATFORM__LINUX
#  define DASH__PLATFORM__POSIX
#endif
// FreeBSD
#if defined(__FreeBSD__)
#  define DASH__PLATFORM__FREEBSD
#  define DASH__PLATFORM__POSIX
#endif

#ifdef _OPENMP
#  define DASH_ENABLE_OPENMP
#  if _OPENMP >= 201307
#    define DASH__OPENMP_VERSION 40
#  else
#    define DASH__OPENMP_VERSION 30
#  endif
#endif

// Compiler ID
#if defined (__GNUC_MINOR__)
// GCC
#define DASH_COMPILER_ID            \
  "GCC-"                            \
  dash__toxstr(__GNUC__) "."        \
  dash__toxstr(__GNUC_MINOR__) "."  \
  dash__toxstr(__GNUC_PATCHLEVEL__)
#elif defined (__INTEL_COMPILER)
#define DASH_COMPILER_ID            \
  "Intel-"                          \
  dash__toxstr(__INTEL_COMPILER)    \
#elif defined (__clang__)
#define DASH_COMPILER_ID            \
  "Clang-"                          \
  dash__toxstr(__clang_major__) "." \
  dash__toxstr(__clang_minor__) "." \
  dash__toxstr(__clang_patchlevel__)
#elif defined (_CRAYC)
#define DASH_COMPILER_ID            \
  "Cray-"                           \
  dash__toxstr(_RELEASE) "."        \
  dash__toxstr(_RELEASE_MINOR)
#elif defined (__IBMC__)
#define DASH_COMPILER_ID
  "IBM-"                            \
  dash__toxstr(__IBMC__)
#else
#define COMPILER_ID                 \
  "UNKNOWN"
#endif


#endif // DOXYGEN

#endif // DASH__INTERNAL__CONFIG_H_
