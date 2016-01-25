/** @file dart_globmem.c
 *  @date 25 Aug 2014
 *  @brief Implementation of all the related global pointer operations
 *
 *  All the following functions are implemented with the underlying *MPI-3*
 *  one-sided runtime system.
 */

#include <stdio.h>
#include <mpi.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/if/dart_types.h>
#include <dash/dart/mpi/dart_mem.h>
#include <dash/dart/mpi/dart_translation.h>
#include <dash/dart/mpi/dart_team_private.h>
#include <dash/dart/if/dart_globmem.h>
#include <dash/dart/if/dart_team_group.h>
#include <dash/dart/if/dart_communication.h>


/* For PRIu64, uint64_t in printf */
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

/**
 * @note For dart collective allocation/free: offset in the returned gptr
 * represents the displacement relative to the beginning of sub-memory
 * spanned by certain dart collective allocation.
 * For dart local allocation/free: offset in the returned gptr represents
 * the displacement relative to
 * the base address of memory region reserved for the dart local
 * allocation/free.
 */
int16_t dart_memid;
int16_t dart_registermemid;
dart_ret_t dart_gptr_getaddr (const dart_gptr_t gptr, void **addr)
{
	int16_t seg_id = gptr.segid;
	uint64_t offset = gptr.addr_or_offs.offset;
	dart_unit_t myid;
	dart_myid (&myid);

	if (myid == gptr.unitid) {
		if (seg_id) {
			int flag;
			if (dart_adapt_transtable_get_selfbaseptr(seg_id, (char **)addr) == -1) {
				return DART_ERR_INVAL;}

			*addr = offset + (char *)(*addr);
		} else {
			if (myid == gptr.unitid) {
				*addr = offset + dart_mempool_localalloc;
			}
		}
	} else {
		*addr = NULL;
	}
	return DART_OK;
}

dart_ret_t dart_gptr_setaddr (dart_gptr_t* gptr, void* addr)
{
	int16_t seg_id = gptr->segid;
	/* The modification to addr is reflected in the fact that modifying
   * the offset.
   */
	if (seg_id) {
		MPI_Win win;
		char* addr_base;
    		if (dart_adapt_transtable_get_selfbaseptr(seg_id, &addr_base) == -1)
			return DART_ERR_INVAL;
		gptr->addr_or_offs.offset = (char *)addr - addr_base;
	} else {
		gptr->addr_or_offs.offset = (char *)addr - dart_mempool_localalloc;
	}
	return DART_OK;
}

dart_ret_t dart_gptr_incaddr (dart_gptr_t* gptr, int offs)
{
	gptr -> addr_or_offs.offset += offs;
	return DART_OK;
}


dart_ret_t dart_gptr_setunit (dart_gptr_t* gptr, dart_unit_t unit_id)
{
	gptr->unitid = unit_id;
	return DART_OK;
}

dart_ret_t dart_memalloc (size_t nbytes, dart_gptr_t *gptr)
{
	dart_unit_t unitid;
	dart_myid (&unitid);
	gptr->unitid = unitid;
	gptr->segid = 0; /* For local allocation, the segid is marked as '0'. */
	gptr->flags = 0; /* For local allocation, the flag is marked as '0'. */
	gptr->addr_or_offs.offset = dart_buddy_alloc (dart_localpool, nbytes);
	if (gptr->addr_or_offs.offset == -1) {
		DART_LOG_ERROR ("Out of bound: the global memory is exhausted");
		return DART_ERR_OTHER;
	}
	DART_LOG_DEBUG("%2d: LOCALALLOC - %d bytes, offset = %d",
        unitid, nbytes, gptr->addr_or_offs.offset);
	return DART_OK;
}

dart_ret_t dart_memfree (dart_gptr_t gptr)
{
  if (dart_buddy_free (dart_localpool, gptr.addr_or_offs.offset) == -1) {
    DART_LOG_ERROR("Free invalid local global pointer: "
          "invalid offset = %"PRIu64"\n", gptr.addr_or_offs.offset);
		return DART_ERR_INVAL;
	}
	DART_LOG_DEBUG("%2d: LOCALFREE - offset = %llu",
        gptr.unitid, gptr.addr_or_offs.offset);
	return DART_OK;
}

