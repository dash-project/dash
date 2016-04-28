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
#include <dash/dart/mpi/dart_communication_priv.h>


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
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	if (user_comm_world != MPI_COMM_NULL){
#endif
#endif
	int16_t seg_id = gptr.segid;
	uint64_t offset = gptr.addr_or_offs.offset;
	dart_unit_t myid;
	dart_myid (&myid);
  
	if (myid == gptr.unitid) {
		if (seg_id) {
		//	int flag;
			if (dart_adapt_transtable_get_selfbaseptr(seg_id, (char **)addr) == -1) {
				return DART_ERR_INVAL;}

			*addr = offset + (char *)(*addr);
		} else {
				*addr = offset + dart_mempool_localalloc;
		}
	} else {*addr = NULL;
	}
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	}
#endif
#endif
	return DART_OK;
}

dart_ret_t dart_gptr_setaddr (dart_gptr_t* gptr, void* addr)
{
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	if (user_comm_world != MPI_COMM_NULL){
#endif
#endif
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
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	}
#endif 
#endif
	return DART_OK;
}

dart_ret_t dart_gptr_incaddr (dart_gptr_t* gptr, int offs)
{
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	if (user_comm_world != MPI_COMM_NULL){
#endif
#endif
	gptr -> addr_or_offs.offset += offs;
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	}
#endif
#endif
	return DART_OK;
}


dart_ret_t dart_gptr_setunit (dart_gptr_t* gptr, dart_unit_t unit_id)
{
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	if (user_comm_world != MPI_COMM_NULL){
#endif
#endif
	gptr->unitid = unit_id;
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	}
#endif
#endif
	return DART_OK;
}

dart_ret_t dart_memalloc (size_t nbytes, dart_gptr_t *gptr)
{
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	if (user_comm_world != MPI_COMM_NULL){
#endif
#endif
	dart_unit_t unitid;
	dart_myid (&unitid);
	//printf ("dart_memalloc: unitid %d\n", unitid);
	gptr->unitid = unitid;
	gptr->segid = 0; /* For local allocation, the segid is marked as '0'. */
	gptr->flags = 0; /* For local allocation, the flag is marked as '0'. */
	
	gptr->addr_or_offs.offset = dart_buddy_alloc (dart_localpool, nbytes);
	
	if (gptr->addr_or_offs.offset == -1) {
		return DART_ERR_OTHER;
	}
	DART_LOG_DEBUG("%2d: LOCALALLOC - %d bytes, offset = %d\n",
        unitid, nbytes, gptr->addr_or_offs.offset);
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	}
#endif
#endif
	return DART_OK;
}

dart_ret_t dart_memfree (dart_gptr_t gptr)
{	
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	if (user_comm_world != MPI_COMM_NULL){
#endif
#endif
  if (dart_buddy_free (dart_localpool, gptr.addr_or_offs.offset) == -1) {
    DART_LOG_ERROR("Free invalid local global pointer: "
          "invalid offset = %"PRIu64"\n", gptr.addr_or_offs.offset);
		return DART_ERR_INVAL;
	}
	DART_LOG_DEBUG("%2d: LOCALFREE - offset = %llu\n",
        gptr.unitid, gptr.addr_or_offs.offset);
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	}
#endif
#endif
	return DART_OK;
}

