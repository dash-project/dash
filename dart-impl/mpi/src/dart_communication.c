/** 
 * @file dart_communication.c
 * @date 25 Aug 2014
 * @brief Implementations of all the dart communication operations. 
 *  
 * All the following functions are implemented with the underling *MPI-3*
 * one-sided runtime system.
 */


#include <dash/dart/mpi/dart_deb_log.h>
#include <stdio.h>
#include <mpi.h>
#include <string.h>
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
  void *dest,
  dart_gptr_t gptr,
  size_t nbytes)
{
  MPI_Aint disp_s, disp_rel;
  dart_unit_t target_unitid_abs;
  uint64_t offset = gptr.addr_or_offs.offset;
  int16_t seg_id = gptr.segid;
  target_unitid_abs = gptr.unitid;
  MPI_Win win;

  if (seg_id) {
    uint16_t index = gptr.flags;
    dart_unit_t target_unitid_rel;
                               
    win = dart_win_lists[index];
    unit_g2l(index, target_unitid_abs, &target_unitid_rel);      
    if (dart_adapt_transtable_get_disp(
          seg_id,
          target_unitid_rel,
          &disp_s)== -1) {
      return DART_ERR_INVAL;
    }
    disp_rel = disp_s + offset;
    MPI_Get(
      dest,
      nbytes,
      MPI_BYTE,
      target_unitid_rel,
      disp_rel,
      nbytes,
      MPI_BYTE,
      win);
    DEBUG("GET  - %d bytes (allocated with collective allocation) "
          "from %d at the offset %d",
          nbytes, target_unitid_abs, offset);
  } else {
    win = dart_win_local_alloc;
    MPI_Get(
      dest,
      nbytes,
      MPI_BYTE,
      target_unitid_abs,
      offset,
      nbytes,
      MPI_BYTE,
      win);
    DEBUG ("GET  - %d bytes (allocated with local allocation) "
           "from %d at the offset %d",
           nbytes, target_unitid_abs, offset);
  }                
  return DART_OK;
}

dart_ret_t dart_put(
  dart_gptr_t gptr,
  void *src,
  size_t nbytes)
{
  MPI_Aint disp_s, disp_rel;
  dart_unit_t target_unitid_abs;
  uint64_t offset   = gptr.addr_or_offs.offset;
  int16_t  seg_id   = gptr.segid;
  MPI_Win  win;
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
    DEBUG ("PUT  -%d bytes (allocated with collective allocation) "
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
    DEBUG ("PUT  - %d bytes (allocated with local allocation) "
           "to %d at the offset %d", 
           nbytes, target_unitid_abs, offset);
  }
  return DART_OK;
}

dart_ret_t dart_accumulate_int(
  dart_gptr_t dest,
  int *values,
  size_t nvalues,
  dart_operation_t op,
  dart_team_t team) {
  // TODO
  return DART_OK;
}

/* -- Non-blocking dart one-sided operations -- */

dart_ret_t dart_get_handle(
  void *dest,
  dart_gptr_t gptr,
  size_t nbytes,
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
  
  /* The memory accessed is allocated with collective allocation. */
  if (seg_id) {
    uint16_t index = gptr.flags;
    dart_unit_t target_unitid_rel;      
/*
    if (dart_adapt_transtable_get_win (index, offset, &begin, &win) == -1)
    {
      ERROR ("Invalid accessing operation");
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

    if (dart_adapt_transtable_get_disp(
          seg_id,
          target_unitid_rel,
          &disp_s)== -1) {
      return DART_ERR_INVAL;
    }
    disp_rel = disp_s + offset;   
    /* MPI-3 newly added feature: request version of get call. */

    /** 
     * TODO: Check if 
     *    MPI_Rget_accumulate(
     *      NULL, 0, MPI_BYTE, dest, nbytes, MPI_BYTE, 
     *      target_unitid, disp_rel, nbytes, MPI_BYTE, MPI_NO_OP, win,
     *      &mpi_req)
     *  ... could be an better alternative? 
     */
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
    (*handle) -> dest = target_unitid_rel;
    DEBUG ("GET  - %d bytes (allocated with collective allocation) "  
           "from %d at the offset %d", 
           nbytes, target_unitid_abs, offset);
  } else {
    /* The memory accessed is allocated with local allocation. */
    win = dart_win_local_alloc;
    MPI_Rget(
      dest,
      nbytes,
      MPI_BYTE,
      target_unitid_abs,
      offset,
      nbytes,
      MPI_BYTE,
      win,
      &mpi_req);
    (*handle) -> dest = target_unitid_abs;
    DEBUG ("GET  - %d bytes (allocated with local allocation) "
           "from %d at the offset %d",
           nbytes, target_unitid_abs, offset);
  }
  (*handle) -> request = mpi_req;
  (*handle) -> win     = win;
  return DART_OK;
}

