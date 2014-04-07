/** @file dart_adapt_synchronization.h
 *  @date 25 Mar 2014
 *  @brief Function prototypes for all the synchronization operations.
 *
 *  the implementation based on MPI-3 standard and MCS algorithm.
 *  @see http://www.mcs.anl.gov/uploads/cels/papers/P4014-0113.pdf 
 *  @see http://www.cise.ufl.edu/tr/DOC/REP-1992-71.pdf
 */
#ifndef DART_ADAPT_SYNCHRONIZATION_H_INCLUDED
#define DART_ADAPT_SYNCHRONIZATION_H_INCLUDED

#include "dart_deb_log.h"
#include "dart_adapt_synchronization_priv.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DART_INTERFACE_ON

#define DART_LOCK_TAIL_DISP 0

/** @brief Create an lock. Collective on teamid.
 *   
 *  @param[in] 	teamid  Team containing all the processes that will use the lock.
 *  @param[out]	lock	Pointer to the lock.
 *  @return		Dart status.
 */

dart_ret_t dart_adapt_team_lock_init(dart_team_t teamid, 
			       dart_lock_t* lock);

/** TODO: Deleting the redundant parameter teamid, no use here. 
 */

/** @brief Free the lock.  Collective on units in the team used at the moment of creation.
 *  @param[in] teamid	Team containing all processes that will free the lock.
 *  @param[in] lock	Handle to the lock that going to be freed.
 *  @return 		Dart status.
 */
dart_ret_t dart_adapt_team_lock_free(dart_team_t teamid,
			       dart_lock_t* lock);

/* -- Blocking calls -- */

/** @brief Acquire a lock.
 * 
 *  @param[in] lock	Handle to the lock.
 *  @return		Dart status.
 */
dart_ret_t dart_adapt_lock_acquire(dart_lock_t lock);

/** TODO: Need a parameter to indicate if the unit has gotten the lock or not.\n
 *  FIX:  Add the success param here.
 */

/** @brief Attempt to acquire a lock.
 *  
 *  If the lock is already claimed, it will return '0' in success, otherwise, return '1' in success. 
 *  @param[in] lock	Handle to the lock.
 *  @param[out] acquired	Indicates whether the lock was acquired or not.
 *  @return 		Dart status.
 */
dart_ret_t dart_adapt_lock_try_acquire(dart_lock_t lock, int *acquired);

/** @brief Release a lock.
 *   
 *  @param[in] lock	 Handle to the lock.
 *  @return		 Dart status.
 */

dart_ret_t dart_adapt_lock_release(dart_lock_t lock);


#define DART_INTERFACE_OFF

#ifdef __cplusplus
}
#endif

#endif /* DART_ADAPT_SYNCHRONIZATION_H_INCLUDED */

