/*
 * dart_malloc.h
 *
 *  Created on: Apr 9, 2013
 *      Author: maierm
 */

#ifndef DART_MALLOC_H_
#define DART_MALLOC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "dart_gptr.h"

/*
 --- DART memory allocation ---
 */

/*
 dart_alloc() allocates nbytes of memory in the global address space
 of the calling unit and returns a global pointer to it.  This is not
 a collective function but a local function.  Probably trivial to
 implement on GASNet but difficult on MPI?
 */
gptr_t dart_alloc(size_t nbytes);

/**
 * ???????????????????????????????????????????????????????????????
 * same as dart_alloc but the returned pointer may only be used by
 * the specified team
 *
 * @param team_id: set of processes that may access the allocated memory
 */
//gptr_t dart_alloc_restricted(int team_id, size_t nbytes);

/*
 dart_team_alloc_aligned() is a collective function on the specified
 team. Each team member calls the function and must request the same
 amount of memory (nbytes). The return value of this function on each
 unit in the team is a global pointer pointing to the beginning of the
 allocation. The returned memory allocation is symmetric and aligned
 allowing for an easy determination of global poitners to anywhere in
 the allocated memory block
 */

/* Question: can a unit that was not part of the allocation team
 later access the memory? */

/*
 * Question: 'aligned' might also mean something different:
 * A pointer must be properly aligned in order to allow the creation of an object of a specified struct (or built-in type)
 * at the location the pointer is pointing to (see C-API -> malloc).
 * TODO:
 * - we must also define that the returned global pointer is properly aligned according to the rules malloc specifies?!
 * - PROBLEM: What about 'boundary'-memory addresses, i.e. addresses that might be used for object-creation, but there isn't
 *   sufficient space in the local memory segment (according to the object size)?
 */
gptr_t dart_alloc_aligned(int teamid, size_t nbytes);

/* collective call to free the allocated memory */
void dart_free(int teamid, gptr_t ptr);

// TODO: do we need this? how to free locally allocated memory?
// void dart_free(gptr_t ptr);

/**
 * reverves (dart_team_size(teamid)*local_size)-bytes of globally addressable memory
 * for the specified team. dart_alloc_alligned(teamid) might only be called AFTER a
 * call to this function.
 * TODO: error handling
 */
void dart_team_attach_mempool(int teamid, size_t local_size);
void dart_team_detach_mempool(int teamid);

#ifdef __cplusplus
}
#endif

#endif /* DART_MALLOC_H_ */
