/**
 * \file dart/impl/base/internal/papi.c
 *
 * Wrapper for PAPI initialization and error handling.
 */

#include <dash/dart/if/dart_types.h>

#ifdef DART_ENABLE_PAPI
#include <dash/dart/base/internal/papi.h>
#include <dash/dart/base/logging.h>

#include <papi.h>
#include <errno.h>

#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>


void dart__base__locality__papi_handle_error(
  int papi_ret)
{
  /*
   * PAPI_EINVAL   papi.h is different from the version used to compile the
   *               PAPI library.
   * PAPI_ENOMEM   Insufficient memory to complete the operation.
   * PAPI_ECMP     This component does not support the underlying hardware.
   * PAPI_ESYS     A system or C library call failed inside PAPI, see the
   *               errno variable.
   */
  switch (papi_ret) {
    case PAPI_EINVAL:
         DART_LOG_ERROR("dart__base__locality: PAPI_EINVAL: "
                        "papi.h is different from the version used to "
                        "compile the PAPI library.");
         break;
    case PAPI_ENOMEM:
         DART_LOG_ERROR("dart__base__locality: PAPI_ENOMEM: "
                        "insufficient memory to complete the operation.");
         break;
    case PAPI_ECMP:
         DART_LOG_ERROR("dart__base__locality: PAPI_ENOMEM: "
                        "this component does not support the underlying "
                        "hardware.");
         break;
    case PAPI_ESYS:
         DART_LOG_ERROR("dart_domain_locality: PAPI_ESYS: "
                        "a system or C library call failed inside PAPI, see "
                        "the errno variable");
         DART_LOG_ERROR("dart__base__locality: PAPI_ESYS: errno: %d", errno);
         break;
    default:
         DART_LOG_ERROR("dart__base__locality: PAPI: unknown error: %d",
                        papi_ret);
         break;
  }
}

dart_ret_t dart__base__locality__papi_init(
  const PAPI_hw_info_t ** hwinfo)
{
  int papi_ret;

  if (PAPI_is_initialized()) {
    *hwinfo = PAPI_get_hardware_info();
    return DART_OK;
  }
  DART_LOG_DEBUG("dart__base__locality: PAPI: init");

  papi_ret = PAPI_library_init(PAPI_VER_CURRENT);

  if (papi_ret != PAPI_VER_CURRENT && papi_ret > 0) {
    DART_LOG_ERROR("dart__base__locality: PAPI: version mismatch");
    return DART_ERR_OTHER;
  } else if (papi_ret < 0) {
    DART_LOG_ERROR("dart__base__locality: PAPI: init failed, returned %d",
                   papi_ret);
    dart__base__locality__papi_handle_error(papi_ret);
    return DART_ERR_OTHER;
  } else {
    papi_ret = PAPI_is_initialized();
    if (papi_ret != PAPI_LOW_LEVEL_INITED) {
      dart__base__locality__papi_handle_error(papi_ret);
      return DART_ERR_OTHER;
    }
  }

#if 0
  papi_ret = PAPI_thread_init(pthread_self);

  if (papi_ret != PAPI_OK) {
    DART_LOG_ERROR("dart__base__locality: PAPI: PAPI_thread_init failed");
    return DART_ERR_OTHER;
  }
#endif
  *hwinfo = PAPI_get_hardware_info();
  if (*hwinfo == NULL) {
    DART_LOG_ERROR("dart__base__locality: PAPI: get hardware info failed");
    return DART_ERR_OTHER;
  }

  DART_LOG_DEBUG("dart__base__locality: PAPI: initialized");
  return DART_OK;
}

#endif /* DART_ENABLE_PAPI */
