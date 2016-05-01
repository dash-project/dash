/**
 * \file dart/impl/base/internal/papi.h
 *
 * Wrapper for PAPI initialization and error handling.
 */
#ifndef DART__BASE__INTERNAL__PAPI_H__
#define DART__BASE__INTERNAL__PAPI_H__

#ifdef DART_ENABLE_PAPI
#include <papi.h>

static void dart__base__locality__papi_handle_error(
  int papi_ret)

static dart_ret_t dart__base__locality__papi_init(
  const PAPI_hw_info_t ** hwinfo)

#endif /* DART_ENABLE_PAPI */

#endif /* DART__BASE__INTERNAL__PAPI_H__ */
