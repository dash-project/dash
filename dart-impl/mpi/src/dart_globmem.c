/**
 * \file dart_globmem.c
 *
 * Implementation of all the related global pointer operations
 *
 * All the following functions are implemented with the underlying *MPI-3*
 * one-sided runtime system.
 */

#include <dash/dart/base/logging.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_globmem.h>
#include <dash/dart/if/dart_team_group.h>

#include <dash/dart/mpi/dart_communication_priv.h>
#include <dash/dart/mpi/dart_mpi_util.h>
#include <dash/dart/mpi/dart_mem.h>
#include <dash/dart/mpi/dart_team_private.h>
#include <dash/dart/mpi/dart_segment.h>

#include <stdio.h>
#include <mpi.h>

/* For PRIu64, uint64_t in printf */
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

MPI_Win dart_win_local_alloc;
#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
MPI_Win dart_sharedmem_win_local_alloc;
#endif

dart_ret_t dart_gptr_getaddr(const dart_gptr_t gptr, void **addr)
{
  int16_t seg_id = gptr.segid;
  uint64_t offset = gptr.addr_or_offs.offset;
  dart_global_unit_t myid;
  dart_myid(&myid);

  uint16_t index;
  int result = dart_adapt_teamlist_convert(gptr.teamid, &index);
  if (result == -1) {
    DART_LOG_ERROR("dart_gptr_getaddr: Unknown team %i", gptr.teamid);
    return DART_ERR_INVAL;
  }

  if (myid.id == gptr.unitid) {
    if (seg_id != DART_SEGMENT_LOCAL) {
      if (dart_segment_get_selfbaseptr(&dart_team_data[index].segdata, seg_id, (char **)addr) != DART_OK) {
        return DART_ERR_INVAL;
      }

      *addr = offset + (char *)(*addr);
    } else {
      *addr = offset + dart_mempool_localalloc;
    }
  } else {
    *addr = NULL;
  }
  return DART_OK;
}


dart_ret_t dart_gptr_incaddr(dart_gptr_t *gptr, int64_t offs)
{
  gptr->addr_or_offs.offset += offs;
  return DART_OK;
}


dart_ret_t dart_gptr_setaddr(dart_gptr_t* gptr, void* addr)
{
	int16_t seg_id = gptr->segid;
	/* The modification to addr is reflected in the fact that modifying
   * the offset.
   */

  uint16_t index;
  int result = dart_adapt_teamlist_convert(gptr->teamid, &index);
  if (result == -1) {
    return DART_ERR_INVAL;
  }

  if (seg_id != DART_SEGMENT_LOCAL) {
    char * addr_base;
    if (dart_segment_get_selfbaseptr(&dart_team_data[index].segdata, seg_id, &addr_base) != DART_OK) {
      return DART_ERR_INVAL;
    }
		gptr->addr_or_offs.offset = (char *)addr - addr_base;
	} else {
		gptr->addr_or_offs.offset = (char *)addr - dart_mempool_localalloc;
	}
	return DART_OK;
}

dart_ret_t dart_gptr_setunit(dart_gptr_t *gptr, dart_global_unit_t unit)
{
  gptr->unitid = unit.id;
  return DART_OK;
}

dart_ret_t dart_gptr_getflags(dart_gptr_t gptr, uint16_t *flags)
{
  *flags = 0;

  uint16_t index;
  if (dart_adapt_teamlist_convert(gptr.teamid, &index) == -1) {
    DART_LOG_ERROR("dart_team_memalloc_aligned ! Unknown team %i", gptr.teamid);
    return DART_ERR_INVAL;
  }

  return dart_segment_get_flags(&dart_team_data[index].segdata, gptr.segid, flags);
}

dart_ret_t dart_gptr_setflags(dart_gptr_t gptr, uint16_t flags)
{
  uint16_t index;
  if (dart_adapt_teamlist_convert(gptr.teamid, &index) == -1) {
    DART_LOG_ERROR("dart_team_memalloc_aligned ! Unknown team %i", gptr.teamid);
    return DART_ERR_INVAL;
  }

  return dart_segment_set_flags(&dart_team_data[index].segdata, gptr.segid, flags);
}


