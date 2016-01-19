/**
 * @file dart_communication.c
 * @date 25 Aug 2014
 * @brief Implementations of all the dart communication operations.
 *
 * All the following functions are implemented with the underling *MPI-3*
 * one-sided runtime system.
 */


#include <stdio.h>
#include <mpi.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/base/math.h>
#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_initialization.h>
#include <dash/dart/if/dart_globmem.h>
#include <dash/dart/if/dart_team_group.h>
#include <dash/dart/if/dart_communication.h>
#include <dash/dart/mpi/dart_communication_priv.h>
#include <dash/dart/mpi/dart_translation.h>
#include <dash/dart/mpi/dart_team_private.h>
#include <dash/dart/mpi/dart_mem.h>

int unit_g2l(
  uint16_t index,
  dart_unit_t abs_id,
  dart_unit_t *rel_id)
{
  if (index == 0) {
    *rel_id = abs_id;
  }
  else {
    MPI_Comm comm;
    MPI_Group group, group_all;
    comm = dart_teams[index];
    MPI_Comm_group (comm, &group);
    MPI_Comm_group (MPI_COMM_WORLD, &group_all);
    MPI_Group_translate_ranks (group_all, 1, &abs_id, group, rel_id);
  }
  return 0;
}

dart_ret_t dart_get(
  void        * dest,
  dart_gptr_t   gptr,
  size_t        nbytes)
{
  MPI_Aint    disp_s,
              disp_rel;
  MPI_Win     win;
  dart_unit_t target_unitid_abs = gptr.unitid;
  uint64_t    offset            = gptr.addr_or_offs.offset;
  int16_t     seg_id            = gptr.segid;

  DART_LOG_DEBUG("dart_get() nbytes:%lld s:%d o:%lld u:%d",
                 nbytes, seg_id, offset, target_unitid_abs);
  if (seg_id) {
    uint16_t    index = gptr.flags;
    win               = dart_win_lists[index];
    dart_unit_t target_unitid_rel;

    DART_LOG_TRACE("dart_get:  index:%d win:%d",
                   index, win);
    unit_g2l(index,
             target_unitid_abs,
             &target_unitid_rel);
    DART_LOG_TRACE("dart_get:  relative unit id: %d",
                   target_unitid_rel);
    if (dart_adapt_transtable_get_disp(
          seg_id,
          target_unitid_rel,
          &disp_s) == -1) {
      return DART_ERR_INVAL;
    }
    disp_rel = disp_s + offset;
    DART_LOG_TRACE("dart_get:  nbytes:%lld "
                   "source (collect.): win:%d unit:%d disp:%lld -> dest: %p",
                   nbytes, win, target_unitid_rel, disp_rel, dest);
    DART_LOG_TRACE("dart_get:  MPI_Get");
    MPI_Get(
      dest,
      nbytes,
      MPI_BYTE,
      target_unitid_rel,
      disp_rel,
      nbytes,
      MPI_BYTE,
      win);
  } else {
    win = dart_win_local_alloc;
    DART_LOG_TRACE("dart_get:  nbytes:%lld "
                   "source (local): win:%d unit:%d offset:%lld -> dest: %p",
                   nbytes, win, target_unitid_abs, offset, dest);
    DART_LOG_TRACE("dart_get:  MPI_Get");
    MPI_Get(
      dest,
      nbytes,
      MPI_BYTE,
      target_unitid_abs,
      offset,
      nbytes,
      MPI_BYTE,
      win);
  }
  DART_LOG_DEBUG("dart_get > finished");
  return DART_OK;
}

dart_ret_t dart_put(
  dart_gptr_t  gptr,
  const void * src,
  size_t       nbytes)
{
  MPI_Aint    disp_s,
              disp_rel;
  MPI_Win     win;
  dart_unit_t target_unitid_abs;
  uint64_t offset   = gptr.addr_or_offs.offset;
  int16_t  seg_id   = gptr.segid;
  target_unitid_abs = gptr.unitid;
  if (seg_id) {
    uint16_t index = gptr.flags;
    dart_unit_t target_unitid_rel;
    win = dart_win_lists[index];
    unit_g2l (index, target_unitid_abs, &target_unitid_rel);
    if (dart_adapt_transtable_get_disp(
          seg_id,
          target_unitid_rel,
          &disp_s) == -1) {
      return DART_ERR_INVAL;
    }
    disp_rel = disp_s + offset;
    MPI_Put(
      src,
      nbytes,
      MPI_BYTE,
      target_unitid_rel,
      disp_rel,
      nbytes,
      MPI_BYTE,
      win);
    DART_LOG_DEBUG ("PUT  -%d bytes (allocated with collective allocation) "
           "to %d at the offset %d",
           nbytes, target_unitid_abs, offset);
  } else {
    win = dart_win_local_alloc;
    MPI_Put(
      src,
      nbytes,
      MPI_BYTE,
      target_unitid_abs,
      offset,
      nbytes,
      MPI_BYTE,
      win);
    DART_LOG_DEBUG ("PUT  - %d bytes (allocated with local allocation) "
           "to %d at the offset %d",
           nbytes, target_unitid_abs, offset);
  }
  return DART_OK;
}

