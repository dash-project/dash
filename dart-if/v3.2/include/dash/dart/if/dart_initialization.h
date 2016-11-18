#ifndef DART_INITIALIZATION_H_INCLUDED
#define DART_INITIALIZATION_H_INCLUDED

#include "dart_types.h"

/**
 * \file dart_initialization.h
 * \defgroup  DartInitialization  DART Initialization and Finalization
 * \ingroup   DartInterface
 *
 * \brief Initialization and finalization of the DASH runtime backend.
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
 * \brief Initialize the DART runtime
 *
 * \param argc Pointer to the number of command line arguments.
 * \param argv Pointer to the array of command line arguments.
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

/** \cond DART_HIDDEN_SYMBOLS */
#define DART_INTERFACE_OFF
/** \endcond */

#ifdef __cplusplus
}
#endif

#endif /* DART_INITIALIZATION_H_INCLUDED */
