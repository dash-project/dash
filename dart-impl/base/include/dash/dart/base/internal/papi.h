/**
 * \file dart/impl/base/internal/papi.h
 *
 * Wrapper for PAPI initialization and error handling.
 */
#ifndef DART__BASE__INTERNAL__PAPI_H__
#define DART__BASE__INTERNAL__PAPI_H__

#ifdef DART_ENABLE_PAPI

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_util.h>
#include <papi.h>

DART_API
void dart__base__locality__papi_handle_error(
  int papi_ret);

DART_API
dart_ret_t dart__base__locality__papi_init(
  const PAPI_hw_info_t ** hwinfo);

#endif /* DART_ENABLE_PAPI */

#endif /* DART__BASE__INTERNAL__PAPI_H__ */