dart_ret_t dart_put_handle(
  dart_gptr_t gptr,
  void *src,
  size_t nbytes,
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
      ERROR ("Invalid accessing operation");
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
    DEBUG("PUT  -%d bytes (allocated with collective allocation) "
          "to %d at the offset %d",
          nbytes, target_unitid_abs, offset);
  } else {
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
    DEBUG("PUT  - %d bytes (allocated with local allocation) "
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
  dart_gptr_t gptr,
  void *src,
  size_t nbytes)
{
  MPI_Win win;
  MPI_Aint disp_s, disp_rel;

  uint64_t offset = gptr.addr_or_offs.offset;
  int16_t seg_id = gptr.segid;
  uint16_t index = gptr.flags;
  dart_unit_t unitid, target_unitid_rel, target_unitid_abs = gptr.unitid;

#ifdef SHAREDMEM_ENABLE
if (seg_id >= 0){
  int i, is_sharedmem = 0;
  MPI_Aint maximum_size;
  int disp_unit;
  char* baseptr;
//  char *baseptr;
//MPI_Request mpi_req;

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
    if (seg_id) {
      if (dart_adapt_transtable_get_baseptr (seg_id, i, &baseptr) == -1) {
        return DART_ERR_INVAL;
      }
    } else {
      baseptr = dart_sharedmem_local_baseptr_set[i];
    }
    disp_rel = offset;
    baseptr  = baseptr + disp_rel;
    memcpy(baseptr, ((char*)src), nbytes);
    return DART_OK;}}
   
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
    MPI_Win_flush(target_unitid_rel, win);
    /* MPI_Wait is invoked to release the resource brought by the mpi
     * request handle
     */
  if (seg_id) {
    DEBUG("PUT_BLOCKING  - %d bytes "
           "(allocated with collective allocation) to %d at the offset %d", 
           nbytes, target_unitid_abs, offset);
  } else {
    DEBUG("PUT_BLOCKING - %d bytes "
          "(allocated with local allocation) to %d at the offset %d",
          nbytes, target_unitid_abs, offset);
  }
  return DART_OK;}
}

/** 
 * TODO: Check if MPI_Accumulate (REPLACE) can bring better performance? 
 */
dart_ret_t dart_get_blocking(
  void *dest,
  dart_gptr_t gptr,
  size_t nbytes)
{
  MPI_Win win;
  MPI_Status mpi_sta;
  MPI_Request mpi_req;
  MPI_Aint disp_s, disp_rel;
  
  uint64_t offset = gptr.addr_or_offs.offset;
  int16_t seg_id = gptr.segid;
  uint16_t index  = gptr.flags;
  dart_unit_t unitid, target_unitid_rel, target_unitid_abs = gptr.unitid;

#ifdef SHAREDMEM_ENABLE 
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
      if (seg_id) {
      if (dart_adapt_transtable_get_baseptr(seg_id, i, &baseptr)!=-1)
      return DART_ERR_INVAL;
      } else {
        baseptr = dart_sharedmem_local_baseptr_set[i];
      }
      disp_rel = offset;
      baseptr += disp_rel;
      memcpy ((char*)dest, baseptr, nbytes);
      return DART_OK;
    }
  }

#if 0
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
#endif  
#endif // SHAREDMEM_ENABLE
  
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
      win = dart_win_local_alloc;
      disp_rel = offset;
      target_unitid_rel = target_unitid_abs;
    }
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
    MPI_Wait(&mpi_req, &mpi_sta);
  
    if (seg_id) {
      DEBUG("GET_BLOCKING  - %d bytes "
            "(allocated with collective allocation) from %d "        
            "at the offset %d", 
            nbytes, target_unitid_abs, offset);
    } else {  
      DEBUG("GET_BLOCKING - %d bytes "
            "(allocated with local allocation) from %d at the offset %d", 
             nbytes, target_unitid_abs, offset);
    }
    return DART_OK;
  }
}

/* -- Dart RMA Synchronization Operations -- */

dart_ret_t dart_flush(
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
    unit_g2l (index, target_unitid_abs, &target_unitid_rel);    
    MPI_Win_flush(target_unitid_rel, win);
  } else {
    win = dart_win_local_alloc;
    MPI_Win_flush(target_unitid_abs, win);
  }
  LOG("FLUSH  - finished");
  return DART_OK;
}