dart_ret_t
dart_team_memalloc_aligned(
  dart_team_t   teamid,
  size_t        nbytes,
  int           elem_size,
  dart_gptr_t * gptr)
{
	size_t size;
	dart_unit_t unitid, gptr_unitid = -1;
	dart_team_myid(teamid, &unitid);
	dart_team_size (teamid, &size);

	char *sub_mem;

	/* The units belonging to the specified team are eligible to participate
	 * below codes enclosed. */

	MPI_Win win;
	MPI_Comm comm;
	MPI_Aint disp;
	MPI_Aint* disp_set = (MPI_Aint*)malloc (size * sizeof (MPI_Aint));

	uint16_t index;
	int result = dart_adapt_teamlist_convert(teamid, &index);

	if (result == -1) {
		return DART_ERR_INVAL;
	}
	comm = dart_teams[index];
#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
	MPI_Win sharedmem_win;
	MPI_Comm sharedmem_comm;
	sharedmem_comm = dart_sharedmem_comm_list[index];
#endif
	dart_unit_t localid = 0;
	if (index == 0) {
		gptr_unitid = localid;
	} else {
		MPI_Group group;
		MPI_Group group_all;
		MPI_Comm_group (comm, &group);
		MPI_Comm_group (MPI_COMM_WORLD, &group_all);
		MPI_Group_translate_ranks (group, 1, &localid, group_all, &gptr_unitid);
	}
#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
	MPI_Info win_info;
	MPI_Info_create (&win_info);
	MPI_Info_set(win_info, "alloc_shared_noncontig", "true");

	/* Allocate shared memory on sharedmem_comm, and create the related
   * sharedmem_win */

  /* NOTE:
   * Windows should definitely be optimized for the concrete value type i.e.
   * via MPI_Type_create_index_block as this greatly improves performance of
   * MPI_Get, MPI_Put and other RMA friends.
   *
   * !!! BUG IN INTEL-MPI 5.0
   * !!!
   * !!! See:
   * !!! https://software.intel.com/de-de/forums/intel-clusters-and-hpc-technology/topic/519995
   * !!!
   * !!! Quote:
   * !!!  "[When allocating, e.g., an] integer*4-array of array dimension N,
   * !!!   then use it by the MPI-processes (on the same node), and then
   * !!!   repeats the same for the next shared allocation [...] the number of
   * !!!   shared windows do accumulate in the run, because I do not free the
   * !!!   shared windows allocated so far. This allocation of shared windows
   * !!!   works, but only until the total number of allocated memory exceeds
   * !!!   a limit of ~30 millions of Integer*4 numbers (~120 MB).
   * !!!   When that limit is reached, the next call of
   * !!!   MPI_WIN_ALLOCATE_SHARED, MPI_WIN_SHARED_QUERY to allocated one
   * !!!   more shared window do not give an error message, but the 1st
   * !!!   attempt to use that allocated shared array results in a bus error
   * !!!   (because the shared array has not been allocated correctly)."
   * !!!
   * !!! Reproduced on SuperMUC and mpich3.1 on projekt03.
   */
	MPI_Win_allocate_shared(
    nbytes,
//  sizeof (char),
    elem_size,
    win_info,
    sharedmem_comm,
    &sub_mem,
    &sharedmem_win);

	int         sharedmem_unitid;
	MPI_Aint    winseg_size;
	char     ** baseptr_set;
	char      * baseptr;
	int         disp_unit, i;
	MPI_Comm_rank (sharedmem_comm, &sharedmem_unitid);
	baseptr_set = (char**)malloc(sizeof(char*) * dart_sharedmemnode_size[index]);

	for (i = 0; i < dart_sharedmemnode_size[index]; i++)
	{
		if (sharedmem_unitid != i) {
			MPI_Win_shared_query(sharedmem_win, i, &winseg_size, &disp_unit, &baseptr);
			baseptr_set[i] = baseptr;
		}
		else
		{
      baseptr_set[i] = sub_mem;
    }
	}
#else
	MPI_Alloc_mem(nbytes, MPI_INFO_NULL, &sub_mem);
#endif
	win = dart_win_lists[index];
	/* Attach the allocated shared memory to win */
	MPI_Win_attach(win, sub_mem, nbytes);

	MPI_Get_address(sub_mem, &disp);

	/* Collect the disp information from all the ranks in comm */
	MPI_Allgather(&disp, 1, MPI_AINT, disp_set, 1, MPI_AINT, comm);

	/* -- Updating infos on gptr -- */
	gptr->unitid = gptr_unitid;
  /* Segid equals to dart_memid (always a positive integer), identifies an
   * unique collective global memory. */
	gptr->segid  = dart_memid;
  /* For collective allocation, the flag is marked as 'index' */
	gptr->flags  = index;
	gptr->addr_or_offs.offset = 0;

	/* Updating the translation table of teamid with the created
   * (offset, win) infos */
	info_t item;
	item.seg_id  = dart_memid;
	item.size    = nbytes;
	item.disp    = disp_set;
#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
	item.win     = sharedmem_win;
	item.baseptr = baseptr_set;
#else
	item.win     = MPI_WIN_NULL;
	item.baseptr = NULL;
#endif
	item.selfbaseptr = sub_mem;
	/* Add this newly generated correspondence relationship record into the
   * translation table. */
	dart_adapt_transtable_add(item);
#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
	MPI_Info_free(&win_info);
#endif
	dart_memid++;

  DART_LOG_DEBUG(
    "dart_team_memalloc_aligned: bytes:%lu offset:%lu gptr_unitid:%d "
    "across team %d",
		nbytes, 0, gptr_unitid, teamid);

	return DART_OK;
}

