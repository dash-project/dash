#ifndef DART__IF__CONFIG_H__
#define DART__IF__CONFIG_H__

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_util.h>

/**
 * \file dart_config.h
 *
 * \defgroup  DartConfig  DART runtime configuration interface
 * \ingroup   DartInterface
 *
 * Interface to access the DART runtime configuration.
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Access the DART runtime configuration descriptor.
 *
 * \ingroup DartConfig
 */
void dart_config(
  dart_config_t ** config_out) DART_NOTHROW;

#define DART_INTERFACE_OFF

#ifdef __cplusplus
} /* extern "C" */
#endif

#define DART_INTERFACE_ON

#endif /* DART__IF__CONFIG_H__ */
