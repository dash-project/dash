/** @file dart_adapt_synchronization.h
 *  @date 19 Nov 2013
 *  @brief Function prototypes for all the synchronization operations.
 *
 *  the implementation based on MPI-3 standard and MCS algorithm.
 *  @see http://www.mcs.anl.gov/uploads/cels/papers/P4014-0113.pdf 
 *  @see http://www.cise.ufl.edu/tr/DOC/REP-1992-71.pdf
 */
#ifndef DART_ADAPT_SYNCHRONIZATION_H_INCLUDED
#define DART_ADAPT_SYNCHRONIZATION_H_INCLUDED

#include <mpi.h>
#include "dart_deb_log.h"
#include "dart_globmem.h"
#include "dart_synchronization.h"
#ifdef __cplusplus
extern "C" {
#endif

#define DART_INTERFACE_ON

/** @brief Dart lock type.
 */
struct dart_lock_struct
{
	dart_gptr_t gptr_tail; /**< Pointer to the tail of lock queue. Stored in unit 0 by default. */
	dart_gptr_t gptr_list; /**< Pointer to next waiting unit. envisioned as a distributed list across the teamid. */
	dart_team_t teamid; 
	MPI_Comm comm; /**< Communicator corresponding to the above teamid. */
	MPI_Win win; /**< Distinguish different locks on the identical team. */
	int acquired; /**< Indicate if certain unit has acquired the lock or not. */
};

#define DART_LOCK_TAIL_DISP 0

/** @brief Create an lock. Collective on teamid.
 *   
 *  @param[in] 	teamid  Team containing all the processes that will use the lock.
 *  @param[out]	lock	Pointer to the lock.
 *  @return		Dart status.
 */

dart_ret_t dart_adapt_team_lock_init(dart_team_t teamid, 
			       dart_lock_t* lock);

/** TODO: Deleting the redudent parameter teamid, no use here. 
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

/** TODO: Adding the parameter success.
 */

/** @brief Attempt to acquire a lock.
 *  
 *  If the lock is already claimed, it will return '0' in success, otherwise, return '1' in success. 
 *  @param[in] lock	Handle to the lock.
 *  @param[out] success	Indicates whether the lock was acquired or not.
 *  @return 		Dart status.
 */
dart_ret_t dart_adapt_lock_try_acquire(dart_lock_t lock, int *success);

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

