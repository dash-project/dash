#ifndef DART_INITIALIZATION_H_INCLUDED
#define DART_INITIALIZATION_H_INCLUDED

#include "dart_types.h"

/**
 * \file dart_init.h
 *
 * Initializationk and finalization of the DASH runtime backend.
 */

/**
 * \defgroup  DartInitialization  DART Initialization and Finalization
 * \ingroup   DartInterface
 */
#ifdef __cplusplus
extern "C" {
#endif

#define DART_INTERFACE_ON

/**
 * Initialize the DART runtime
 *
 * No other DART function may be called before dart_init() or after
 * dart_exit().
 *
 * \ingroup DartInitialization
*/
dart_ret_t dart_init(int *argc, char ***argv);

/**
 * Finalize the DASH runtime.
 *
 * \ingroup DartInitialization
 */
dart_ret_t dart_exit();

/**
 * Whether the DASH runtime has been initialized.
 *
 * \ingroup DartInitialization
 */
char       dart_initialized();

#define DART_INTERFACE_OFF

#ifdef __cplusplus
}
#endif

#endif /* DART_INITIALIZATION_H_INCLUDED */