dart_ret_t dart_memalloc(
  size_t            nelem,
  dart_datatype_t   dtype,
  dart_gptr_t     * gptr)
{
  size_t      nbytes = nelem * dart_mpi_sizeof_datatype(dtype);
  dart_global_unit_t unitid;
  dart_myid(&unitid);
  gptr->unitid  = unitid.id;
  gptr->segid   = DART_SEGMENT_LOCAL; /* For local allocation, the segid is marked as '0'. */
  gptr->teamid  = DART_TEAM_ALL;      /* Locally allocated gptr belong to the global team. */
  gptr->addr_or_offs.offset = dart_buddy_alloc(dart_localpool, nbytes);
  if (gptr->addr_or_offs.offset == (uint64_t)(-1)) {
    DART_LOG_ERROR("dart_memalloc: Out of bounds "
                   "(dart_buddy_alloc %zu bytes): global memory exhausted",
                   nbytes);
    return DART_ERR_OTHER;
  }
  DART_LOG_DEBUG("dart_memalloc: local alloc nbytes:%lu offset:%"PRIu64"",
                 nbytes, gptr->addr_or_offs.offset);
	return DART_OK;
}

dart_ret_t dart_memfree (dart_gptr_t gptr)
{

  if (gptr.segid != DART_SEGMENT_LOCAL) {
    DART_LOG_ERROR("dart_memfree: invalid segment id: %d", gptr.segid);
    return DART_ERR_INVAL;
  }

  if (dart_buddy_free(dart_localpool, gptr.addr_or_offs.offset) == -1) {
    DART_LOG_ERROR("dart_memfree: invalid local global pointer: "
                   "invalid offset: %"PRIu64"",
                   gptr.addr_or_offs.offset);
		return DART_ERR_INVAL;
	}
	DART_LOG_DEBUG("dart_memfree: local free, gptr.unitid:%2d offset:%"PRIu64"",
                 gptr.unitid, gptr.addr_or_offs.offset);
	return DART_OK;
}

