#ifndef DART_INITIALIZATION_H_INCLUDED
#define DART_INITIALIZATION_H_INCLUDED

#include <stdbool.h>
#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_util.h>

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
dart_ret_t dart_init(int *argc, char ***argv) DART_NOTHROW;

/**
 * Initialize the DART runtime with support for thread-based concurrency.
 *
 * \param argc  Pointer to the number of command line arguments.
 * \param argv  Pointer to the array of command line arguments.
 * \param[out] thread_safety The provided thread safety,
 *                           one of \ref dart_thread_support_level_t.
 *
 * \return \c DART_OK on sucess or an error code from \see dart_ret_t otherwise.
 *
 * \threadsafe_none
 * \ingroup DartInitialization
 */
dart_ret_t dart_init_thread(
  int*                          argc,
  char***                       argv,
  dart_thread_support_level_t * thread_safety) DART_NOTHROW;

/**
 * Finalize the DASH runtime.
 *
 * \return \c DART_OK on sucess or an error code from \c dart_ret_t otherwise.
 *
 * \threadsafe_none
 * \ingroup DartInitialization
 */
dart_ret_t dart_exit() DART_NOTHROW;

/**
 * Whether the DASH runtime has been initialized.
 *
 * \return false if DART has not been initialized or has been shut down already,
 *         true  otherwise.
 *
 * \threadsafe
 * \ingroup DartInitialization
 */
bool       dart_initialized() DART_NOTHROW;


/**
 * Abort the application run without performing any cleanup.
 *
 * \c dart_abort tries to call the underlying runtime's abort function
 * (such as \c MPI_Abort) and is guaranteed to not return.
 *
 * \threadsafe_none
 * \ingroup DartInitialization
 */
void       dart_abort(int errorcode) __attribute__((noreturn));

/** \cond DART_HIDDEN_SYMBOLS */
#define DART_INTERFACE_OFF
/** \endcond */

#ifdef __cplusplus
}
#endif

#endif /* DART_INITIALIZATION_H_INCLUDED */
