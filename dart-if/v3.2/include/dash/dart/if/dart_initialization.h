#ifndef DART_INITIALIZATION_H_INCLUDED
#define DART_INITIALIZATION_H_INCLUDED

#include "dart_types.h"

/**
 * \file dart_initialization.h
 * \defgroup  DartInitialization  DART Initialization and Finalization
 * \ingroup   DartInterface
 *
 * Initialization and finalization of the DASH runtime backend.
 *
 * No other DART function may be called before \ref dart_init or after \ref dart_exit.
 *
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

/** \cond DART_HIDDEN_SYMBOLS */
#define DART_INTERFACE_ON
/** \endcond */

/**
 * Initialize the DART runtime
 *
 * \param argc Pointer to the number of command line arguments.
 * \param argv Pointer to the array of command line arguments.
 *
 * \return \c DART_OK on sucess or an error code from \see dart_ret_t otherwise.
 *
 * \threadsafe_none
 * \ingroup DartInitialization
*/
dart_ret_t dart_init(int *argc, char ***argv);

/**
 * Initialize the DART runtime with support for thread-based concurrency.
 *
 * \param argc  Pointer to the number of command line arguments.
 * \param argv  Pointer to the array of command line arguments.
 * \param[out] thread_safety The provided thread safety,
 *                           one of \ref dart_thread_level_t.
 *
 * \return \c DART_OK on sucess or an error code from \see dart_ret_t otherwise.
 *
 * \threadsafe_none
 * \ingroup DartInitialization
 */
dart_ret_t dart_init_thread(
  int*                  argc,
  char***               argv,
  dart_thread_support_level_t * thread_safety);

/**
 * Finalize the DASH runtime.
 *
 * \return \c DART_OK on sucess or an error code from \c dart_ret_t otherwise.
 *
 * \threadsafe_none
 * \ingroup DartInitialization
 */
dart_ret_t dart_exit();

/**
 * Whether the DASH runtime has been initialized.
 *
 * \return 0 if DART has not been initialized or has been shut down already, >0 otherwise.
 *
 * \threadsafe
 * \ingroup DartInitialization
 */
char       dart_initialized();

/** \cond DART_HIDDEN_SYMBOLS */
#define DART_INTERFACE_OFF
/** \endcond */

#ifdef __cplusplus
}
#endif

#endif /* DART_INITIALIZATION_H_INCLUDED */
