/** @file dart_adapt_synchronization_priv.h
 *  @date 25 Mar 2014
 *  @brief Definition of dart_lock_struct.
 */

#ifndef DART_ADAPT_SYNCHRONIZATION_PRIV_H_INCLUDED
#define DART_ADAPT_SYNCHRONZIATION_PRIV_H_INCLUDED

#include <stdio.h>
#include <mpi.h>
#include "dart_globmem.h"
#include "dart_synchronization.h"

/** @brief Dart lock type.
 *  */
struct dart_lock_struct
{
	dart_gptr_t gptr_tail; /**< Pointer to the tail of lock queue. Stored in unit 0 by default. */
	dart_gptr_t gptr_list; /**< Pointer to next waiting unit. envisioned as a distributed list across the teamid. */
	dart_team_t teamid; 
	MPI_Comm comm; /**< Communicator corresponding to the above teamid. */
	MPI_Win win; /**< Distinguish different locks on the identical team. */
	int acquired; /**< Indicate if certain unit has acquired the lock or not. */
};

#endif /* DART_ADAPT_SYNCHRONIZATION_PRIV_H_INCLUDED */