dart_ret_t
dart_team_memalloc_aligned(
  dart_team_t       teamid,
  size_t            nelem,
  dart_datatype_t   dtype,
  dart_gptr_t     * gptr)
{
  int         dtype_size = dart_mpi_sizeof_datatype(dtype);
  MPI_Aint    nbytes     = nelem * dtype_size;
	size_t      team_size;
  dart_unit_t gptr_unitid = -1;
	dart_team_size(teamid, &team_size);

  *gptr = DART_GPTR_NULL;

	char * sub_mem;

	uint16_t index;
  if (dart_adapt_teamlist_convert(teamid, &index) == -1) {
    DART_LOG_ERROR("dart_team_memalloc_aligned ! Unknown team %i", teamid);
    return DART_ERR_INVAL;
  }
  DART_LOG_DEBUG(
    "dart_team_memalloc_aligned: dart_adapt_teamlist_convert completed, "
    "index:%d", index);


  dart_team_data_t *team_data = &dart_team_data[index];
  MPI_Comm          comm      = team_data->comm;
	dart_unit_t       localid   = 0;

	if (index == 0) {
		gptr_unitid = localid;
	} else {
		MPI_Group group;
		MPI_Group group_all;
		MPI_Comm_group(comm, &group);
		MPI_Comm_group(DART_COMM_WORLD, &group_all);
		MPI_Group_translate_ranks(group, 1, &localid, group_all, &gptr_unitid);
	}
#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)

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
   *
   * Related support ticket of MPICH:
   * http://trac.mpich.org/projects/mpich/ticket/2178
   */
  MPI_Win  sharedmem_win;
  MPI_Comm sharedmem_comm = team_data->sharedmem_comm;

	MPI_Info win_info;
	MPI_Info_create(&win_info);
	MPI_Info_set(win_info, "alloc_shared_noncontig", "true");

  DART_LOG_DEBUG("dart_team_memalloc_aligned: "
                 "MPI_Win_allocate_shared(nbytes:%ld)", nbytes);

	if (sharedmem_comm != MPI_COMM_NULL) {
    int ret = MPI_Win_allocate_shared(
                nbytes,     // number of bytes
                dtype_size, // displacement unit
                win_info,
                sharedmem_comm,
                &sub_mem,
                &sharedmem_win);
    if (ret != MPI_SUCCESS) {
      DART_LOG_ERROR("dart_team_memalloc_aligned: "
                     "MPI_Win_allocate_shared failed, error %d (%s)",
                     ret, DART__MPI__ERROR_STR(ret));
      return DART_ERR_OTHER;
    }
  } else {
    DART_LOG_ERROR("dart_team_memalloc_aligned: "
                   "Shared memory communicator is MPI_COMM_NULL, "
                   "cannot call MPI_Win_allocate_shared");
    return DART_ERR_OTHER;
  }

  MPI_Aint winseg_size;
  int      sharedmem_unitid;
  char **  baseptr_set;
  char *   baseptr;
  int      disp_unit, i;
  MPI_Comm_rank(sharedmem_comm, &sharedmem_unitid);
  baseptr_set = (char **)malloc(
      sizeof(char *) * team_data->sharedmem_nodesize);

  for (i = 0; i < team_data->sharedmem_nodesize; i++) {
    if (sharedmem_unitid != i) {
      MPI_Win_shared_query(sharedmem_win, i, &winseg_size, &disp_unit,
                           &baseptr);
      baseptr_set[i] = baseptr;
    } else {
      baseptr_set[i] = sub_mem;
    }
	}
#else
	if (MPI_Alloc_mem(nbytes, MPI_INFO_NULL, &sub_mem) != MPI_SUCCESS) {
    DART_LOG_ERROR(
      "dart_team_memalloc_aligned: bytes:%lu MPI_Alloc_mem failed", nbytes);
    return DART_ERR_OTHER;
  }
#endif

  MPI_Aint disp;
  MPI_Win  win = team_data->window;
  /* Attach the allocated shared memory to win */
  /* Calling MPI_Win_attach with nbytes == 0 leads to errors, see #239 */
  if (nbytes > 0) {
    if (MPI_Win_attach(win, sub_mem, nbytes) != MPI_SUCCESS) {
      DART_LOG_ERROR(
        "dart_team_memalloc_aligned: bytes:%lu MPI_Win_attach failed", nbytes);
#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
      free(baseptr_set);
#endif
      return DART_ERR_OTHER;
    }

    if (MPI_Get_address(sub_mem, &disp) != MPI_SUCCESS) {
      DART_LOG_ERROR(
        "dart_team_memalloc_aligned: bytes:%lu MPI_Get_address failed", nbytes);
#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
      free(baseptr_set);
#endif
      return DART_ERR_OTHER;
    }
  } else {
    disp = 0;
  }

  MPI_Aint * disp_set = (MPI_Aint*)(malloc(team_size * sizeof (MPI_Aint)));
	/* Collect the disp information from all the ranks in comm */
	MPI_Allgather(&disp, 1, MPI_AINT, disp_set, 1, MPI_AINT, comm);


  /* Updating the translation table of teamid with the created
   * (offset, win) infos */
  dart_segment_info_t item;
  item.seg_id  = team_data->dart_memid;
  item.size    = nbytes;
  item.disp    = disp_set;
  item.flags   = 0;
#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
	item.win     = sharedmem_win;
	item.baseptr = baseptr_set;
#else
	item.win     = MPI_WIN_NULL;
	item.baseptr = NULL;
#endif
	item.selfbaseptr = sub_mem;

  if (dart_segment_alloc(&team_data->segdata, &item) != DART_OK) {
    DART_LOG_ERROR(
        "dart_team_memalloc_aligned: "
        "bytes:%lu Allocation of segment data failed", nbytes);
#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
    free(baseptr_set);
#endif
    return DART_ERR_OTHER;
  }

  /* -- Updating infos on gptr -- */
  gptr->unitid = gptr_unitid;
  /* Segid equals to dart_memid (always a positive integer), identifies an
   * unique collective global memory. */
  gptr->segid = team_data->dart_memid;
  gptr->addr_or_offs.offset = 0;
  gptr->teamid = teamid;

#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
	MPI_Info_free(&win_info);
#endif
	team_data->dart_memid++;

  DART_LOG_DEBUG(
    "dart_team_memalloc_aligned: bytes:%lu offset:%d gptr_unitid:%d "
    "across team %d",
		nbytes, 0, gptr_unitid, teamid);

	return DART_OK;
}

