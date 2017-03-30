#ifndef DART_HANDLE_H_
#define DART_HANDLE_H_

#ifdef __cplusplus
extern "C" {
#endif

/** \cond DART_HIDDEN_SYMBOLS */
#define DART_INTERFACE_ON
/** \endcond */

/*
 * \file dart_handle.h
 *
 *  Functions for handling of asynchronous handles.
 */

#include <dash/dart/if/dart_types.h>

/**
 * Handle returned by asynchronous operations to wait for a specific
 * operation to complete using \c dart_wait etc.
 */
typedef struct dart_handle_struct * dart_handle_t;

dart_ret_t 
dart_test(
  dart_handle_t   handle,
  int32_t       * result);

dart_ret_t
dart_wait(
  dart_handle_t   handle,
  int32_t       * result);


/** \cond DART_HIDDEN_SYMBOLS */
#define DART_INTERFACE_OFF
/** \endcond */

#ifdef __cplusplus
}
#endif


#endif /* DART_HANDLE_H_ */