/*
 * TODO: Define and use macro DART__DEFINE__ACCUMULATE(int)
 */
dart_ret_t dart_accumulate_int(
  dart_gptr_t gptr,
  int *values,
  size_t nelem,
  dart_operation_t op,
  dart_team_t team)
{
  MPI_Aint    disp_s,
              disp_rel;
  MPI_Win     win;
  dart_unit_t target_unitid_abs;
  uint64_t offset   = gptr.addr_or_offs.offset;
  int16_t  seg_id   = gptr.segid;
  target_unitid_abs = gptr.unitid;
  if (seg_id) {
    dart_unit_t target_unitid_rel;
    uint16_t index = gptr.flags;
    win            = dart_win_lists[index];
    unit_g2l(index,
             target_unitid_abs,
             &target_unitid_rel);
    if (dart_adapt_transtable_get_disp(
          seg_id,
          target_unitid_rel,
          &disp_s) == -1) {
      return DART_ERR_INVAL;
    }
    disp_rel = disp_s + offset;
    MPI_Accumulate(
      values,            // Origin address
      nelem,             // Number of entries in buffer
      MPI_INT,           // Data type of each buffer entry
      target_unitid_rel, // Rank of target
      disp_rel,          // Displacement from start of window to beginning
                         // of target buffer
      nelem,             // Number of entries in target buffer
      MPI_INT,           // Data type of each entry in target buffer
      dart_mpi_op(op),   // Reduce operation
      win);
    DART_LOG_DEBUG ("ACC  - %d elements (allocated with collective allocation) "
           "to %d at offset %d",
           nelem, target_unitid_abs, offset);
  } else {
    win = dart_win_local_alloc;
    MPI_Accumulate(
      values,            // Origin address
      nelem,             // Number of entries in buffer
      MPI_INT,           // Data type of each buffer entry
      target_unitid_abs, // Rank of target
      offset,            // Displacement from start of window to beginning
                         // of target buffer
      nelem,             // Number of entries in target buffer
      MPI_INT,           // Data type of each entry in target buffer
      dart_mpi_op(op),   // Reduce operation
      win);
    DART_LOG_DEBUG ("ACC  - %d elements (allocated with local allocation) "
           "to %d at offset %d",
           nelem, target_unitid_abs, offset);
  }
  return DART_OK;
}

/* -- Non-blocking dart one-sided operations -- */

dart_ret_t dart_get_handle(
  void          * dest,
  dart_gptr_t     gptr,
  size_t          nbytes,
  dart_handle_t * handle)
{
  MPI_Request mpi_req;
  MPI_Aint    disp_s,
              disp_rel;
  dart_unit_t target_unitid_abs,
              target_unitid_rel;
  MPI_Win     win;
  int         mpi_ret;
  uint64_t    offset = gptr.addr_or_offs.offset;
  uint16_t    index  = gptr.flags;
  int16_t     seg_id = gptr.segid;
  /* MPI uses offset type int, do not copy more than INT_MAX elements: */
  int n_count = (int)(nbytes);
  if (nbytes > INT_MAX) {
    DART_LOG_ERROR("dart_get_handle > failed: nbytes > INT_MAX");
    return DART_ERR_INVAL;
  }

  *handle = (dart_handle_t) malloc(sizeof(struct dart_handle_struct));
  target_unitid_abs = gptr.unitid;

  DART_LOG_DEBUG("dart_get_handle() uid_abs:%d o:%lld s:%d i:%d, bytes:%lld",
                 target_unitid_abs, offset, seg_id, index, nbytes);
  DART_LOG_TRACE("dart_get_handle:  allocated handle:%p", (void *)(*handle));

  /* The memory accessed is allocated with collective allocation. */
  if (seg_id) {
    DART_LOG_TRACE("dart_get_handle:  collective, segment:%d", seg_id);
/*
    if (dart_adapt_transtable_get_win (index, offset, &begin, &win) == -1)
    {
      DART_LOG_ERROR ("Invalid accessing operation");
      return DART_ERR_INVAL;
    }
*/
    win = dart_win_lists[index];
    /* Translate local unitID (relative to teamid) into global unitID
     * (relative to DART_TEAM_ALL).
     *
     * Note: target_unitid should not be the global unitID but rather the
     * local unitID relative to the team associated with the specified win
     * object.
     */
    unit_g2l(index, target_unitid_abs, &target_unitid_rel);
    DART_LOG_DEBUG("dart_get_handle:  -- uid_rel:%d", target_unitid_rel);

    if (dart_adapt_transtable_get_disp(
          seg_id,
          target_unitid_rel,
          &disp_s) == -1)
    {
      DART_LOG_ERROR(
        "dart_get_handle > dart_adapt_transtable_get_disp failed");
      free(*handle);
      return DART_ERR_INVAL;
    }
    disp_rel = disp_s + offset;
    DART_LOG_TRACE("dart_get_handle:  -- disp_s:%lld disp_rel:%lld",
                   disp_s, disp_rel);

    /* MPI-3 newly added feature: request version of get call. */
    /**
     * TODO: Check if
     *    MPI_Rget_accumulate(
     *      NULL, 0, MPI_BYTE, dest, nbytes, MPI_BYTE,
     *      target_unitid, disp_rel, nbytes, MPI_BYTE, MPI_NO_OP, win,
     *      &mpi_req)
     *  ... could be an better alternative?
     */
    DART_LOG_DEBUG("dart_get_handle:  -- %d bytes (collective allocation) "
                   "from %d at offset %d",
                   n_count, target_unitid_rel, offset);
    DART_LOG_DEBUG("dart_get_handle:  -- MPI_Rget");
    mpi_ret = MPI_Rget(
                dest,
                n_count,
                MPI_BYTE,
                target_unitid_rel,
                disp_rel,
                n_count,
                MPI_BYTE,
                win,
                &mpi_req);
    if (mpi_ret != MPI_SUCCESS) {
      DART_LOG_ERROR("dart_get_handle > MPI_Rget failed");
      free(*handle);
      return DART_ERR_INVAL;
    }
    (*handle)->dest = target_unitid_rel;
  } else {
    /* The memory accessed is allocated with local allocation. */
    DART_LOG_TRACE("dart_get_handle:  -- local, segment:%d", seg_id);
    DART_LOG_DEBUG("dart_get_handle:  -- %d bytes (local allocation) "
                   "from %d at offset %d",
                   n_count, target_unitid_abs, offset);
    win     = dart_win_local_alloc;
    DART_LOG_DEBUG("dart_get_handle:  -- MPI_Rget");
    mpi_ret = MPI_Rget(
                dest,
                n_count,
                MPI_BYTE,
                target_unitid_abs,
                offset,
                n_count,
                MPI_BYTE,
                win,
                &mpi_req);
    if (mpi_ret != MPI_SUCCESS) {
      DART_LOG_ERROR("dart_get_handle > MPI_Rget failed");
      free(*handle);
      return DART_ERR_INVAL;
    }
    (*handle)->dest = target_unitid_abs;
  }
  (*handle)->request = mpi_req;
  (*handle)->win     = win;
  DART_LOG_TRACE("dart_get_handle > handle(%p) dest:%d win:%d req:%d",
                 *handle, (*handle)->dest, win, mpi_req);
  return DART_OK;
}