dart_ret_t
dart_team_memalloc_aligned(
  dart_team_t teamid,
  size_t nbytes,
  dart_gptr_t *gptr)
{
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	if (user_comm_world != MPI_COMM_WORLD){
#endif
#endif
	size_t size;
 	dart_unit_t unitid, gptr_unitid = -1;
	dart_myid (&unitid);
//	dart_team_myid(teamid, &unitid);
	dart_team_size (teamid, &size);
	
	char *sub_mem;

	/* The units belonging to the specified team are eligible to participate
	 * below codes enclosed. */
	 
	MPI_Win win;
	MPI_Comm comm;
	MPI_Aint disp;
	MPI_Aint* disp_set = (MPI_Aint*)malloc (size * sizeof (MPI_Aint));
	
	uint16_t index;
	int result = dart_adapt_teamlist_convert (teamid, &index);

	if (result == -1) {
		return DART_ERR_INVAL;
	}
	comm = dart_teams[index];
#ifdef SHAREDMEM_ENABLE
	MPI_Win sharedmem_win;
	MPI_Comm sharedmem_comm;
	sharedmem_comm = dart_sharedmem_comm_list[index];
#endif
	dart_unit_t localid = 0;
	if (index == 0) {
		gptr_unitid = localid;		
	} else {
		MPI_Group group;
		MPI_Group user_group_all;
		MPI_Comm_group (comm, &group);
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
		MPI_Comm_group (user_comm_world, &user_group_all);
#endif
#endif
#ifndef PROGRESS_ENABLE
		MPI_Comm_group (MPI_COMM_WORLD, &user_group_all);
#endif

		MPI_Group_translate_ranks (group, 1, &localid, user_group_all, &gptr_unitid);
	}
#ifdef SHAREDMEM_ENABLE
	int shmem_unitid;
	MPI_Comm_rank (sharedmem_comm, &shmem_unitid);
#ifdef PROGRESS_ENABLE
	//if (sharedmem_comm != MPI_COMM_NULL)
//	MPI_Comm_rank (sharedmem_comm, &shmem_unitid);
	if (shmem_unitid == PROGRESS_NUM){
		int i;
		for (i = 0; i < PROGRESS_NUM; i++){
			MPI_Send (&dart_progress_index[index], 1, MPI_INT32_T, PROGRESS_UNIT+i, MEMALLOC, dart_sharedmem_comm_list[0]);
		}
	}

//	printf ("shmem_unitid is %d still\n", shmem_unitid);
	MPI_Win_allocate_shared (nbytes, sizeof(char), MPI_INFO_NULL, sharedmem_comm, 
			&sub_mem, &sharedmem_win);
//	printf ("shmem_unitid is %d still\n", shmem_unitid);
#else

	MPI_Info win_info;
	MPI_Info_create (&win_info);
	MPI_Info_set (win_info, "alloc_shared_noncontig", "true");
	
	/* Allocate shared memory on sharedmem_comm, and create the related
   * sharedmem_win */
	MPI_Win_allocate_shared(
    nbytes, sizeof (char),
    win_info,
    sharedmem_comm,
    &sub_mem,
    &sharedmem_win);
#endif
//	int sharedmem_unitid;
	MPI_Aint winseg_size;
	char**baseptr_set;
	char *baseptr;
	int disp_unit, i;
//	MPI_Comm_rank (sharedmem_comm, &sharedmem_unitid);
	baseptr_set = (char**)malloc (sizeof (char*) * dart_sharedmemnode_size[index]);

#ifdef PROGRESS_ENABLE
	for (i = PROGRESS_NUM; i < dart_sharedmemnode_size[index]; i++)
#else
	for (i = 0; i < dart_sharedmemnode_size[index]; i++)
#endif
	{
		if (shmem_unitid != i){
			MPI_Win_shared_query (sharedmem_win, i, &winseg_size, &disp_unit, &baseptr);
			baseptr_set[i] = baseptr;
		}
		else 
		{baseptr_set[i] = sub_mem;}
	}
#else
	MPI_Alloc_mem (nbytes, MPI_INFO_NULL, &sub_mem);
#endif		
	win = dart_win_lists[index];
	/* Attach the allocated shared memory to win */
	MPI_Win_attach (win, sub_mem, nbytes);

	MPI_Get_address (sub_mem, &disp);

	/* Collect the disp information from all the ranks in comm */
	MPI_Allgather (&disp, 1, MPI_AINT, disp_set, 1, MPI_AINT, comm);
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	MPI_Comm real_comm = dart_realteams[index];
	int16_t max_memid;
//	printf ("inside memalloc_aligned still\n");
	MPI_Allreduce (&dart_memid, &max_memid, 1, MPI_INT16_T, MPI_MAX, real_comm);
#endif
#endif

	/* -- Updating infos on gptr -- */
	gptr->unitid = gptr_unitid;
  /* Segid equals to dart_memid (always a positive integer), identifies an
   * unique collective global memory. */
#ifndef PROGRESS_ENABLE
	gptr->segid = dart_memid;
#else
	gptr->segid = max_memid;
#endif
//	gptr->segid  = dart_memid;
  /* For collective allocation, the flag is marked as 'index' */
	gptr->flags  = index;
	gptr->addr_or_offs.offset = 0;
	
	/* Updating the translation table of teamid with the created
   * (offset, win) infos */
	info_t item;
	item.seg_id = gptr->segid;
	item.size   = nbytes;
  	item.disp   = disp_set;
#ifdef SHAREDMEM_ENABLE
	item.win    = sharedmem_win;
	item.baseptr = baseptr_set;
#else
	item.win   = MPI_WIN_NULL;
	item.baseptr = NULL;
#endif
	item.selfbaseptr = sub_mem;
	/* Add this newly generated correspondence relationship record into the 
   * translation table. */
	dart_adapt_transtable_add (item);
#ifdef SHAREDMEM_ENABLE
#ifndef PROGRESS_ENABLE
	MPI_Info_free (&win_info);
#endif
#endif
#ifndef PROGRESS_ENABLE
	dart_memid++;
#else
	dart_memid = max_memid + 1;
#endif
#ifdef SHAREDMEM_ENABLE
	MPI_Win_lock_all (0, sharedmem_win);
#endif

  DART_LOG_DEBUG(
    "%2d: COLLECTIVEALLOC - %d bytes, offset = %d, gptr_unitid = %d "
    "across team %d\n",
		unitid, nbytes, 0, gptr_unitid, teamid);
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	}
#endif
#endif

	return DART_OK;
}

dart_ret_t dart_team_memfree (dart_team_t teamid, dart_gptr_t gptr)
{	
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
        if (user_comm_world != MPI_COMM_NULL){
#endif
#endif	
	dart_unit_t id, unitid;
       	dart_team_myid (teamid, &id);
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

#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	MPI_Comm sharedmem_comm = dart_sharedmem_comm_list[index];
	MPI_Comm_rank (sharedmem_comm, &unitid);
	if (unitid == PROGRESS_NUM){
		int i;
		for (i = 0; i < PROGRESS_NUM; i++){
			MPI_Send (&seg_id, 1, MPI_INT16_T, PROGRESS_UNIT+i, MEMFREE, dart_sharedmem_comm_list[0]);
		}
	}
#endif
	MPI_Win sharedmem_win;
	if (dart_adapt_transtable_get_win (seg_id, &sharedmem_win) == -1)
		return DART_ERR_INVAL;
	MPI_Win_unlock_all (sharedmem_win);
	MPI_Win_free (&sharedmem_win); 
#endif

  DART_LOG_DEBUG("%2d: COLLECTIVEFREE - offset = %d, gptr_unitid = %d "
        "across team %d\n", 
        id, gptr.addr_or_offs.offset, gptr.unitid, teamid);
	/* Remove the related correspondence relation record from the related 
   * translation table. */
	if (dart_adapt_transtable_remove (seg_id) == -1) {
		return DART_ERR_INVAL;
	}
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	}
#endif
#endif
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
			"across team %d\n",
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
			"across team %d\n",
			unitid, gptr.addr_or_offs.offset, gptr.unitid,
			teamid);
	return DART_OK;
}


