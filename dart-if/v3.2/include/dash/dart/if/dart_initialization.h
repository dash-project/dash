#ifndef DART_INITIALIZATION_H_INCLUDED
#define DART_INITIALIZATION_H_INCLUDED

#include "dart_types.h"

/**
 * \file dart_init.h
 *
 * \brief Initialization and finalization of the DASH runtime backend. No other DART function may be called before \see dart_init() or after
 * \see dart_exit().
 *
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
 * \brief Initialize the DART runtime
 *
 * \return \c DART_OK on sucess or an error code from \see dart_ret_t otherwise.
 *
 * \ingroup DartInitialization
*/
dart_ret_t dart_init(int *argc, char ***argv);

/**
 * \brief Finalize the DASH runtime.
 *
 * \return \c DART_OK on sucess or an error code from \c dart_ret_t otherwise.
 *
 * \ingroup DartInitialization
 */
dart_ret_t dart_exit();

/**
 * \brief Whether the DASH runtime has been initialized.
 *
 * \return 0 if DART has not been initialized or has been shut down already, >0 otherwise.
 *
 * \ingroup DartInitialization
 */
char       dart_initialized();

#define DART_INTERFACE_OFF

#ifdef __cplusplus
}
#endif

#endif /* DART_INITIALIZATION_H_INCLUDED */