dart_ret_t dart_put_handle(
  dart_gptr_t  gptr,
  const void * src,
  size_t       nbytes,
  dart_handle_t *handle)
{
  MPI_Request mpi_req;
  MPI_Aint disp_s, disp_rel;
  dart_unit_t target_unitid_abs;
  uint64_t offset = gptr.addr_or_offs.offset;
  int16_t seg_id = gptr.segid;
  MPI_Win win;

  *handle = (dart_handle_t) malloc(sizeof(struct dart_handle_struct));
  target_unitid_abs = gptr.unitid;

  if (seg_id) {
    uint16_t index = gptr.flags;
    dart_unit_t target_unitid_rel;
/*
    if (dart_adapt_transtable_get_win(index, offset, &begin, &win) == -1) {
      DART_LOG_ERROR ("Invalid accessing operation");
      return DART_ERR_INVAL;
    }
*/
    win = dart_win_lists[index];
    unit_g2l (index, target_unitid_abs, &target_unitid_rel);
    if (dart_adapt_transtable_get_disp(
          seg_id,
          target_unitid_rel,
          &disp_s) == -1) {
      return DART_ERR_INVAL;
    }
    disp_rel = disp_s + offset;
    /**
     * TODO: Check if
     *   MPI_Raccumulate(
     *     src, nbytes, MPI_BYTE, target_unitid,
     *     disp_rel, nbytes, MPI_BYTE,
     *     REPLACE, win, &mpi_req)
     * ... could be a better alternative?
     */
    DART_LOG_DEBUG("dart_put_blocking: MPI_RPut");
    MPI_Rput(
      src,
      nbytes,
      MPI_BYTE,
      target_unitid_rel,
      disp_rel,
      nbytes,
      MPI_BYTE,
      win,
      &mpi_req);
    (*handle) -> dest = target_unitid_rel;
    DART_LOG_DEBUG("PUT  -%d bytes (allocated with collective allocation) "
          "to %d at the offset %d",
          nbytes, target_unitid_abs, offset);
  } else {
    DART_LOG_DEBUG("dart_put_blocking: MPI_RPut");
    win = dart_win_local_alloc;
    MPI_Rput(
      src,
      nbytes,
      MPI_BYTE,
      target_unitid_abs,
      offset,
      nbytes,
      MPI_BYTE,
      win,
      &mpi_req);
    DART_LOG_DEBUG("PUT  - %d bytes (allocated with local allocation) "
          "to %d at the offset %d",
          nbytes, target_unitid_abs, offset);
    (*handle) -> dest = target_unitid_abs;
  }
  (*handle) -> request = mpi_req;
  (*handle) -> win     = win;
  return DART_OK;
}

/* -- Blocking dart one-sided operations -- */

/**
 * TODO: Check if MPI_Get_accumulate (MPI_NO_OP) can bring better
 * performance?
 */

