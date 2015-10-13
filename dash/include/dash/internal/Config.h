#ifndef DASH__INTERNAL__CONFIG_H_
#define DASH__INTERNAL__CONFIG_H_

// Architecture defines

#if defined(__x86_64__)
#  define DASH__ARCH__ARCH_X86_64
#  define DASH__ARCH__ARCH_X86
#  define DASH__ARCH__HAS_CAS_64
#elif defined(__i386)
#  define DASH__ARCH__ARCH_X86_32
#  define DASH__ARCH__ARCH_X86
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

#endif // DASH__INTERNAL__CONFIG_H_
