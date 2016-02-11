#ifndef DART_INITIALIZATION_H_INCLUDED
#define DART_INITIALIZATION_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "dart_types.h"

#define DART_INTERFACE_ON

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
 * Whether the DASH runtime has been initialized.
 */
char       dart_initialized();

#define DART_INTERFACE_OFF

#ifdef __cplusplus
}
#endif

#endif /* DART_INITIALIZATION_H_INCLUDED */