dart_ret_t dart_put_blocking(
  dart_gptr_t  gptr,
  const void * src,
  size_t       nbytes)
{
  MPI_Win win;
  MPI_Aint disp_s, disp_rel;

  uint64_t    offset = gptr.addr_or_offs.offset;
  int16_t     seg_id = gptr.segid;
  uint16_t    index  = gptr.flags;
  dart_unit_t unitid,
              target_unitid_rel,
              target_unitid_abs = gptr.unitid;
  DART_LOG_DEBUG("dart_put_blocking: gptr dest: "
                 "unitid: %d segid:%d offset:%d flags:%d",
                 target_unitid_abs, seg_id, offset, index);
  DART_LOG_DEBUG("dart_put_blocking: nbytes: %d", nbytes);

#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
  if (seg_id >= 0) {
    int i, is_sharedmem = 0;
    MPI_Aint maximum_size;
    int disp_unit;
    char* baseptr;

    /* Checking whether origin and target are in the same node.
     * We use the approach of shared memory accessing only when it passed
     * the above check.
     */
  //  i = binary_search (
  //        dart_unit_mapping[j],
  //        gptr.unitid,
  //        0,
  //        dart_sharedmem_size[j] - 1);
    /* The value of i will be the target's relative ID in teamid. */
    i = dart_sharedmem_table[index][gptr.unitid];

    if (i >= 0)  {
      is_sharedmem = 1;
    }
    if (is_sharedmem) {
      DART_LOG_DEBUG("dart_put_blocking: shared memory segment, seg_id:%d",
                     seg_id);
      if (seg_id) {
        if (dart_adapt_transtable_get_baseptr(seg_id, i, &baseptr) == -1) {
          return DART_ERR_INVAL;
        }
      } else {
        baseptr = dart_sharedmem_local_baseptr_set[i];
      }
      disp_rel = offset;
      baseptr  = baseptr + disp_rel;
      memcpy(baseptr, ((char*)src), nbytes);
      return DART_OK;
    }
  }

#if 0
    if (unitid == target_unitid_abs) {
      /* If orgin and target are identical, then switches to local
       * access.
       */
      if (seg_id) {
        int flag;
        MPI_Win_get_attr (win, MPI_WIN_BASE, &baseptr, &flag);
        baseptr = baseptr + offset;
      }  else {
        baseptr = offset + dart_mempool_localalloc;
      }
    } else {
      /* Accesses through shared memory (store). */
      disp_rel = offset;
      MPI_Win_shared_query(
        win,
        i,
        &maximum_size,
        &disp_unit,
        &baseptr);
      baseptr += disp_rel;
    }
    memcpy (baseptr, ((char*)src), nbytes);
#endif
#endif
  {
    /* The traditional remote access method */
    if (seg_id)  {
      win = dart_win_lists[index];
      unit_g2l (index, target_unitid_abs, &target_unitid_rel);
      if (dart_adapt_transtable_get_disp(
            seg_id,
            target_unitid_rel,
            &disp_s) == -1)  {
        return DART_ERR_INVAL;
      }
      disp_rel = disp_s + offset;
    }  else{
      win = dart_win_local_alloc;
      disp_rel = offset;
      target_unitid_rel = target_unitid_abs;
    }
    DART_LOG_DEBUG("dart_put_blocking: MPI_Put");
    MPI_Put(
      src,
      nbytes,
      MPI_BYTE,
      target_unitid_rel,
      disp_rel,
      nbytes,
      MPI_BYTE,
      win);
    /* Make sure the access is completed remotedly */
    DART_LOG_DEBUG("dart_put_blocking: MPI_Win_flush");
    MPI_Win_flush(target_unitid_rel, win);
    /* MPI_Wait is invoked to release the resource brought by the mpi
     * request handle
     */
    if (seg_id) {
      DART_LOG_DEBUG(
        "dart_put_blocking: %d bytes "
        "(allocated with collective allocation) to %d at the offset %d",
        nbytes, target_unitid_abs, offset);
    } else {
      DART_LOG_DEBUG(
        "dart_put_blocking: %d bytes "
        "(allocated with local allocation) to %d at the offset %d",
        nbytes, target_unitid_abs, offset);
    }
    return DART_OK;
  }
}

/**
 * TODO: Check if MPI_Accumulate (REPLACE) can bring better performance?
 */
