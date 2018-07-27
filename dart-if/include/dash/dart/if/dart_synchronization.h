#ifndef DART_SYNCHRONIZATION_H_INCLUDED
#define DART_SYNCHRONIZATION_H_INCLUDED

/**
 * \file dart_synchronization.h
 * \defgroup  DartSync    Synchronization primitives for mutual exclusion of units.
 * \ingroup   DartInterface
 *
 * Synchronization primitives for mutual exclusion of units.
 *
 */

#include <dash/dart/if/dart_util.h>
#include <dash/dart/if/dart_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** \cond DART_HIDDEN_SYMBOLS */
#define DART_INTERFACE_ON
/** \endcond */

/**
 * Lock type to ensure mutual exclusion among units in a team.
 * The lock is thread-aware so only one thread of a unit can acquire
 * the lock at once.
 * \ingroup DartSync
 */
typedef struct dart_lock_struct *dart_lock_t;


/**
 * Collective operation to initialize the \c lock object.
 *
 * \param teamid Team this lock is used for.
 * \param lock   The lock to initialize.
 *
 * \return \c DART_OK on sucess or an error code from \ref dart_ret_t otherwise.
 *
 * \threadsafe_none
 * \ingroup DartSync
 */
dart_ret_t dart_team_lock_init(
  dart_team_t   teamid,
  dart_lock_t * lock)   DART_NOTHROW;

/**
 * Collective operation to destroy a \c lock initialized using
 * \ref dart_team_lock_init.
 *
 * \param lock   The \c lock to free.
 * \return \c DART_OK on sucess or an error code from \ref dart_ret_t otherwise.
 *
 * \threadsafe_none
 * \ingroup DartSync
 */
dart_ret_t dart_team_lock_destroy(
  dart_lock_t * lock)   DART_NOTHROW;

/**
 * Block until the \c lock was acquired.
 *
 * The lock can be held by any thread in any unit of the team.
 *
 * Note that the lock is not recursive, trying to acquire the lock twice
 * in the same thread is erroneous.
 *
 * \param lock The lock to acquire
 * \return \c DART_OK on sucess or an error code from \ref dart_ret_t otherwise.
 *
 * \threadsafe
 * \ingroup DartSync
 */
dart_ret_t dart_lock_acquire(
  dart_lock_t   lock)   DART_NOTHROW;

/**
 * Try to acquire the lock and return immediately.
 *
 * Note that the lock is not recursive, trying to acquire the lock twice
 * in the same thread is erroneous.
 *
 * \param lock The lock to acquire
 * \param[out] result \c True if the lock was successfully acquired,
 *             false otherwise.
 *
 * \return \c DART_OK on success or an error code from \ref dart_ret_t
 *         otherwise.
 *
 * \threadsafe
 * \ingroup DartSync
 */
dart_ret_t dart_lock_try_acquire(
  dart_lock_t   lock,
  int32_t     * result) DART_NOTHROW;

/**
 * Release the lock acquired through \ref dart_lock_acquire or
 * \ref dart_lock_try_acquire.
 *
 * \param lock The lock to release.
 * \return \c DART_OK on sucess or an error code from \ref dart_ret_t otherwise.
 *
 * \threadsafe
 * \ingroup DartSync
 */
dart_ret_t dart_lock_release(
  dart_lock_t   lock)   DART_NOTHROW;

/**
 * Whether the lock has been properly initialized.
 *
 * \return true if the DART lock is properly initialized
 *         false  otherwise.
 *
 * \threadsafe_none
 * \ingroup DartInitialization
 */
dart_ret_t dart_lock_initialized(
    struct dart_lock_struct const *lock) DART_NOTHROW;

/** \cond DART_HIDDEN_SYMBOLS */
#define DART_INTERFACE_OFF
/** \endcond */

#ifdef __cplusplus
}
#endif

#endif /* DART_SYNCHRONIZATION_H_INCLUDED */