dart_ret_t dart_team_memfree(
  dart_gptr_t gptr)
{
  int16_t seg_id = gptr.segid;
  char * sub_mem;
  MPI_Win win;
  dart_team_t teamid = gptr.teamid;

  uint16_t index;
  if (dart_adapt_teamlist_convert(gptr.teamid, &index) == -1) {
    DART_LOG_ERROR("dart_team_free ! failed: Unknown team %i!", gptr.teamid);
    return DART_ERR_INVAL;
  }

  if (DART_GPTR_ISNULL(gptr)) {
    /* corresponds to free(NULL) which is a valid operation */
    return DART_OK;
  }


  win = dart_team_data[index].window;

  if (dart_segment_get_selfbaseptr(&dart_team_data[index].segdata, seg_id, &sub_mem) != DART_OK) {
    return DART_ERR_INVAL;
  }

  /* Detach the window associated with sub-memory to be freed */
  if (sub_mem != NULL) {
    MPI_Win_detach(win, sub_mem);
  }

	/* Free the window's associated sub-memory */
#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
  MPI_Win sharedmem_win;
  if (dart_segment_get_win(&dart_team_data[index].segdata, seg_id, &sharedmem_win) != DART_OK) {
    return DART_ERR_OTHER;
  }
	if (MPI_Win_free(&sharedmem_win) != MPI_SUCCESS) {
    DART_LOG_ERROR("dart_team_memfree: MPI_Win_free failed");
    return DART_ERR_OTHER;
  }
#else
	if (MPI_Free_mem(sub_mem) != MPI_SUCCESS) {
    DART_LOG_ERROR("dart_team_memfree: MPI_Free_mem failed");
    return DART_ERR_OTHER;
  }
#endif

#ifdef DART_ENABLE_LOGGING
  dart_team_unit_t unitid;
  dart_team_myid(teamid, &unitid);
#endif
  DART_LOG_DEBUG("dart_team_memfree: collective free, team unit id: %2d "
                 "offset:%"PRIu64" gptr_unitid:%d across team %d",
                 unitid.id, gptr.addr_or_offs.offset, gptr.unitid, teamid);
	/* Remove the related correspondence relation record from the related
   * translation table. */
  if (dart_segment_free(&dart_team_data[index].segdata, seg_id) != DART_OK) {
    return DART_ERR_INVAL;
  }

  return DART_OK;
}

dart_ret_t
dart_team_memregister_aligned(
   dart_team_t       teamid,
   size_t            nelem,
   dart_datatype_t   dtype,
   void            * addr,
   dart_gptr_t     * gptr)
{
	size_t size;
  int    dtype_size = dart_mpi_sizeof_datatype(dtype);
  size_t nbytes     = nelem * dtype_size;
  dart_unit_t gptr_unitid = -1;
  dart_team_size(teamid, &size);

  *gptr = DART_GPTR_NULL;

  uint16_t index;
  int result = dart_adapt_teamlist_convert(teamid, &index);

  if (result == -1) {
    return DART_ERR_INVAL;
  }

  MPI_Aint disp;
  MPI_Aint * disp_set = (MPI_Aint *)malloc(size * sizeof(MPI_Aint));

  dart_team_data_t *team_data = &dart_team_data[index];
  MPI_Comm comm = team_data->comm;
  dart_unit_t localid = 0;
  if (index == 0) {
    gptr_unitid = localid;
  } else {
    MPI_Group group;
    MPI_Group group_all;
    MPI_Comm_group(comm, &group);
    MPI_Comm_group(DART_COMM_WORLD, &group_all);
    MPI_Group_translate_ranks(group, 1, &localid, group_all, &gptr_unitid);
  }
  MPI_Win win = team_data->window;
  MPI_Win_attach(win, (char *)addr, nbytes);
  MPI_Get_address((char *)addr, &disp);
  MPI_Allgather(&disp, 1, MPI_AINT, disp_set, 1, MPI_AINT, comm);

  dart_segment_info_t item;
  item.seg_id  = team_data->dart_registermemid;
  item.size    = nbytes;
  item.disp    = disp_set;
  item.win     = MPI_WIN_NULL;
  item.baseptr = NULL;
  item.selfbaseptr = (char *)addr;
  item.flags   = 0;
  if (dart_segment_alloc(&team_data->segdata, &item) != DART_OK) {
    DART_LOG_ERROR(
        "dart_team_memalloc_aligned: bytes:%lu Allocation of segment data failed",
        nbytes);
    free(disp_set);
    return DART_ERR_OTHER;
  }

  gptr->unitid = gptr_unitid;
  gptr->segid  = team_data->dart_registermemid;
  gptr->teamid = teamid;
  gptr->addr_or_offs.offset = 0;

  team_data->dart_registermemid--;

#if DART_ENABLE_LOGGING
  dart_team_unit_t unitid;
  dart_team_myid(teamid, &unitid);
#endif
  DART_LOG_DEBUG(
    "dart_team_memregister_aligned: collective alloc, "
    "unit:%2d, nbytes:%zu offset:%d gptr_unitid:%d " "across team %d",
    unitid.id, nbytes, 0, gptr_unitid, teamid);
  return DART_OK;
}