dart_ret_t dart_get_blocking(
  void        * dest,
  dart_gptr_t   gptr,
  size_t        nbytes)
{
  MPI_Win win;
  MPI_Status mpi_sta;
  MPI_Request mpi_req;
  MPI_Aint disp_s, disp_rel;

  uint64_t    offset = gptr.addr_or_offs.offset;
  int16_t     seg_id = gptr.segid;
  uint16_t    index  = gptr.flags;
  dart_unit_t unitid,
              target_unitid_rel,
              target_unitid_abs = gptr.unitid;
  DART_LOG_DEBUG("dart_get_blocking: gptr source: "
                 "unitid: %d segid:%d offset:%d flags:%d",
                 target_unitid_abs, seg_id, offset, index);
  DART_LOG_DEBUG("dart_get_blocking: nbytes: %d", nbytes);

#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
  DART_LOG_DEBUG("dart_get_blocking: shared windows enabled");
  if (seg_id >= 0) {
    int       i,
              is_sharedmem = 0;
    MPI_Aint  maximum_size;
    int       disp_unit;
    char*     baseptr;

//  i = binary_search(
//        dart_unit_mapping[j],
//        gptr.unitid,
//        0,
//        dart_sharedmem_size[j] - 1);

    /* Check whether the target is in the same node as the calling unit
     * or not.
     */
    i = dart_sharedmem_table[index][gptr.unitid];
    if (i >= 0) {
      is_sharedmem = 1;
    }
    if (is_sharedmem) {
      DART_LOG_DEBUG("dart_get_blocking: shared memory segment, seg_id:%d",
                     seg_id);
      if (seg_id) {
        if (dart_adapt_transtable_get_baseptr(seg_id, i, &baseptr) == -1) {
          return DART_ERR_INVAL;
        }
      } else {
        baseptr = dart_sharedmem_local_baseptr_set[i];
      }
      disp_rel = offset;
      baseptr += disp_rel;
      DART_LOG_DEBUG("dart_get_blocking: memcpy %d bytes", nbytes);
      memcpy((char*)dest, baseptr, nbytes);
      return DART_OK;
    }
  }

#  if 0
     if (unitid == target_unitid_abs) {
       if (seg_id) {
         int flag;
         MPI_Win_get_attr(win, MPI_WIN_BASE, &baseptr, &flag);
         baseptr = baseptr + offset;
       } else {
         baseptr = offset + dart_mempool_localalloc;
       }
     } else {
       /* Accesses through shared memory (load)*/
       disp_rel = offset;
       MPI_Win_shared_query(win, i, &maximum_size, &disp_unit, &baseptr);
       baseptr += disp_rel;
     }
     memcpy((char*)dest, baseptr, nbytes);
#  endif
#else
  DART_LOG_DEBUG("dart_get_blocking: shared windows disabled");
#endif // !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
  {
    if (seg_id) {
      win = dart_win_lists[index];
      unit_g2l(index, target_unitid_abs, &target_unitid_rel);
      if (dart_adapt_transtable_get_disp(
            seg_id,
            target_unitid_rel,
            &disp_s) == -1) {
        return DART_ERR_INVAL;
      }
      disp_rel = disp_s + offset;
    } else {
      win               = dart_win_local_alloc;
      disp_rel          = offset;
      target_unitid_rel = target_unitid_abs;
    }
    DART_LOG_DEBUG("dart_get_blocking: MPI_RGet");
    MPI_Rget(
      dest,
      nbytes,
      MPI_BYTE,
      target_unitid_rel,
      disp_rel,
      nbytes,
      MPI_BYTE,
      win,
      &mpi_req);
    DART_LOG_DEBUG("dart_get_blocking: MPI_Wait");
    MPI_Wait(&mpi_req, &mpi_sta);

    if (seg_id) {
      DART_LOG_DEBUG("dart_get_blocking: %d bytes "
                     "(allocated with collective allocation) from %d "
                     "at offset %d",
                     nbytes, target_unitid_abs, offset);
    } else {
      DART_LOG_DEBUG("dart_get_blocking: %d bytes "
                     "(allocated with local allocation) from %d "
                     "at offset %d",
                     nbytes, target_unitid_abs, offset);
    }
    return DART_OK;
  }
}

/* -- Dart RMA Synchronization Operations -- */

dart_ret_t dart_flush(
  dart_gptr_t gptr)
{
  MPI_Win     win;
  dart_unit_t target_unitid_abs;
  int16_t     seg_id = gptr.segid;
  target_unitid_abs  = gptr.unitid;
  if (seg_id) {
    dart_unit_t target_unitid_rel;
    uint16_t    index = gptr.flags;
    win               = dart_win_lists[index];
    DART_LOG_DEBUG("dart_flush() win:%d seg:%d unit:%d",
                   win, seg_id, target_unitid_abs);
    unit_g2l(index, target_unitid_abs, &target_unitid_rel);
    DART_LOG_TRACE("dart_flush: MPI_Win_flush");
    MPI_Win_flush(target_unitid_rel, win);
  } else {
    win = dart_win_local_alloc;
    DART_LOG_DEBUG("dart_flush() lwin:%p seg:%d unit:%d",
                   win, seg_id, target_unitid_abs);
    DART_LOG_TRACE("dart_flush: MPI_Win_flush");
    MPI_Win_flush(target_unitid_abs, win);
  }
  DART_LOG_DEBUG("dart_flush > finished");
  return DART_OK;
}

dart_ret_t dart_flush_all(
  dart_gptr_t gptr)
{
  int16_t seg_id;
  seg_id = gptr.segid;
  MPI_Win win;
  DART_LOG_DEBUG("dart_flush_all() win:%d", win);
  if (seg_id) {
    uint16_t index = gptr.flags;
    win = dart_win_lists[index];
  } else {
    win = dart_win_local_alloc;
  }
  DART_LOG_TRACE("dart_flush_all: MPI_Win_flush_all");
  MPI_Win_flush_all(win);
  DART_LOG_DEBUG("dart_flush_all > finished");
  return DART_OK;
}

