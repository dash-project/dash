#ifndef DART__IF__CONFIG_H__
#define DART__IF__CONFIG_H__

#include <dash/dart/if/dart_types.h>

#ifdef __cplusplus
extern "C" {
#endif

void dart_config(
  dart_config_t ** config_out);

#define DART_INTERFACE_OFF

#ifdef __cplusplus
} /* extern "C" */
#endif

#define DART_INTERFACE_ON

#endif /* DART__IF__CONFIG_H__ */