dart_ret_t
dart_team_memregister(
   dart_team_t       teamid,
   size_t            nelem,
   dart_datatype_t   dtype,
   void            * addr,
   dart_gptr_t     * gptr)
{
  int    nil;
	size_t size;
  int    dtype_size = dart_mpi_sizeof_datatype(dtype);
  size_t nbytes     = nelem * dtype_size;
  dart_unit_t gptr_unitid = -1;
	dart_team_size(teamid, &size);

  *gptr = DART_GPTR_NULL;

  if (nbytes == 0) {
    // Attaching empty memory region, set sendbuf to valid dummy pointer:
    addr = (void*)(&nil);
  }

	uint16_t   index;
	int        result   = dart_adapt_teamlist_convert(teamid, &index);
  dart_team_data_t *team_data = &dart_team_data[index];
  if (result == -1) {
    return DART_ERR_INVAL;
  }

  MPI_Aint   disp;
  MPI_Aint * disp_set = (MPI_Aint*)malloc(size * sizeof (MPI_Aint));
  MPI_Comm comm = dart_team_data[index].comm;
  dart_unit_t localid = 0;
  if (index == 0) {
    gptr_unitid = localid;
  } else {
    MPI_Group group;
    MPI_Group group_all;
    MPI_Comm_group(comm, &group);
    MPI_Comm_group(DART_COMM_WORLD, &group_all);
    MPI_Group_translate_ranks(group, 1, &localid, group_all, &gptr_unitid);
  }
  MPI_Win win = dart_team_data[index].window;
  MPI_Win_attach(win, (char *)addr, nbytes);
  MPI_Get_address((char *)addr, &disp);
  MPI_Allgather(&disp, 1, MPI_AINT, disp_set, 1, MPI_AINT, comm);

  dart_segment_info_t item;
  item.seg_id = team_data->dart_registermemid;
  item.size = nbytes;
  item.disp = disp_set;
  item.win = MPI_WIN_NULL;
  item.baseptr = NULL;
  item.selfbaseptr = (char *)addr;
  item.flags = 0;

  if (dart_segment_alloc(&team_data->segdata, &item) != DART_OK) {
    DART_LOG_ERROR(
        "dart_team_memalloc_aligned: bytes:%lu Allocation of segment data failed",
        nbytes);
    free(disp_set);
    return DART_ERR_OTHER;
  }

  gptr->unitid = gptr_unitid;
  gptr->segid  = team_data->dart_registermemid;
  gptr->teamid = teamid;
  gptr->addr_or_offs.offset = 0;

  team_data->dart_registermemid--;

#ifdef DART_ENABLE_LOGGING
  dart_team_unit_t unitid;
  dart_team_myid(teamid, &unitid);
#endif
  DART_LOG_DEBUG(
    "dart_team_memregister: collective alloc, "
    "unit:%2d, nbytes:%zu offset:%d gptr_unitid:%d " "across team %d",
    unitid.id, nbytes, 0, gptr_unitid, teamid);
  return DART_OK;
}

dart_ret_t
dart_team_memderegister(
   dart_gptr_t gptr)
{
  int16_t seg_id = gptr.segid;
  char * sub_mem;
  MPI_Win win;
  dart_team_t teamid = gptr.teamid;

  uint16_t index;
  int result = dart_adapt_teamlist_convert(gptr.teamid, &index);
  if (result == -1) {
    return DART_ERR_INVAL;
  }

  win = dart_team_data[index].window;

  if (dart_segment_get_selfbaseptr(&dart_team_data[index].segdata, seg_id, &sub_mem) != DART_OK) {
    return DART_ERR_INVAL;
  }

  MPI_Win_detach(win, sub_mem);
  if (dart_segment_free(&dart_team_data[index].segdata, seg_id) != DART_OK) {
    return DART_ERR_INVAL;
  }

#ifdef DART_ENABLE_LOGGING
  dart_team_unit_t unitid;
  dart_team_myid(teamid, &unitid);
#endif
  DART_LOG_DEBUG(
    "dart_team_memderegister: collective free, "
    "team unit %2d offset:%"PRIu64" gptr_unitid:%d" "across team %d",
    unitid.id, gptr.addr_or_offs.offset, gptr.unitid, teamid);
  return DART_OK;
}