dart_ret_t dart_flush_local(
  dart_gptr_t gptr)
{
  dart_unit_t target_unitid_abs;
  int16_t seg_id = gptr.segid;
  MPI_Win win;
  target_unitid_abs = gptr.unitid;
  if (seg_id) {
    uint16_t index = gptr.flags;
    dart_unit_t target_unitid_rel;
    win = dart_win_lists[index];
    DART_LOG_DEBUG("dart_flush_local() win:%d seg:%d unit:%d",
                   win, seg_id, target_unitid_abs);
    unit_g2l(index, target_unitid_abs, &target_unitid_rel);
    DART_LOG_TRACE("dart_flush_local: MPI_Win_flush_local");
    MPI_Win_flush_local(target_unitid_rel, win);
  } else {
    win = dart_win_local_alloc;
    DART_LOG_DEBUG("dart_flush_local() lwin:%p seg:%d unit:%d",
                   win, seg_id, target_unitid_abs);
    DART_LOG_TRACE("dart_flush_local: MPI_Win_flush_local");
    MPI_Win_flush_local(target_unitid_abs, win);
  }
  DART_LOG_DEBUG("dart_flush_local > finished");
  return DART_OK;
}

dart_ret_t dart_flush_local_all(
  dart_gptr_t gptr)
{
  int16_t seg_id = gptr.segid;
  MPI_Win win;
  if (seg_id) {
    uint16_t index = gptr.flags;
    win = dart_win_lists[index];
  } else {
    win = dart_win_local_alloc;
  }
  MPI_Win_flush_local_all(win);
  DART_LOG_DEBUG("dart_flush_local_all > finished");
  return DART_OK;
}

dart_ret_t dart_wait_local(
  dart_handle_t handle)
{
  DART_LOG_DEBUG("dart_wait_local()");
  if (handle) {
    MPI_Status mpi_sta;
    MPI_Wait (&(handle->request), &mpi_sta);
  }
  DART_LOG_DEBUG("dart_wait_local > finished");
  return DART_OK;
}

dart_ret_t dart_wait(
  dart_handle_t handle)
{
  int mpi_ret;
  DART_LOG_DEBUG("dart_wait() handle:%p", (void*)(handle));
  if (handle != NULL) {
    DART_LOG_TRACE("dart_wait:     handle->dest:    %d", handle->dest);
    DART_LOG_TRACE("dart_wait:     handle->win:     %d", handle->win);
    DART_LOG_TRACE("dart_wait:     handle->request: %d", handle->request);
    if (handle->request != MPI_REQUEST_NULL) {
      MPI_Status mpi_sta;
      DART_LOG_DEBUG("dart_wait:     -- MPI_Wait");
      mpi_ret = MPI_Wait(&(handle->request), &mpi_sta);
      DART_LOG_TRACE("dart_wait:        -- mpi_sta.MPI_SOURCE = %d",
                     mpi_sta.MPI_SOURCE);
      DART_LOG_TRACE("dart_wait:        -- mpi_sta.MPI_ERROR  = %d",
                     mpi_sta.MPI_ERROR);
      if (mpi_ret != MPI_SUCCESS) {
        DART_LOG_DEBUG("dart_wait > MPI_Wait failed");
        return DART_ERR_INVAL;
      }
      DART_LOG_DEBUG("dart_wait:     -- MPI_Win_flush");
      mpi_ret = MPI_Win_flush(handle->dest, handle->win);
      if (mpi_ret != MPI_SUCCESS) {
        DART_LOG_DEBUG("dart_wait > MPI_Win_flush failed");
        return DART_ERR_INVAL;
      }
    } else {
      DART_LOG_TRACE("dart_wait:     handle->request: MPI_REQUEST_NULL");
    }
    /* Free handle resource */
    DART_LOG_DEBUG("dart_wait:   free handle %p", (void*)(handle));
    free(handle);
  }
  DART_LOG_DEBUG("dart_wait > finished");
  return DART_OK;
}

dart_ret_t dart_test_local(
  dart_handle_t handle,
  int32_t* is_finished)
{
  DART_LOG_DEBUG("dart_test_local()");
  if (!handle) {
    *is_finished = 1;
    return DART_OK;
  }
  MPI_Status mpi_sta;
  MPI_Test (&(handle->request), is_finished, &mpi_sta);
  DART_LOG_DEBUG("dart_test_local > finished");
  return DART_OK;
}

dart_ret_t dart_waitall_local(
  dart_handle_t *handle,
  size_t n)
{
  int i, r_n = 0;
  DART_LOG_DEBUG("dart_waitall_local()");
  if (*handle) {
    MPI_Status  *mpi_sta;
    MPI_Request *mpi_req;
    mpi_req = (MPI_Request *)malloc(n * sizeof (MPI_Request));
    mpi_sta = (MPI_Status *)malloc(n * sizeof (MPI_Status));
    for (i = 0; i < n; i++)  {
      if (handle[i]) {
        mpi_req[r_n++] = handle[i] -> request;
      }
    }
    MPI_Waitall(r_n, mpi_req, mpi_sta);
    r_n = 0;
    for (i = 0; i < n; i++) {
      if (handle[i]) {
        handle[r_n++] -> request = mpi_req[i++];
      }
    }
    free(mpi_req);
    free(mpi_sta);
  }
  DART_LOG_DEBUG("dart_waitall_local > finished");
  return DART_OK;
}

