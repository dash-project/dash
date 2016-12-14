#ifndef DART__INITIALIZATION_H
#define DART__INITIALIZATION_H

/* DART v4.0 */

#ifdef __cplusplus
extern "C" {
#endif

#include "dart_types.h"

#define DART_INTERFACE_ON

/**
 * \file dart_initialization.h
 *
 * ### DART runtime initialization and shutdown ###
 *
 */

/**
 * Initialize the DART runtime
 *
 * No other DART function may be called before dart_init() or after
 * dart_exit().
*/
dart_ret_t dart_init(int *argc, char ***argv);

/**
 * Finalize the DASH runtime.
 */
dart_ret_t dart_exit();

/**
 * Determine whether the DASH runtime has been initialized.
 */
dart_ret_t dart_initialized(int32_t *result);


#define DART_INTERFACE_OFF

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DART__INITIALIZATION_H */

