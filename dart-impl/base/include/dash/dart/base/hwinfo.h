/**
 * \file dash/dart/base/hwinfo.h
 */
#ifndef DART__BASE__HWINFO_H__
#define DART__BASE__HWINFO_H__

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_util.h>

/**
 * Initializes hwinfo object with uninitialized defaults.
 */
DART_API
dart_ret_t dart_hwinfo_init(
  dart_hwinfo_t * hwinfo);

/**
 * Resolves the current unit's hardware locality information.
 *
 * Locality information is obtained from a series of specializes libraries
 * such as PAPI, hwloc, likwid, libnuma and platform-dependent system
 * functions, depending on libraries enabled in the DART build configuration.
 *
 * If a locality property cannot be reliably resolved or deduced, the
 * respective entry is set to \c -1 or an empty string.
 */
DART_API
dart_ret_t dart_hwinfo(
  dart_hwinfo_t * hwinfo);

#endif /* DART__BASE__HWINFO_H__ */