dart_ret_t dart_waitall(
  dart_handle_t * handle,
  size_t          n)
{
  int i, r_n;
  int num_handles = (int)n;
  DART_LOG_DEBUG("dart_waitall()");
  if (n < 1) {
    DART_LOG_ERROR("dart_waitall: number of handles = 0");
    return DART_ERR_INVAL;
  }
  if (n > INT_MAX) {
    DART_LOG_ERROR("dart_waitall: number of handles > INT_MAX");
    return DART_ERR_INVAL;
  }
  DART_LOG_DEBUG("dart_waitall: number of handles: %d", num_handles);
  if (*handle) {
    MPI_Status  *mpi_sta;
    MPI_Request *mpi_req;
    mpi_req = (MPI_Request *) malloc(num_handles * sizeof(MPI_Request));
    mpi_sta = (MPI_Status *)  malloc(num_handles * sizeof(MPI_Status));
    /*
     * copy requests from DART handles to MPI request array:
     */
    DART_LOG_TRACE("dart_waitall: copying DART handles to MPI request array");
    r_n = 0;
    for (i = 0; i < num_handles; i++) {
      if (handle[i]) {
        DART_LOG_DEBUG("dart_waitall:"
                       " -- handle[%d](%p): dest:%d win:%d req:%lld",
                       i, handle[i],
                       handle[i]->dest, handle[i]->win, handle[i]->request);
        mpi_req[r_n] = handle[i]->request;
        r_n++;
      }
    }
    /*
     * wait for communication of MPI requests:
     */
    DART_LOG_DEBUG("dart_waitall: MPI_Waitall, %d requests from %d handles",
                   r_n, num_handles);
    /* From the MPI 3.1 standard:
     *
     * The i-th entry in array_of_statuses is set to the return
     * status of the i-th operation. Active persistent requests
     * are marked inactive.
     * Requests of any other type are deallocated and the
     * corresponding handles in the array are set to
     * MPI_REQUEST_NULL.
     * The list may contain null or inactive handles.
     * The call sets to empty the status of each such entry.
     */
    if (MPI_Waitall(r_n, mpi_req, mpi_sta) == MPI_SUCCESS) {
      DART_LOG_DEBUG("dart_waitall: MPI_Waitall completed");
    } else {
      DART_LOG_ERROR("dart_waitall: MPI_Waitall failed");
      DART_LOG_TRACE("dart_waitall: free MPI_Request temporaries");
      free(mpi_req);
      DART_LOG_TRACE("dart_waitall: free MPI_Status temporaries");
      free(mpi_sta);
      return DART_ERR_INVAL;
    }
    /*
     * copy MPI requests back to DART handles:
     */
    DART_LOG_TRACE("dart_waitall: copying MPI requests back to DART handles");
    r_n = 0;
    for (i = 0; i < num_handles; i++) {
      if (handle[i]) {
        if (mpi_req[r_n] == MPI_REQUEST_NULL) {
          DART_LOG_TRACE("dart_waitall: -- mpi_req[%d] = MPI_REQUEST_NULL",
                         r_n);
        } else {
          DART_LOG_TRACE("dart_waitall: -- mpi_req[%d] = %d",
                         r_n, mpi_req[r_n]);
        }
        DART_LOG_TRACE("dart_waitall: -- mpi_sta[%d].MPI_SOURCE = %d",
                       r_n, mpi_sta[r_n].MPI_SOURCE);
        DART_LOG_TRACE("dart_waitall: -- mpi_sta[%d].MPI_ERROR  = %d",
                       r_n, mpi_sta[r_n].MPI_ERROR);
        handle[i]->request = mpi_req[r_n];
        r_n++;
      }
    }
    /*
     * wait for completion of MPI requests at origins and targets:
     */
    DART_LOG_DEBUG("dart_waitall: waiting for remote completion");
    for (i = 0; i < num_handles; i++) {
      if (handle[i]) {
        if (handle[i]->request == MPI_REQUEST_NULL) {
          DART_LOG_TRACE("dart_waitall: -- handle[%d] done (MPI_REQUEST_NULL)",
                         i);
        } else {
          DART_LOG_TRACE("dart_waitall: -- MPI_Win_flush(handle[%d]: %p)",
                         i, handle[i]);
          DART_LOG_TRACE("dart_waitall:      handle[%d]->dest: %d",
                         i, handle[i]->dest);
          DART_LOG_TRACE("dart_waitall:      handle[%d]->win:  %d",
                         i, handle[i]->win);
          if (MPI_Win_flush(handle[i]->dest, handle[i]->win) != MPI_SUCCESS) {
            DART_LOG_ERROR("dart_waitall: MPI_Win_flush failed");
            DART_LOG_TRACE("dart_waitall: free MPI_Request temporaries");
            free(mpi_req);
            DART_LOG_TRACE("dart_waitall: free MPI_Status temporaries");
            free(mpi_sta);
            return DART_ERR_INVAL;
          }
          DART_LOG_TRACE("dart_waitall: -- MPI_Request_free");
          if (MPI_Request_free(&handle[i]->request) != MPI_SUCCESS) {
            DART_LOG_ERROR("dart_waitall: MPI_Request_free failed");
            DART_LOG_TRACE("dart_waitall: free MPI_Request temporaries");
            free(mpi_req);
            DART_LOG_TRACE("dart_waitall: free MPI_Status temporaries");
            free(mpi_sta);
            return DART_ERR_INVAL;
          }
        }
      }
    }
    /*
     * free memory:
     */
    DART_LOG_DEBUG("dart_waitall: free handles");
    for (i = 0; i < num_handles; i++) {
      if (handle[i]) {
        /* Free handle resource */
        DART_LOG_TRACE("dart_waitall: -- free handle[%d]: %p",
                       i, (void*)(handle[i]));
        free(handle[i]);
        handle[i] = NULL;
      }
    }
    DART_LOG_TRACE("dart_waitall: free MPI_Request temporaries");
    free(mpi_req);
    DART_LOG_TRACE("dart_waitall: free MPI_Status temporaries");
    free(mpi_sta);
  }
  DART_LOG_DEBUG("dart_waitall > finished");
  return DART_OK;
}