dart_ret_t dart_team_memfree (dart_team_t teamid, dart_gptr_t gptr)
{
	dart_unit_t unitid;
       	dart_team_myid (teamid, &unitid);
	uint16_t index = gptr.flags;
	char *sub_mem;

	MPI_Win win;

	int flag;
  	int16_t seg_id = gptr.segid;

	win = dart_win_lists[index];

       if (dart_adapt_transtable_get_selfbaseptr (seg_id, &sub_mem) == -1)
	       return DART_ERR_INVAL;

        /* Detach the freed sub-memory from win */
	MPI_Win_detach (win, sub_mem);

	/* Release the shared memory win object related to the freed shared
   * memory */

#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
	MPI_Win sharedmem_win;
	if (dart_adapt_transtable_get_win (seg_id, &sharedmem_win) == -1)
		return DART_ERR_INVAL;
	MPI_Win_free (&sharedmem_win);
#endif
  DART_LOG_DEBUG("%2d: COLLECTIVEFREE - offset = %d, gptr_unitid = %d "
        "across team %d",
        unitid, gptr.addr_or_offs.offset, gptr.unitid, teamid);
	/* Remove the related correspondence relation record from the related
   * translation table. */
	if (dart_adapt_transtable_remove (seg_id) == -1) {
		return DART_ERR_INVAL;
	}
	return DART_OK;
}

dart_ret_t
dart_team_memregister_aligned (
   dart_team_t teamid,
   size_t nbytes,
   void *addr,
   dart_gptr_t *gptr)
{
	size_t size;
	dart_unit_t unitid, gptr_unitid = -1;
	dart_team_myid (teamid, &unitid);
	dart_team_size (teamid, &size);

	MPI_Win win;
	MPI_Comm comm;
	MPI_Aint disp;
	MPI_Aint *disp_set = (MPI_Aint*)malloc (size * sizeof (MPI_Aint));
	uint16_t index;
	int result = dart_adapt_teamlist_convert (teamid, &index);

	if (result == -1){
		return DART_ERR_INVAL;
	}
	comm = dart_teams[index];
	dart_unit_t localid = 0;
	if (index == 0){
		gptr_unitid = localid;
	}else{
		MPI_Group group;
		MPI_Group group_all;
		MPI_Comm_group (comm, &group);
		MPI_Comm_group (MPI_COMM_WORLD, &group_all);
		MPI_Group_translate_ranks (group, 1, &localid,
				group_all, &gptr_unitid);
	}
	win = dart_win_lists[index];
	MPI_Win_attach (win, (char*)addr, nbytes);
	MPI_Get_address ((char*)addr, &disp);
	MPI_Allgather (&disp, 1, MPI_AINT, disp_set, 1, MPI_AINT, comm);
	gptr->unitid = gptr_unitid;
	gptr->segid = dart_registermemid;
	gptr->flags = index;
	gptr->addr_or_offs.offset = 0;
	info_t item;
	item.seg_id = dart_registermemid;
	item.size = nbytes;
	item.disp = disp_set;
	item.win = MPI_WIN_NULL;
	item.baseptr = NULL;
	item.selfbaseptr = (char*)addr;
	dart_adapt_transtable_add (item);
	dart_registermemid --;
	DART_LOG_DEBUG("%2d: COLLECTIVEALLOC - %d bytes, offset = %d, gptr_unitid = %d"
			"across team %d",
			unitid, nbytes, 0, gptr_unitid, teamid);
	return DART_OK;
}

dart_ret_t
dart_team_memderegister(
   dart_team_t teamid,
   dart_gptr_t gptr)
{
	dart_unit_t unitid;
	dart_team_myid (teamid, &unitid);
	uint16_t index = gptr.flags;
	char *sub_mem;

	MPI_Win win;

	int flag;
	int16_t seg_id = gptr.segid;

	win = dart_win_lists[index];

	if (dart_adapt_transtable_get_selfbaseptr (seg_id, &sub_mem) == -1)
		return DART_ERR_INVAL;
	MPI_Win_detach (win, sub_mem);
	if (dart_adapt_transtable_remove (seg_id) == -1){
		return DART_ERR_INVAL;
	}
        DART_LOG_DEBUG("%2D: COLLECTIVEFREE - offset = %d, gptr_unitid = %d"
			"across team %d",
			unitid, gptr.addr_or_offs.offset, gptr.unitid,
			teamid);
	return DART_OK;
}


