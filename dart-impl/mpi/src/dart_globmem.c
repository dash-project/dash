/**
 * \file dart_globmem.c
 *
 * Implementation of all the related global pointer operations
 *
 * All the following functions are implemented with the underlying *MPI-3*
 * one-sided runtime system.
 */

#include <dash/dart/base/logging.h>
#include <dash/dart/base/atomic.h>
#include <dash/dart/base/assert.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_globmem.h>
#include <dash/dart/if/dart_team_group.h>

#include <dash/dart/mpi/dart_communication_priv.h>
#include <dash/dart/mpi/dart_mpi_util.h>
#include <dash/dart/mpi/dart_mem.h>
#include <dash/dart/mpi/dart_team_private.h>
#include <dash/dart/mpi/dart_segment.h>
#include <dash/dart/mpi/dart_globmem_priv.h>

#include <stdio.h>
#include <mpi.h>

/* For PRIu64, uint64_t in printf */
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

/**
 * TODO: add this window to the team_data for DART_TEAM_ALL as segment 0.
 */
MPI_Win dart_win_local_alloc;

dart_ret_t dart_gptr_getaddr(const dart_gptr_t gptr, void **addr)
{
  int16_t segid = gptr.segid;
  uint64_t offset = gptr.addr_or_offs.offset;
  dart_team_unit_t myid;
  dart_team_myid(gptr.teamid, &myid);

  dart_team_data_t *team_data = dart_adapt_teamlist_get(gptr.teamid);
  if (team_data == NULL) {
    DART_LOG_ERROR("dart_gptr_getaddr ! Unknown team %i", gptr.teamid);
    return DART_ERR_INVAL;
  }

  if (myid.id == gptr.unitid) {
    if (segid != DART_SEGMENT_LOCAL) {
      if (dart_segment_get_selfbaseptr(&team_data->segdata, segid, (char **)addr) != DART_OK) {
        DART_LOG_ERROR("dart_gptr_getaddr ! Unknown segment %i", segid);
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

dart_ret_t dart_gptr_setaddr(dart_gptr_t* gptr, void* addr)
{
  int16_t segid = gptr->segid;
  /* The modification to addr is reflected in the fact that modifying
   * the offset.
   */

  dart_team_data_t *team_data = dart_adapt_teamlist_get(gptr->teamid);
  if (team_data == NULL) {
    DART_LOG_ERROR("dart_gptr_setaddr ! Unknown team %i", gptr->teamid);
    return DART_ERR_INVAL;
  }

  if (segid != DART_SEGMENT_LOCAL) {
    char * addr_base;
    if (dart_segment_get_selfbaseptr(&team_data->segdata, segid, &addr_base) != DART_OK) {
      DART_LOG_ERROR("dart_gptr_setaddr ! Unknown segment %i", segid);
      return DART_ERR_INVAL;
    }
    gptr->addr_or_offs.offset = (char *)addr - addr_base;
  } else {
    gptr->addr_or_offs.offset = (char *)addr - dart_mempool_localalloc;
  }
  return DART_OK;
}

dart_ret_t dart_gptr_getflags(dart_gptr_t gptr, uint16_t *flags)
{
  *flags = 0;

  dart_team_data_t *team_data = dart_adapt_teamlist_get(gptr.teamid);
  if (team_data == NULL) {
    DART_LOG_ERROR("dart_gptr_getflags ! Unknown team %i", gptr.teamid);
    return DART_ERR_INVAL;
  }

  return dart_segment_get_flags(&team_data->segdata, gptr.segid, flags);
}

dart_ret_t dart_gptr_setflags(dart_gptr_t *gptr, uint16_t flags)
{
  dart_team_data_t *team_data = dart_adapt_teamlist_get(gptr->teamid);
  if (team_data == NULL) {
    DART_LOG_ERROR("dart_gptr_setflags ! Unknown team %i", gptr->teamid);
    return DART_ERR_INVAL;
  }

  dart_ret_t ret = dart_segment_set_flags(&team_data->segdata, gptr->segid, flags);

  if (ret != DART_OK) {
    return ret;
  }

  gptr->flags = (flags & 0xFF);
  return DART_OK;
}


dart_ret_t dart_memalloc(
  size_t            nelem,
  dart_datatype_t   dtype,
  dart_gptr_t     * gptr)
{
  size_t      nbytes = nelem * dart__mpi__datatype_sizeof(dtype);
  dart_global_unit_t unitid;
  dart_myid(&unitid);
  gptr->unitid  = unitid.id;
  gptr->flags   = 0;
  gptr->segid   = DART_SEGMENT_LOCAL; /* For local allocation, the segid is marked as '0'. */
  gptr->teamid  = DART_TEAM_ALL;      /* Locally allocated gptr belong to the global team. */
  gptr->addr_or_offs.offset = dart_buddy_alloc(dart_localpool, nbytes);
  if (gptr->addr_or_offs.offset == (uint64_t)(-1)) {
    DART_LOG_ERROR("dart_memalloc: Out of bounds "
                   "(dart_buddy_alloc %zu bytes): global memory exhausted",
                   nbytes);
    *gptr = DART_GPTR_NULL;
    return DART_ERR_OTHER;
  }
  DART_LOG_DEBUG("dart_memalloc: local alloc nbytes:%lu offset:%"PRIu64"",
                 nbytes, gptr->addr_or_offs.offset);
  return DART_OK;
}

dart_ret_t dart_memfree (dart_gptr_t gptr)
{
  if (gptr.segid != DART_SEGMENT_LOCAL || gptr.teamid != DART_TEAM_ALL) {
    DART_LOG_ERROR("dart_memfree: invalid segment id:%d or team id:%d",
                   gptr.segid, gptr.teamid);
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

/**
 * Check that the window support MPI_WIN_UNIFIED, print warning otherwise.
 */
void dart__mpi__check_memory_model(dart_segment_info_t *segment)
{
  int *mem_model, flag;
  MPI_Win_get_attr(segment->win, MPI_WIN_MODEL, &mem_model, &flag);

  DART_ASSERT_MSG(flag != 0, "Failed to query window memory model!");

  segment->sync_needed = false;
  if (*mem_model != MPI_WIN_UNIFIED) {
    static bool warning_printed = false;
    if (!warning_printed) {
      dart_global_unit_t myid;
      dart_myid(&myid);
      if (myid.id == 0) {
        DART_LOG_WARN(
          "The allocated MPI window does not support the unified memory model. "
        );
        DART_LOG_WARN(
          "DASH may not be able to guaranteed consistency of local and remote updates."
        );
        DART_LOG_WARN("USE AT YOUR OWN RISK!");
      }
      warning_printed = true;
    }
    segment->sync_needed = true;
  }
}

#ifdef DART_MPI_ENABLE_DYNAMIC_WINDOWS
static dart_ret_t
dart_team_memalloc_aligned_dynamic(
  dart_team_t       teamid,
  size_t            nelem,
  dart_datatype_t   dtype,
  dart_gptr_t     * gptr)
{
  char * sub_mem;
  dart_unit_t gptr_unitid = 0; // the team-local ID 0 has the beginning
  int         dtype_size  = dart__mpi__datatype_sizeof(dtype);
  MPI_Aint    nbytes      = nelem * dtype_size;
  size_t      team_size;
  MPI_Win     sharedmem_win = MPI_WIN_NULL;
  dart_team_size(teamid, &team_size);

  *gptr = DART_GPTR_NULL;

  DART_LOG_TRACE("dart_team_memalloc_aligned : dts:%i nelem:%zu nbytes:%zu",
    dtype_size, nelem, nbytes);


  dart_team_data_t *team_data = dart_adapt_teamlist_get(teamid);
  if (team_data == NULL) {
    DART_LOG_ERROR(
        "dart_team_memalloc_aligned_dynamic ! Unknown team %i", teamid);
    return DART_ERR_INVAL;
  }

  MPI_Comm  comm = team_data->comm;

  dart_segment_info_t *segment = dart_segment_alloc(
                                &team_data->segdata, DART_SEGMENT_ALLOC);

#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)

  char     ** baseptr_set = NULL;
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
   * Related support ticket of MPICH:
   * http://trac.mpich.org/projects/mpich/ticket/2178
   *
   * !!! BUG IN OPENMPI 1.10.5 and 2.0.2
   * !!!
   * !!! The alignment of the memory returned by MPI_Win_allocate_shared is not
   * !!! guaranteed to be natural, i.e., on 64b systems it can be only 4 byte
   * !!! if running with an odd number of processes.
   * !!! The issue has been reported.
   * !!!
   *
   */
  MPI_Comm sharedmem_comm = team_data->sharedmem_comm;

  DART_LOG_DEBUG("dart_team_memalloc_aligned: "
                 "MPI_Win_allocate_shared(nbytes:%ld)", nbytes);

  if (sharedmem_comm != MPI_COMM_NULL) {
    MPI_Info win_info;
    MPI_Info_create(&win_info);
    MPI_Info_set(win_info, "alloc_shared_noncontig", "true");

    int ret = MPI_Win_allocate_shared(
                nbytes,     // number of bytes
                dtype_size, // displacement unit
                win_info,
                sharedmem_comm,
                &sub_mem,
                &sharedmem_win);
    MPI_Info_free(&win_info);
    if (ret != MPI_SUCCESS) {
      DART_LOG_ERROR("dart_team_memalloc_aligned_dynamic: "
                     "MPI_Win_allocate_shared failed, error %d (%s)",
                     ret, DART__MPI__ERROR_STR(ret));
      dart_segment_free(&team_data->segdata, segment->segid);
      return DART_ERR_OTHER;
    }
  } else {
    DART_LOG_ERROR("dart_team_memalloc_aligned_dynamic: "
                   "Shared memory communicator is MPI_COMM_NULL, "
                   "cannot call MPI_Win_allocate_shared");
    dart_segment_free(&team_data->segdata, segment->segid);
    return DART_ERR_OTHER;
  }

  MPI_Aint winseg_size;
  int      sharedmem_unitid;
  char *   baseptr;
  int      disp_unit, i;
  MPI_Comm_rank(sharedmem_comm, &sharedmem_unitid);
  // re-use previously allocated memory
  if (segment->baseptr == NULL) {
    segment->baseptr = calloc(team_data->sharedmem_nodesize, sizeof(char *));
  }
  baseptr_set = segment->baseptr;

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
      "dart_team_memalloc_aligned_dynamic: bytes:%lu MPI_Alloc_mem failed",
      nbytes);
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
        "dart_team_memalloc_aligned_dynamic: bytes:%lu MPI_Win_attach failed",
        nbytes);
      dart_segment_free(&team_data->segdata, segment->segid);
      return DART_ERR_OTHER;
    }

    if (MPI_Get_address(sub_mem, &disp) != MPI_SUCCESS) {
      DART_LOG_ERROR(
        "dart_team_memalloc_aligned_dynamic: bytes:%lu MPI_Get_address failed",
        nbytes);
      dart_segment_free(&team_data->segdata, segment->segid);
      return DART_ERR_OTHER;
    }
  } else {
    disp = 0;
  }

  // re-use previously allocated memory
  if (segment->disp == NULL) {
    segment->disp = malloc(team_size * sizeof (MPI_Aint));
  }
  MPI_Aint * disp_set = segment->disp;
  /* Collect the disp information from all the ranks in comm */
  MPI_Allgather(&disp, 1, MPI_AINT, disp_set, 1, MPI_AINT, comm);


  /* Updating the translation table of teamid with the created
   * (offset, win) infos */
  if (segment == NULL) {
    DART_LOG_ERROR(
        "dart_team_memalloc_aligned_dynamic: "
        "bytes:%lu Allocation of segment data failed", nbytes);
    dart_segment_free(&team_data->segdata, segment->segid);
    return DART_ERR_OTHER;
  }
  segment->size    = nbytes;
  segment->flags   = 0;
  segment->shmwin  = sharedmem_win;
  segment->win     = team_data->window;
  segment->selfbaseptr = sub_mem;
  segment->is_dynamic  = true;
  /**
   * Following the example 11.21 in the MPI standard v3.1, a sync is necessary
   * even in the unified memory model if load/stores are used in shared memory.
   */
  segment->sync_needed = true;

  /* -- Updating infos on gptr -- */
  /* Segid equals to dart_memid (always a positive integer), identifies an
   * unique collective global memory. */
  gptr->segid  = segment->segid;
  gptr->unitid = gptr_unitid;
  gptr->teamid = teamid;
  gptr->flags  = 0;
  gptr->addr_or_offs.offset = 0;


  DART_LOG_DEBUG(
    "dart_team_memalloc_aligned_dynamic: bytes:%lu gptr_unitid:%d "
    "baseptr:%p segid:%i across team %d",
    nbytes, gptr_unitid, sub_mem, segment->segid, teamid);

  return DART_OK;
}

#else // DART_MPI_ENABLE_DYNAMIC_WINDOWS

static dart_ret_t
dart_team_memalloc_aligned_full(
  dart_team_t       teamid,
  size_t            nelem,
  dart_datatype_t   dtype,
  dart_gptr_t     * gptr)
{
  char *baseptr;
  MPI_Win win;
  dart_unit_t gptr_unitid = 0; // the team-local ID 0 has the beginning
  int         dtype_size  = dart__mpi__datatype_sizeof(dtype);
  MPI_Aint    nbytes      = nelem * dtype_size;
  *gptr = DART_GPTR_NULL;

  DART_LOG_TRACE(
    "dart_team_memalloc_aligned_full : dts:%i nelem:%zu nbytes:%zu",
    dtype_size, nelem, nbytes);

  dart_team_data_t *team_data = dart_adapt_teamlist_get(teamid);
  if (team_data == NULL) {
    DART_LOG_ERROR("dart_team_memalloc_aligned_full ! Unknown team %i", teamid);
    return DART_ERR_INVAL;
  }

  dart_segment_info_t *segment = dart_segment_alloc(
                                &team_data->segdata, DART_SEGMENT_ALLOC);

  MPI_Info win_info;
  MPI_Info_create(&win_info);
  MPI_Info_set(win_info, "same_disp_unit", "true");
  if (MPI_Win_allocate(
      nbytes, 1, win_info,
      team_data->comm, &baseptr, &win) != MPI_SUCCESS) {
    DART_LOG_ERROR("dart_team_memalloc_aligned_full: MPI_Win_allocate failed");
    MPI_Info_free(&win_info);
    return DART_ERR_OTHER;
  }
  MPI_Info_free(&win_info);

  if (MPI_Win_lock_all(MPI_MODE_NOCHECK, win) != MPI_SUCCESS) {
    DART_LOG_ERROR("dart_team_memalloc_aligned_full: MPI_Win_lock_all failed");
    return DART_ERR_OTHER;
  }

  if (segment->baseptr != NULL) {
    free(segment->baseptr);
    segment->baseptr = NULL;
  }

  if (segment->disp != NULL) {
    free(segment->disp);
    segment->disp = NULL;
  }

  segment->flags       = 0;
  segment->selfbaseptr = baseptr;
  segment->size        = nbytes;
  segment->shmwin      = MPI_WIN_NULL;
  segment->win         = win;
  segment->is_dynamic  = false;

  dart__mpi__check_memory_model(segment);

  gptr->segid  = segment->segid;
  gptr->unitid = gptr_unitid;
  gptr->teamid = teamid;
  gptr->flags  = 0;
  gptr->addr_or_offs.offset = 0;

  return DART_OK;
}
#endif // DART_MPI_ENABLE_DYNAMIC_WINDOWS

dart_ret_t
dart_team_memalloc_aligned(
  dart_team_t       teamid,
  size_t            nelem,
  dart_datatype_t   dtype,
  dart_gptr_t     * gptr)
{
  CHECK_IS_BASICTYPE(dtype);
#ifdef DART_MPI_ENABLE_DYNAMIC_WINDOWS
  return dart_team_memalloc_aligned_dynamic(teamid, nelem, dtype, gptr);
#else
  return dart_team_memalloc_aligned_full(teamid, nelem, dtype, gptr);
#endif
}

dart_ret_t dart_team_memfree(
  dart_gptr_t gptr)
{
  int16_t segid = gptr.segid;
  char  * sub_mem = NULL;
  dart_team_t teamid = gptr.teamid;

  if (DART_GPTR_ISNULL(gptr)) {
    /* corresponds to free(NULL) which is a valid operation */
    return DART_OK;
  }

  dart_team_data_t *team_data = dart_adapt_teamlist_get(teamid);
  if (team_data == NULL) {
    DART_LOG_ERROR("dart_team_memfree ! failed: Unknown team %i!", teamid);
    return DART_ERR_INVAL;
  }

  dart_segment_info_t *seginfo = dart_segment_get_info(
                                    &(team_data->segdata), segid);
  if (seginfo == NULL) {
    DART_LOG_ERROR("dart_team_memfree ! "
                   "Unknown segment %i on team %i", segid, teamid);
    return DART_ERR_INVAL;
  }

  if (seginfo->is_dynamic) {
    MPI_Win win = team_data->window;
    if (dart_segment_get_selfbaseptr(
          &team_data->segdata, segid, &sub_mem) != DART_OK) {
      return DART_ERR_INVAL;
    }
    /* Detach the window associated with sub-memory to be freed */
    if (sub_mem != NULL) {
      MPI_Win_detach(win, sub_mem);
    }

	/* Free the window's associated sub-memory */
#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
    MPI_Win sharedmem_win;
    if (dart_segment_get_shmwin(
          &team_data->segdata,
          segid,
          &sharedmem_win) != DART_OK) {
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
  } else {
    // full allocation
    if (MPI_Win_unlock_all(seginfo->win) != MPI_SUCCESS) {
      DART_LOG_ERROR("dart_team_memfree: MPI_Win_unlock_all failed");
      return DART_ERR_OTHER;
    }
    if (MPI_Win_free(&seginfo->win) != MPI_SUCCESS) {
      DART_LOG_ERROR("dart_team_memfree: MPI_Win_free failed");
      return DART_ERR_OTHER;
    }
  }


#if defined(DART_ENABLE_LOGGING)
  dart_team_unit_t unitid;
  dart_team_myid(teamid, &unitid);
#endif
  DART_LOG_DEBUG("dart_team_memfree: collective free, team unit id: %2d "
                 "offset:%"PRIu64", segid=%d, baseptr=%p, gptr_unitid:%d across team %d",
                 unitid.id, gptr.addr_or_offs.offset, segid, sub_mem, gptr.unitid, teamid);
  /* Remove the related correspondence relation record from the related
   * translation table. */
  if (dart_segment_free(&team_data->segdata, segid) != DART_OK) {
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
  CHECK_IS_BASICTYPE(dtype);
  size_t   size;
  int      dtype_size = dart__mpi__datatype_sizeof(dtype);
  size_t   nbytes     = nelem * dtype_size;
  MPI_Aint disp;
  dart_unit_t gptr_unitid = 0;
  dart_team_size(teamid, &size);

  *gptr = DART_GPTR_NULL;

  dart_team_data_t *team_data = dart_adapt_teamlist_get(teamid);
  if (team_data == NULL) {
    DART_LOG_ERROR("dart_team_memregister_aligned ! failed: Unknown team %i!",
                   teamid);
    return DART_ERR_INVAL;
  }

  dart_segment_info_t *segment = dart_segment_alloc(
                                &team_data->segdata, DART_SEGMENT_REGISTER);
  if (segment == NULL) {
    DART_LOG_ERROR(
        "dart_team_memalloc_aligned: bytes:%lu Allocation of segment data failed",
        nbytes);
    return DART_ERR_OTHER;
  }

  if (segment->disp == NULL) {
    segment->disp = malloc(size * sizeof(MPI_Aint));
  }
  MPI_Aint * disp_set = segment->disp;

  MPI_Comm comm = team_data->comm;
  MPI_Win win = team_data->window;
  MPI_Win_attach(win, addr, nbytes);
  MPI_Get_address(addr, &disp);
  MPI_Allgather(&disp, 1, MPI_AINT, disp_set, 1, MPI_AINT, comm);

  segment->size    = nbytes;
  segment->shmwin  = MPI_WIN_NULL;
  segment->win     = team_data->window;
  segment->selfbaseptr = (char *)addr;
  segment->flags   = 0;

  dart__mpi__check_memory_model(segment);

  gptr->unitid = gptr_unitid;
  gptr->segid  = segment->segid;
  gptr->teamid = teamid;
  gptr->flags  = 0;
  gptr->addr_or_offs.offset = 0;

#if defined(DART_ENABLE_LOGGING)
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
  CHECK_IS_BASICTYPE(dtype);
  int    nil;
  size_t size;
  int    dtype_size = dart__mpi__datatype_sizeof(dtype);
  size_t nbytes     = nelem * dtype_size;
  dart_unit_t gptr_unitid = 0;
  dart_team_size(teamid, &size);

  *gptr = DART_GPTR_NULL;

  if (nbytes == 0) {
    // Attaching empty memory region, set sendbuf to valid dummy pointer:
    addr = (void*)(&nil);
  }

  dart_team_data_t *team_data = dart_adapt_teamlist_get(teamid);
  if (team_data == NULL) {
    DART_LOG_ERROR("dart_team_memregister ! failed: Unknown team %i!", teamid);
    return DART_ERR_INVAL;
  }

  dart_segment_info_t *segment = dart_segment_alloc(
                                &team_data->segdata, DART_SEGMENT_REGISTER);
  if (segment == NULL) {
    DART_LOG_ERROR(
        "dart_team_memalloc_aligned: bytes:%lu Allocation of segment data failed",
        nbytes);
    return DART_ERR_OTHER;
  }

  MPI_Aint   disp;
  if (segment->disp == NULL) {
    segment->disp = malloc(size * sizeof(MPI_Aint));
  }
  MPI_Aint * disp_set = segment->disp;
  MPI_Comm   comm     = team_data->comm;
  MPI_Win    win      = team_data->window;
  MPI_Win_attach(win, addr, nbytes);
  MPI_Get_address(addr, &disp);
  MPI_Allgather(&disp, 1, MPI_AINT, disp_set, 1, MPI_AINT, comm);

  segment->size   = nbytes;
  segment->shmwin = MPI_WIN_NULL;
  segment->win    = team_data->window;
  segment->selfbaseptr = (char *)addr;
  segment->flags = 0;

  dart__mpi__check_memory_model(segment);

  gptr->unitid = gptr_unitid;
  gptr->segid  = segment->segid;
  gptr->teamid = teamid;
  gptr->flags  = 0;
  gptr->addr_or_offs.offset = 0;

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
  int16_t segid = gptr.segid;
  char  * sub_mem;
  MPI_Win win;
  dart_team_t teamid = gptr.teamid;

  if (DART_GPTR_ISNULL(gptr)) {
    /* corresponds to free(NULL) which is a valid operation */
    return DART_OK;
  }

  dart_team_data_t *team_data = dart_adapt_teamlist_get(gptr.teamid);
  if (team_data == NULL) {
    DART_LOG_ERROR("dart_team_memderegister ! failed: Unknown team %i!", teamid);
    return DART_ERR_INVAL;
  }

  win = team_data->window;

  if (dart_segment_get_selfbaseptr(
        &team_data->segdata, segid, &sub_mem) != DART_OK) {
    DART_LOG_ERROR("dart_team_memderegister ! Unknown segment %i", segid);
    return DART_ERR_INVAL;
  }

  MPI_Win_detach(win, sub_mem);
  if (dart_segment_free(&team_data->segdata, segid) != DART_OK) {
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