dart_ret_t dart_testall_local(
  dart_handle_t * handle,
  size_t          n,
  int32_t       * is_finished)
{
  int i, r_n;
  DART_LOG_DEBUG("dart_testall_local()");
  MPI_Status *mpi_sta;
  MPI_Request *mpi_req;
  mpi_req = (MPI_Request *)malloc(n * sizeof (MPI_Request));
  mpi_sta = (MPI_Status *)malloc(n * sizeof (MPI_Status));
  r_n = 0;
  for (i = 0; i < n; i++) {
    if (handle[i]){
      mpi_req[r_n] = handle[i] -> request;
      r_n++;
    }
  }
  MPI_Testall(r_n, mpi_req, is_finished, mpi_sta);
  r_n = 0;
  for (i = 0; i < n; i++) {
    if (handle[i]) {
      handle[i] -> request = mpi_req[r_n];
      r_n++;
    }
  }
  free(mpi_req);
  free(mpi_sta);
  DART_LOG_DEBUG("dart_testall_local > finished");
  return DART_OK;
}

/* -- Dart collective operations -- */

dart_ret_t dart_barrier(
  dart_team_t teamid)
{
  MPI_Comm comm;
  uint16_t index;
  int      result;
  DART_LOG_DEBUG("dart_barrier()");
  result = dart_adapt_teamlist_convert(teamid, &index);
  if (result == -1) {
    return DART_ERR_INVAL;
  }
  /* Fetch proper communicator from teams. */
  comm = dart_teams[index];
  if (MPI_Barrier(comm) == MPI_SUCCESS) {
    DART_LOG_DEBUG("dart_barrier > finished");
    return DART_OK;
  }
  DART_LOG_DEBUG("dart_barrier ! MPI_Barrier failed");
  return DART_ERR_INVAL;
}

dart_ret_t dart_bcast(
  void *buf,
  size_t nbytes,
  int root,
  dart_team_t teamid)
{
  MPI_Comm comm;
  uint16_t index;
  int result = dart_adapt_teamlist_convert (teamid, &index);
  if (result == -1) {
    return DART_ERR_INVAL;
  }
  comm = dart_teams[index];
  return MPI_Bcast(buf, nbytes, MPI_BYTE, root, comm);
}

dart_ret_t dart_scatter(
  void *sendbuf,
  void *recvbuf,
  size_t nbytes,
  int root,
  dart_team_t teamid)
{
  MPI_Comm comm;
  uint16_t index;
  int result = dart_adapt_teamlist_convert(teamid, &index);
  if (result == -1) {
    return DART_ERR_INVAL;
  }
  comm = dart_teams[index];
  return MPI_Scatter(
           sendbuf,
           nbytes,
           MPI_BYTE,
           recvbuf,
           nbytes,
           MPI_BYTE,
           root,
           comm);
}

dart_ret_t dart_gather(
  void *sendbuf,
  void *recvbuf,
  size_t nbytes,
  int root,
  dart_team_t teamid)
{
  MPI_Comm comm;
  uint16_t index;
  int result = dart_adapt_teamlist_convert(teamid, &index);
  if (result == -1) {
    return DART_ERR_INVAL;
  }
  comm = dart_teams[index];
  return MPI_Gather(
           sendbuf,
           nbytes,
           MPI_BYTE,
           recvbuf,
           nbytes,
           MPI_BYTE,
           root,
           comm);
}

dart_ret_t dart_allgather(
  void *sendbuf,
  void *recvbuf,
  size_t nbytes,
  dart_team_t teamid)
{
  MPI_Comm comm;
  uint16_t index;
  int result = dart_adapt_teamlist_convert (teamid, &index);
  if (result == -1) {
    return DART_ERR_INVAL;
  }
  comm = dart_teams[index];
  return MPI_Allgather(
           sendbuf,
           nbytes,
           MPI_BYTE,
           recvbuf,
           nbytes,
           MPI_BYTE,
           comm);
}

dart_ret_t dart_reduce_double(
  double *sendbuf,
  double *recvbuf,
  dart_team_t teamid)
{
  MPI_Comm comm;
  uint16_t index;
  int result = dart_adapt_teamlist_convert (teamid, &index);
  if (result == -1) {
    return DART_ERR_INVAL;
  }
  comm = dart_teams[index];
  return MPI_Reduce(
           sendbuf,
           recvbuf,
           1,
           MPI_DOUBLE,
           MPI_MAX,
           0,
           comm);
}