dart_ret_t dart_flush_all(
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
  MPI_Win_flush_all(win);
  LOG("FLUSH_ALL  - finished");
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
    unit_g2l(index, target_unitid_abs, &target_unitid_rel);    
    MPI_Win_flush_local(target_unitid_rel, win);
  } else {
    win = dart_win_local_alloc;
    MPI_Win_flush_local(target_unitid_abs, win);
  }
  LOG("FLUSH_LOCAL - finished");
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
  LOG("FLUSH_LOCAL_ALL  - finished");
  return DART_OK;
}

dart_ret_t dart_wait_local(
  dart_handle_t handle)
{
  if (handle) {
    MPI_Status mpi_sta;
    MPI_Wait (&(handle->request), &mpi_sta);
  }
  LOG("WAIT_LOCAL  - finished");
  return DART_OK;
}

dart_ret_t dart_wait(
  dart_handle_t handle)
{
  if (handle) {
    MPI_Status mpi_sta;
    MPI_Wait (&(handle -> request), &mpi_sta);  
    MPI_Win_flush(handle->dest, handle->win);   
    /* Free handle resource */
    handle = NULL;
    free (handle);
  }
  LOG("WAIT  - finished");
  return DART_OK;
}

dart_ret_t dart_test_local(
  dart_handle_t handle,
  int32_t* is_finished)
{
  if (!handle) {
    *is_finished = 1;
    return DART_OK;
  }
  MPI_Status mpi_sta;
  MPI_Test (&(handle->request), is_finished, &mpi_sta);
  LOG("TEST_LOCAL  - finished");
  return DART_OK;
}

dart_ret_t dart_waitall_local(
  dart_handle_t *handle,
  size_t n)
{
  int i, r_n = 0;
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
    r_n=0;
    for (i = 0; i < n; i++) {
      if (handle[i]) {
        handle[r_n++] -> request = mpi_req[i++];
      }
    }
    free (mpi_req);
    free (mpi_sta);
  }
  LOG ("WAITALL_LOCAL  - finished");
  return DART_OK;
}

dart_ret_t dart_waitall(
  dart_handle_t *handle,
  size_t n)
{
  int i, n_r = 0;
  if (*handle) {
    MPI_Status  *mpi_sta;
    MPI_Request *mpi_req;
    mpi_req = (MPI_Request *)malloc(n * sizeof (MPI_Request));
    mpi_sta = (MPI_Status *)malloc(n * sizeof (MPI_Status));
    for (i = 0; i < n; i++) {
      if (handle[i]){
        mpi_req [n_r++] = handle[i] -> request;
      }
    }
    MPI_Waitall (n_r, mpi_req, mpi_sta);
    n_r = 0;
    for (i = 0; i < n; i++) {
      if (handle[i]) {
        handle[i] -> request = mpi_req[n_r++];
      }
    }
    free(mpi_req);
    free(mpi_sta);
    for (i = 0; i < n; i++) {
      if (handle[i]) {
        MPI_Win_flush (handle[i]->dest, handle[i]->win);
        /* Free handle resource */
        handle[i] = NULL;
        free (handle[i]);
      }
    }
  }
  LOG ("WAITALL  - finished");
  return DART_OK;
}

dart_ret_t dart_testall_local(
  dart_handle_t *handle,
  size_t n,
  int32_t* is_finished)
{
  int i, r_n = 0;
  MPI_Status *mpi_sta;
  MPI_Request *mpi_req;
  mpi_req = (MPI_Request *)malloc(n * sizeof (MPI_Request));
  mpi_sta = (MPI_Status *)malloc(n * sizeof (MPI_Status));
  for (i = 0; i < n; i++) {
    if (handle[i]){
      mpi_req[r_n++] = handle[i] -> request;
    }
  }
  MPI_Testall (r_n, mpi_req, is_finished, mpi_sta);
  r_n = 0;
  for (i = 0; i < n; i++) {
    if (handle[i]) {
      handle[i] -> request = mpi_req[r_n++];
    }
  }
  free(mpi_req);
  free(mpi_sta);
  LOG("TESTALL_LOCAL  - finished");
  return DART_OK;
}

/* -- Dart collective operations -- */

dart_ret_t dart_barrier(
  dart_team_t teamid)
{
  MPI_Comm comm;  
  uint16_t index;
  int result = dart_adapt_teamlist_convert(teamid, &index);
  if (result == -1) {
    return DART_ERR_INVAL;
  }
  /* Fetch proper communicator from teams. */
  comm = dart_teams[index];  
  return MPI_Barrier (comm);
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
