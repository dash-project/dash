#include <string.h>
#include <dash/dart/if/dart_communication.h>
#include <dash/dart/gaspi/dart_gaspi.h>
#include <dash/dart/gaspi/gaspi_utils.h>
#include <dash/dart/gaspi/dart_team_private.h>
#include <dash/dart/gaspi/dart_translation.h>
#include <dash/dart/gaspi/dart_mem.h>
#include <dash/dart/gaspi/dart_communication_priv.h>

#include <dash/dart/base/logging.h>
/*********************** Notify Value ************************/
//gaspi_notification_t dart_notify_value = 42;


/********************** Only for testing *********************/
gaspi_queue_id_t dart_handle_get_queue(dart_handle_t handle)
{
    return handle->queue;
}

dart_ret_t dart_scatter(
   const void        * sendbuf,
   void              * recvbuf,
   size_t              nelem,
   dart_datatype_t     dtype,
   dart_team_unit_t    root,
   dart_team_t         teamid)
{
    dart_team_unit_t             myid;
    size_t                  team_size;
    gaspi_notification_id_t first_id;
    gaspi_notification_t    old_value;
    gaspi_segment_id_t      gaspi_seg_id = dart_gaspi_buffer_id;
    gaspi_notification_id_t remote_id    = 0;
    gaspi_pointer_t         seg_ptr      = NULL;
    gaspi_queue_id_t        queue        = 0;
    gaspi_offset_t          local_offset = 0;
    uint16_t                index;
    size_t                  nbytes = dart_gaspi_datatype_sizeof(dtype) * nelem;

    DART_CHECK_ERROR(dart_team_myid(teamid, &myid));
    DART_CHECK_ERROR(dart_team_size(teamid, &team_size));

    if(dart_adapt_teamlist_convert(teamid, &index) == -1)
    {
        fprintf(stderr, "dart_scatter: can't find index of given team\n");
        return DART_ERR_OTHER;
    }

    if((nbytes * team_size) > DART_GASPI_BUFFER_SIZE)
    {
        DART_CHECK_GASPI_ERROR(gaspi_segment_create(dart_fallback_seg,
                                                    nbytes * team_size,
                                                    dart_teams[index].id,
                                                    GASPI_BLOCK,
                                                    GASPI_MEM_UNINITIALIZED));
        gaspi_seg_id = dart_fallback_seg;
        dart_fallback_seg_is_allocated = true;
    }

    DART_CHECK_ERROR(dart_barrier(teamid));
    DART_CHECK_GASPI_ERROR(gaspi_segment_ptr(gaspi_seg_id, &seg_ptr));

    if(myid.id == root.id)
    {
        memcpy( seg_ptr, sendbuf, nbytes * team_size );

        for (dart_unit_t unit = 0; unit < team_size; ++unit)
        {
            if ( unit == myid.id )
            {
                continue;
            }

            local_offset = nbytes * unit;
            dart_unit_t unit_abs;

            DART_CHECK_ERROR(unit_l2g(index, &unit_abs, unit));
            DART_CHECK_GASPI_ERROR(wait_for_queue_entries(&queue, 2));

            DART_CHECK_GASPI_ERROR(gaspi_write_notify(gaspi_seg_id,
                                                      local_offset,
                                                      unit_abs,
                                                      gaspi_seg_id,
                                                      0UL,
                                                      nbytes,
                                                      remote_id,
                                                      42,
                                                      queue,
                                                      GASPI_BLOCK));
        }

        memcpy(recvbuf, (void *) ((char *) seg_ptr + (myid.id * nbytes)), nbytes);
    }
    else
    {
        DART_CHECK_GASPI_ERROR(gaspi_notify_waitsome(gaspi_seg_id, remote_id, 1, &first_id, GASPI_BLOCK));
        DART_CHECK_GASPI_ERROR(gaspi_notify_reset(gaspi_seg_id, first_id, &old_value));

        memcpy(recvbuf, seg_ptr, nbytes);
    }

    DART_CHECK_ERROR(dart_barrier(teamid));

    if((nbytes * team_size) > DART_GASPI_BUFFER_SIZE)
    {
        DART_CHECK_GASPI_ERROR(gaspi_segment_delete(gaspi_seg_id));
        dart_fallback_seg_is_allocated = false;
    }
    return DART_OK;
}

//void *sendbuf, void *recvbuf, size_t nbytes, dart_unit_t root, dart_team_t team
dart_ret_t dart_gather(
      const void         * sendbuf,
      void               * recvbuf,
      size_t               nelem,
      dart_datatype_t      dtype,
      dart_team_unit_t     root,
      dart_team_t          teamid)
{
    uint16_t                index;
    dart_team_unit_t        relative_id;
    size_t                  team_size;
    gaspi_notification_id_t first_id;
    gaspi_notification_t    old_value;
    gaspi_segment_id_t      gaspi_seg_id  = dart_gaspi_buffer_id;
    gaspi_notification_t    notify_value  = 42;
    gaspi_pointer_t         seg_ptr       = NULL;
    gaspi_queue_id_t        queue         = 0;
    gaspi_offset_t          remote_offset = 0;
    size_t                  nbytes = dart_gaspi_datatype_sizeof(dtype) * nelem;

    if(dart_adapt_teamlist_convert(teamid, &index) == -1)
    {
        fprintf(stderr, "dart_gather: no team with id: %d\n", teamid);
        return DART_ERR_OTHER;
    }

    DART_CHECK_ERROR(dart_team_myid(teamid, &relative_id));
    DART_CHECK_ERROR(dart_team_size(teamid, &team_size));

    if((nbytes * team_size) > DART_GASPI_BUFFER_SIZE)
    {
        DART_CHECK_GASPI_ERROR(gaspi_segment_create(dart_fallback_seg,
                                                    nbytes * team_size,
                                                    dart_teams[index].id,
                                                    GASPI_BLOCK,
                                                    GASPI_MEM_UNINITIALIZED));
        gaspi_seg_id = dart_fallback_seg;
        dart_fallback_seg_is_allocated = true;
    }

    DART_CHECK_GASPI_ERROR(gaspi_segment_ptr(gaspi_seg_id, &seg_ptr));

    DART_CHECK_ERROR(dart_barrier(teamid));

    if(relative_id.id != root.id)
    {
        dart_unit_t abs_root_id;
        DART_CHECK_ERROR(unit_l2g(index, &abs_root_id, root.id));

        memcpy(seg_ptr, sendbuf, nbytes);
        remote_offset = relative_id.id * nbytes;

        DART_CHECK_GASPI_ERROR(wait_for_queue_entries(&queue, 2));
        DART_CHECK_GASPI_ERROR(gaspi_write_notify(gaspi_seg_id,
                                                  0UL,
                                                  abs_root_id,
                                                  gaspi_seg_id,
                                                  remote_offset,
                                                  nbytes,
                                                  relative_id.id,
                                                  notify_value,
                                                  queue,
                                                  GASPI_BLOCK));
    }
    else
    {
        gaspi_offset_t recv_buffer_offset = relative_id.id * nbytes;
        void         * recv_buffer_ptr    = (void *)((char *) seg_ptr + recv_buffer_offset);
        memcpy(recv_buffer_ptr, sendbuf, nbytes);

        int missing = team_size - 1;
        while(missing-- > 0)
        {
            DART_CHECK_GASPI_ERROR(blocking_waitsome(0, team_size, &first_id, &old_value, gaspi_seg_id));
            if(old_value != notify_value)
            {
                fprintf(stderr, "dart_gather: Error in process synchronization\n");
                break;
            }
        }
        memcpy(recvbuf, seg_ptr, team_size * nbytes);
    }

    DART_CHECK_ERROR(dart_barrier(teamid));

    if((nbytes * team_size) > DART_GASPI_BUFFER_SIZE)
    {
        DART_CHECK_GASPI_ERROR(gaspi_segment_delete(gaspi_seg_id));
        dart_fallback_seg_is_allocated = false;
    }

    return DART_OK;
}
/**
 * Implemented a binominal tree to broadcast the data
 *
 * TODO : In error case memory of children will leak
 * TODO : check if nbytes is the actuall number of bytes
 */
 //void *buf, size_t nbytes, dart_unit_t root, dart_team_t team
dart_ret_t dart_bcast(
   void              * buf,
   size_t              nelem,
   dart_datatype_t     dtype,
   dart_team_unit_t    root,
   dart_team_t         teamid
)
{
    const gaspi_notification_id_t notify_id    = 0;
    gaspi_queue_id_t              queue        = 0;
    gaspi_pointer_t               seg_ptr      = NULL;
    const gaspi_notification_t    notify_val   = 42;
    gaspi_segment_id_t            gaspi_seg_id = dart_gaspi_buffer_id;
    gaspi_notification_id_t       first_id;
    gaspi_notification_t          old_value;
    uint16_t                      index;
    dart_global_unit_t            myid;
    dart_unit_t                   root_abs;
    dart_team_unit_t              team_myid;
    size_t                        team_size;
    int                           parent;
    int                         * children = NULL;
    int                           children_count;
    size_t                  nbytes = dart_gaspi_datatype_sizeof(dtype) * nelem;
    int result = dart_adapt_teamlist_convert(teamid, &index);
    if(result == -1)
    {
        fprintf(stderr, "dart_bcast: can't find index of given team\n");
        return DART_ERR_INVAL;
    }

    DART_CHECK_ERROR(unit_l2g(index, &root_abs, root.id));
    DART_CHECK_ERROR(dart_myid(&myid));
    DART_CHECK_ERROR(dart_team_myid(teamid, &team_myid));
    DART_CHECK_ERROR(dart_team_size(teamid, &team_size));
    DART_CHECK_GASPI_ERROR(gaspi_segment_ptr(gaspi_seg_id, &seg_ptr));

    if(nbytes > DART_GASPI_BUFFER_SIZE)
    {
        DART_CHECK_GASPI_ERROR(gaspi_segment_create(dart_fallback_seg,
                                                    nbytes,
                                                    dart_teams[index].id,
                                                    GASPI_BLOCK,
                                                    GASPI_MEM_UNINITIALIZED));
        gaspi_seg_id = dart_fallback_seg;
        dart_fallback_seg_is_allocated = true;
    }

    if(myid.id == root_abs)
    {
        memcpy(seg_ptr, buf, nbytes);
    }

    children_count = gaspi_utils_compute_comms(&parent, &children, team_myid.id, root.id, team_size);

    DART_CHECK_ERROR(dart_barrier(teamid));

    dart_unit_t abs_parent;
    DART_CHECK_ERROR(unit_l2g(index, &abs_parent, parent));
    /*
     * parents + children wait for upper parents data
     */
    if (myid.id != abs_parent)
    {
        blocking_waitsome(notify_id, 1, &first_id, &old_value, gaspi_seg_id);
        if(old_value != notify_val)
        {
            fprintf(stderr, "dart_bcast : Got wrong notify value -> data transfer error\n");
        }
    }
    /*
     * write to all childs
     */
    for (int child = 0; child < children_count; child++)
    {
        dart_unit_t abs_child;
        DART_CHECK_ERROR(unit_l2g(index, &abs_child, children[child]));

        DART_CHECK_GASPI_ERROR(wait_for_queue_entries(&queue, 2));
        DART_CHECK_GASPI_ERROR(gaspi_write_notify(gaspi_seg_id,
                                                  0UL,
                                                  abs_child,
                                                  gaspi_seg_id,
                                                  0UL,
                                                  nbytes,
                                                  notify_id,
                                                  notify_val,
                                                  queue,
                                                  GASPI_BLOCK));
    }

    free(children);
    DART_CHECK_ERROR(dart_barrier(teamid));

    if(myid.id != root_abs)
    {
        memcpy(buf, seg_ptr, nbytes);
    }

    if(nbytes > DART_GASPI_BUFFER_SIZE)
    {
        DART_CHECK_GASPI_ERROR(gaspi_segment_delete(gaspi_seg_id));
        dart_fallback_seg_is_allocated = false;
    }

    return DART_OK;
}
//(void *sendbuf, void *recvbuf, size_t nbytes, dart_team_t team)
dart_ret_t dart_allgather(
  const void      * sendbuf,
  void            * recvbuf,
  size_t            nelem,
  dart_datatype_t   dtype,
  dart_team_t       teamid)
{
    gaspi_queue_id_t     queue = 0;
    gaspi_notification_t notify_value = 42;
    gaspi_segment_id_t   gaspi_seg_id = dart_gaspi_buffer_id;
    gaspi_pointer_t      seg_ptr      = NULL;
    dart_team_unit_t     relative_id;
    gaspi_offset_t       offset;
    size_t               teamsize;
    uint16_t             index;
    size_t               nbytes = dart_gaspi_datatype_sizeof(dtype) * nelem;

    DART_CHECK_ERROR(dart_team_myid(teamid, &relative_id));
    DART_CHECK_ERROR(dart_team_size(teamid, &teamsize));
    DART_CHECK_ERROR(dart_barrier(teamid));

    int result = dart_adapt_teamlist_convert(teamid, &index);
    if (result == -1)
    {
        return DART_ERR_INVAL;
    }

    if((nbytes * teamsize) > DART_GASPI_BUFFER_SIZE)
    {
        DART_CHECK_GASPI_ERROR(gaspi_segment_create(dart_fallback_seg,
                                                    nbytes * teamsize,
                                                    dart_teams[index].id,
                                                    GASPI_BLOCK,
                                                    GASPI_MEM_UNINITIALIZED));
        gaspi_seg_id = dart_fallback_seg;
        dart_fallback_seg_is_allocated = true;
    }


    offset = nbytes * relative_id.id;

    DART_CHECK_GASPI_ERROR(gaspi_segment_ptr(gaspi_seg_id, &seg_ptr));

    memcpy((void *) ((char *)seg_ptr + offset), sendbuf, nbytes);

    for (dart_unit_t unit = 0; unit < teamsize; ++unit)
    {
        if(unit == relative_id.id) continue;

        dart_unit_t unit_abs_id;
        DART_CHECK_ERROR(unit_l2g(index, &unit_abs_id, unit));

        DART_CHECK_GASPI_ERROR(wait_for_queue_entries(&queue, 2));
        DART_CHECK_GASPI_ERROR(gaspi_write_notify(gaspi_seg_id,
                                                  offset,
                                                  unit_abs_id,
                                                  gaspi_seg_id,
                                                  offset,
                                                  nbytes,
                                                  (gaspi_notification_id_t) relative_id.id,
                                                  notify_value,
                                                  queue,
                                                  GASPI_BLOCK));
    }

    int missing = teamsize - 1;
    gaspi_notification_id_t id_available;
    gaspi_notification_t    id_val;

    while(missing-- > 0)
    {
        DART_CHECK_GASPI_ERROR(blocking_waitsome(0, teamsize, &id_available, &id_val, gaspi_seg_id));
        if(id_val != notify_value)
        {
            fprintf(stderr, "Error: Get wrong notify in allgather\n");
        }
    }

    memcpy(recvbuf, seg_ptr, nbytes * teamsize);
    DART_CHECK_ERROR(dart_barrier(teamid));

    if((nbytes * teamsize) > DART_GASPI_BUFFER_SIZE)
    {
        DART_CHECK_GASPI_ERROR(gaspi_segment_delete(gaspi_seg_id));
        dart_fallback_seg_is_allocated = false;
    }

    return DART_OK;
}


//slightly altered version of gaspi dart_allgather
dart_ret_t dart_allgatherv(
  const void      * sendbuf,
  size_t            nsendelem,
  dart_datatype_t   dtype,
  void            * recvbuf,
  const size_t    * nrecvcounts,
  const size_t    * recvdispls,
  dart_team_t       teamid)
{
    gaspi_queue_id_t     queue = 0;
    gaspi_notification_t notify_value = 42;
    gaspi_segment_id_t   gaspi_seg_id = dart_gaspi_buffer_id;
    gaspi_pointer_t      seg_ptr      = NULL;
    dart_team_unit_t     relative_id;
    gaspi_offset_t       offset;
    size_t               teamsize;
    uint16_t             index;
    size_t               nbytes = dart_gaspi_datatype_sizeof(dtype) * nsendelem;

    DART_CHECK_ERROR(dart_team_myid(teamid, &relative_id));
    DART_CHECK_ERROR(dart_team_size(teamid, &teamsize));
    DART_CHECK_ERROR(dart_barrier(teamid));

    /*number of all allements
    *|data+disp of unit 0..n-1|disp for last(n-th) unit|data of last(n-th) unit|
    *                                                  ^-- pointer recvdispls[teamsize-1]
    *|----------------------------- num_overall_ellem -------------------------|
    */
    size_t num_overall_elemnts = recvdispls[teamsize-1] + nrecvcounts[teamsize-1];
    size_t n_total_bytes = dart_gaspi_datatype_sizeof(dtype) * num_overall_elemnts;

    int result = dart_adapt_teamlist_convert(teamid, &index);
    if (result == -1)
    {
        return DART_ERR_INVAL;
    }

    if((n_total_bytes * teamsize) > DART_GASPI_BUFFER_SIZE)
    {
        DART_CHECK_GASPI_ERROR(gaspi_segment_create(dart_fallback_seg,
                                                    n_total_bytes * teamsize,
                                                    dart_teams[index].id,
                                                    GASPI_BLOCK,
                                                    GASPI_MEM_UNINITIALIZED));
        gaspi_seg_id = dart_fallback_seg;
        dart_fallback_seg_is_allocated = true;
    }

    //local offset in Bytes. copies data from sendbuff in local part of the
    //segment to avoid communication and send to other units.
    offset = nbytes * relative_id.id + recvdispls[relative_id.id-1] * \
             dart_gaspi_datatype_sizeof(dtype);

    DART_CHECK_GASPI_ERROR(gaspi_segment_ptr(gaspi_seg_id, &seg_ptr));

    memcpy((void *) ((char *)seg_ptr + offset), sendbuf, nbytes);

    for (dart_unit_t unit = 0; unit < teamsize; ++unit)
    {
        //Avoid communication if target is this unit.
        if(unit == relative_id.id) continue;

        dart_unit_t unit_abs_id;
        DART_CHECK_ERROR(unit_l2g(index, &unit_abs_id, unit));

        DART_CHECK_GASPI_ERROR(wait_for_queue_entries(&queue, 2));
        DART_CHECK_GASPI_ERROR(gaspi_write_notify(gaspi_seg_id,
                                                  offset,
                                                  unit_abs_id,
                                                  gaspi_seg_id,
                                                  offset,
                                                  nbytes,
                                                  (gaspi_notification_id_t) relative_id.id,
                                                  notify_value,
                                                  queue,
                                                  GASPI_BLOCK));
    }

    int missing = teamsize - 1;
    gaspi_notification_id_t id_available;
    gaspi_notification_t    id_val;

    while(missing-- > 0)
    {
        DART_CHECK_GASPI_ERROR(blocking_waitsome(0, teamsize, &id_available, &id_val, gaspi_seg_id));
        if(id_val != notify_value)
        {
            fprintf(stderr, "Error: Get wrong notify in allgather\n");
        }
    }

    memcpy(recvbuf, seg_ptr, nbytes * teamsize);
    DART_CHECK_ERROR(dart_barrier(teamid));

    if((nbytes * teamsize) > DART_GASPI_BUFFER_SIZE)
    {
        DART_CHECK_GASPI_ERROR(gaspi_segment_delete(gaspi_seg_id));
        dart_fallback_seg_is_allocated = false;
    }

    return DART_OK;
}

dart_ret_t dart_barrier (
  dart_team_t teamid)
{
    uint16_t       index;
    if (dart_adapt_teamlist_convert (teamid, &index) == -1)
    {
        return DART_ERR_INVAL;
    }
    gaspi_group_t gaspi_group_id = dart_teams[index].id;
    DART_CHECK_GASPI_ERROR(gaspi_barrier(gaspi_group_id, GASPI_BLOCK));

    return DART_OK;
}
#if 0
/**
 * This operation sends a notification to a target specified in handle.
 * The notification reaches the target if the datatransfer(of the dart_put_gptr_handle)
 * is completed(remote and local).
 *
 * This function can be used with dart_put_gptr_handle to indicate the target unit the
 * availability of the data.
 *
 * @param handle: is the returned handle of the dart_put_gptr_handle
 * @param tag: must be not null and can be used to differentiate between notifies
 *
 * @return DART_OK => success
 * @return DART_ERR_* => failed
 */
dart_ret_t dart_notify_handle(dart_handle_t handle, unsigned int tag)
{
    dart_global_unit_t myid;
    dart_unit_t rel_myid;
    if(handle == NULL)
    {
        fprintf(stderr, "dart_notify_handle : Handle structure not initialized\n");
        return DART_ERR_OTHER;
    }

    DART_CHECK_ERROR(dart_myid(&myid));
    DART_CHECK_ERROR(unit_g2l(handle->index, myid.id, &rel_myid));
    DART_CHECK_GASPI_ERROR(gaspi_notify(handle->remote_seg, handle->target_unit, rel_myid, tag, handle->queue, GASPI_BLOCK));

    return DART_OK;
}
/**
 * This operation sends a notification to specified target and can be combined with a dart_put_gptr operation to
 * indicate the completion(remote+local) of the put operation to the target unit.
 *
 *  See also dart_notify_waitsome.
 *
 * @param gptr specifies the target(remote), is the same pointer which was used as destination in dart_put_gptr
 * @param tag must be not null and can be used to differentiate between notifies
 *
 * @return DART_OK => success
 * @return DART_ERR_* => failed
 */
dart_ret_t dart_notify(dart_gptr_t gptr, unsigned int tag)
{
    dart_global_unit_t myid;
    dart_unit_t        my_rel_id;
    gaspi_queue_id_t   queue;
    gaspi_segment_id_t gaspi_seg_id    = dart_mempool_seg_localalloc;
    int16_t            seg_id          = gptr.segid;
    uint16_t           index           = gptr.flags;
    dart_unit_t        target_unit     = gptr.unitid;

    DART_CHECK_ERROR(dart_myid(&myid));

    if(seg_id)
    {
        dart_unit_t rel_target_unit;
        DART_CHECK_ERROR(unit_g2l(index, target_unit, &rel_target_unit));
        if(dart_adapt_transtable_get_gaspi_seg_id(seg_id, rel_target_unit, &gaspi_seg_id) == -1)
        {
            fprintf(stderr, "Can't find given target segment id in dart_notify\n");
            return DART_ERR_NOTFOUND;
        }
    }

    int32_t found = 0;
    DART_CHECK_ERROR(find_rma_request(target_unit, seg_id, &queue, &found));
    if(!found)
    {
        DART_CHECK_ERROR(dart_get_minimal_queue(&queue));
    }

    DART_CHECK_ERROR(unit_g2l(index, myid.id, &my_rel_id));
    DART_CHECK_GASPI_ERROR(check_queue_size(queue));

    DART_CHECK_GASPI_ERROR(gaspi_notify(gaspi_seg_id, target_unit, my_rel_id, tag, queue, GASPI_BLOCK));

    return DART_OK;
}

/**
 * This operation waits on an incomming notification on a given global pointer.
 *
 * @param gptr specifies the data portion(segment)
 * @param tag returns the received tag of the transfered notification
 *
 * @return DART_OK => success
 * @return DART_ERR_* => failed
 */
dart_ret_t dart_notify_waitsome(dart_gptr_t gptr, unsigned int * tag)
{
    gaspi_notification_id_t notify_received;
    gaspi_segment_id_t      gaspi_seg_id = dart_mempool_seg_localalloc;
    int16_t                 seg_id       = gptr.segid;
    uint16_t                index        = gptr.flags;
    size_t                  team_size    = dart_teams[index].group->nmem;

    if(seg_id)
    {
        if(dart_adapt_transtable_get_local_gaspi_seg_id(seg_id, &gaspi_seg_id) == -1)
        {
            fprintf(stderr, "Can't find given segment id in dart_notify_wait\n");
        }
    }

    DART_CHECK_GASPI_ERROR(gaspi_notify_waitsome(gaspi_seg_id, 0, team_size, &notify_received, GASPI_BLOCK));
    DART_CHECK_GASPI_ERROR(gaspi_notify_reset(gaspi_seg_id, notify_received, tag));

    return DART_OK;
}
#endif

dart_ret_t dart_get_blocking(
  void            * dst,
  dart_gptr_t       gptr,
  size_t            nelem,
  dart_datatype_t   src_type,
  dart_datatype_t   dst_type)
{
    DART_CHECK_DATA_TYPE(src_type, dst_type);

    size_t nbytes = dart_gaspi_datatype_sizeof(src_type) * nelem;

    // initialized with relative team unit id
    dart_unit_t global_src_unit_id = gptr.unitid;

    gaspi_segment_id_t gaspi_src_seg_id = 0;
    DART_CHECK_ERROR(check_seg_id(&gptr, &global_src_unit_id, &gaspi_src_seg_id, "dart_get_blocking"));

    dart_global_unit_t global_my_unit_id;
    DART_CHECK_ERROR(dart_myid(&global_my_unit_id));
    if(global_my_unit_id.id == global_src_unit_id)
    {
        DART_CHECK_ERROR(local_copy_get(&gptr, gaspi_src_seg_id, dst, nbytes));
        return DART_OK;
    }

    gaspi_queue_id_t   queue = 0;
    DART_CHECK_ERROR( dart_get_minimal_queue(&queue));

    DART_CHECK_GASPI_ERROR(gaspi_segment_bind(dart_onesided_seg, dst, nbytes, 0));

    DART_CHECK_GASPI_ERROR_GOTO(dart_error_label, gaspi_read(dart_onesided_seg,
                                                             0,
                                                             global_src_unit_id,
                                                             gaspi_src_seg_id,
                                                             gptr.addr_or_offs.offset,
                                                             nbytes,
                                                             queue,
                                                             GASPI_BLOCK));

    DART_CHECK_GASPI_ERROR_GOTO(dart_error_label, gaspi_wait(queue, GASPI_BLOCK));

    DART_CHECK_ERROR(gaspi_segment_delete(dart_onesided_seg));


    return DART_OK;

dart_error_label:
    DART_CHECK_ERROR(gaspi_segment_delete(dart_onesided_seg));
    return DART_ERR_OTHER;
}

dart_ret_t dart_put_blocking(
  dart_gptr_t       gptr,
  const void      * src,
  size_t            nelem,
  dart_datatype_t   src_type,
  dart_datatype_t   dst_type)
{
    DART_CHECK_DATA_TYPE(src_type, dst_type);

    size_t nbytes = dart_gaspi_datatype_sizeof(dst_type) * nelem;

    // initialized with relative team unit id
    dart_unit_t global_dst_unit_id = gptr.unitid;

    gaspi_segment_id_t gaspi_dst_seg_id = 0;
    DART_CHECK_ERROR(check_seg_id(&gptr, &global_dst_unit_id, &gaspi_dst_seg_id, "dart_put_blocking"));

    dart_global_unit_t global_my_unit_id;
    DART_CHECK_ERROR(dart_myid(&global_my_unit_id));
    if(global_my_unit_id.id == global_dst_unit_id)
    {
        DART_CHECK_ERROR(local_copy_put(&gptr, gaspi_dst_seg_id, src, nbytes));
        return DART_OK;
    }

    gaspi_queue_id_t   queue = 0;
    DART_CHECK_ERROR( dart_get_minimal_queue(&queue));

    // cast to void* to avoid warnings in gaspi_segment_binding
    DART_CHECK_GASPI_ERROR(gaspi_segment_bind(dart_onesided_seg, (void*)src, nbytes, 0));

    DART_CHECK_GASPI_ERROR(gaspi_write(dart_onesided_seg,
                                       0,
                                       global_dst_unit_id,
                                       gaspi_dst_seg_id,
                                       gptr.addr_or_offs.offset,
                                       nbytes,
                                       queue,
                                       GASPI_BLOCK));

    DART_CHECK_GASPI_ERROR_GOTO(dart_error_label, gaspi_wait(queue, GASPI_BLOCK));

    DART_CHECK_ERROR(gaspi_segment_delete(dart_onesided_seg));

    return DART_OK;

dart_error_label:
    DART_CHECK_ERROR(gaspi_segment_delete(dart_onesided_seg));
    return DART_ERR_OTHER;
}

dart_ret_t dart_free_handle(
  dart_handle_t * handleptr)
{
    dart_handle_t handle = *handleptr;
    gaspi_notification_t val = 0;
    DART_CHECK_GASPI_ERROR(gaspi_notify_reset (handle->local_seg_id, handle->local_seg_id, &val));
    DART_CHECK_GASPI_ERROR(gaspi_segment_delete(handle->local_seg_id));
    DART_CHECK_ERROR(seg_stack_push(&dart_free_coll_seg_ids, handle->local_seg_id));
    free(handle);
    *handleptr = DART_HANDLE_NULL;

    if(val == 0)
    {
      DART_LOG_ERROR("Error: gaspi_notify value is not != 0\n");
    }

    return DART_OK;
}

dart_ret_t dart_wait_local (
  dart_handle_t * handleptr)
{
    if (handleptr != NULL && *handleptr != DART_HANDLE_NULL)
    {
        dart_handle_t handle = *handleptr;
        gaspi_notification_id_t id;

        DART_CHECK_GASPI_ERROR(gaspi_notify_waitsome(handle->local_seg_id, handle->local_seg_id, 1, &id, GASPI_BLOCK));

        DART_CHECK_ERROR(dart_free_handle(handleptr));
    }

    return DART_OK;
}

dart_ret_t dart_waitall_local(
  dart_handle_t handles[],
  size_t        num_handles)
{
    if (handles != NULL)
    {
        for(size_t i = 0 ; i < num_handles ; ++i)
        {
            DART_CHECK_ERROR(dart_wait_local(&handles[i]));
        }
    }

    return DART_OK;
}

dart_ret_t dart_wait(
  dart_handle_t * handleptr)
{

    if( handleptr != NULL && *handleptr != DART_HANDLE_NULL )
    {
        DART_CHECK_GASPI_ERROR((gaspi_wait((*handleptr)->queue, GASPI_BLOCK)));
        dart_free_handle(handleptr);
    }

    return DART_OK;
}

dart_ret_t dart_waitall(
  dart_handle_t handles[],
  size_t        num_handles)
{
    DART_LOG_DEBUG("dart_waitall()");
    if ( handles == NULL || num_handles == 0)
    {
        DART_LOG_DEBUG("dart_waitall: empty handles");
        return DART_OK;
    }

    for(size_t i = 0; i < num_handles; ++i)
    {
        DART_CHECK_GASPI_ERROR(gaspi_wait(handles[i]->queue, GASPI_BLOCK));
    }

    return DART_OK;
}

dart_ret_t dart_test_local (
  dart_handle_t * handleptr,
  int32_t       * is_finished)
{
    if (handleptr == NULL || *handleptr == DART_HANDLE_NULL)
    {
        *is_finished = 1;
        DART_LOG_DEBUG("dart_test_local: empty handle");

        return DART_OK;
    }

    *is_finished = 0;
    dart_handle_t handle = *handleptr;
    gaspi_notification_id_t id;
    gaspi_return_t test = gaspi_notify_waitsome(handle->local_seg_id, handle->local_seg_id, 1, &id, 1);
    if(test == GASPI_TIMEOUT)
    {
      return DART_OK;
    }

    if(test != GASPI_SUCCESS)
    {
        DART_LOG_ERROR("gaspi_notify_waitsome failed in dart_test_local");

        return DART_ERR_OTHER;
    }
    // finished is true even if freeing the handle will fail
    *is_finished = 1;
    DART_CHECK_ERROR(dart_free_handle(handleptr));

    return DART_OK;
}

dart_ret_t dart_testall_local(
  dart_handle_t   handles[],
  size_t          num_handles,
  int32_t       * is_finished)
{
    if (handles == NULL || num_handles == 0)
    {
        *is_finished = 1;
        DART_LOG_DEBUG("dart_testall_local: empty handle");

        return DART_OK;
    }

    *is_finished = 1;

    gaspi_notification_id_t id;
    for(int i = 0; i < num_handles; ++i)
    {
        dart_handle_t handle = handles[i];
        if(handle != DART_HANDLE_NULL)
        {
            gaspi_return_t test = gaspi_notify_waitsome(handle->local_seg_id, handle->local_seg_id, 1, &id, 1);
            if(test == GASPI_TIMEOUT)
            {
              *is_finished = 0;
              return DART_OK;
            }

            if(test != GASPI_SUCCESS)
            {
                DART_LOG_ERROR("gaspi_notify_waitsome failed in dart_testall_local");

                return DART_ERR_OTHER;
            }
        }
    }

    *is_finished = 1;
    for(int i = 0; i < num_handles; ++i)
    {
        DART_CHECK_ERROR(dart_free_handle(&handles[i]));
    }

    return DART_OK;
}

dart_ret_t dart_test(
  dart_handle_t * handleptr,
  int32_t       * is_finished)
{
    // Works for "get" requests only yet
    DART_CHECK_ERROR(dart_test_local (handleptr, is_finished));

    return DART_OK;
}

dart_ret_t dart_testall(
  dart_handle_t   handles[],
  size_t          num_handles,
  int32_t       * is_finished)
{
    // Works for "get" requests only yet
    DART_CHECK_ERROR(dart_testall_local (handles, num_handles, is_finished));
    return DART_OK;
}


dart_ret_t dart_get_handle(
  void*           dst,
  dart_gptr_t     gptr,
  size_t          nelem,
  dart_datatype_t src_type,
  dart_datatype_t dst_type,
  dart_handle_t*  handleptr)
{
    DART_CHECK_DATA_TYPE(src_type, dst_type);

    *handleptr = DART_HANDLE_NULL;
    dart_handle_t handle = *handleptr;

    size_t nbytes = dart_gaspi_datatype_sizeof(src_type) * nelem;

    // initialized with relative team unit id
    dart_unit_t global_src_unit_id = gptr.unitid;

    gaspi_segment_id_t gaspi_src_seg_id = 0;
    DART_CHECK_ERROR(check_seg_id(&gptr, &global_src_unit_id, &gaspi_src_seg_id, "dart_get_handle"));

    dart_global_unit_t global_my_unit_id;
    DART_CHECK_ERROR(dart_myid(&global_my_unit_id));
    if(global_my_unit_id.id == global_src_unit_id)
    {
        DART_CHECK_ERROR(local_copy_get(&gptr, gaspi_src_seg_id, dst, nbytes));
        return DART_OK;
    }

    gaspi_queue_id_t   queue = 0;
    DART_CHECK_ERROR( dart_get_minimal_queue(&queue));

    // get gaspi segment id and bind it to dst
    gaspi_segment_id_t free_seg_id;
    DART_CHECK_ERROR(seg_stack_pop(&dart_free_coll_seg_ids, &free_seg_id));
    DART_CHECK_GASPI_ERROR(gaspi_segment_bind(free_seg_id, dst, nbytes, 0));

    DART_CHECK_GASPI_ERROR(gaspi_read_notify(free_seg_id,
                                      0,
                                      global_src_unit_id,
                                      gaspi_src_seg_id,
                                      gptr.addr_or_offs.offset,
                                      nbytes,
                                      free_seg_id,
                                      queue,
                                      GASPI_BLOCK));

    handle = (dart_handle_t) malloc(sizeof(struct dart_handle_struct));
    handle->queue         = queue;
    handle->local_seg_id   = free_seg_id;
    handle->remote_seg_id   = gaspi_src_seg_id;
    *handleptr = handle;
    DART_LOG_DEBUG("dart_get_handle: handle(%p) dest:%d\n", (void*)(handle), global_src_unit_id);

    return DART_OK;
}

dart_ret_t dart_put_handle(
  dart_gptr_t       gptr,
  const void      * src,
  size_t            nelem,
  dart_datatype_t   src_type,
  dart_datatype_t   dst_type,
  dart_handle_t   * handleptr)
{
    DART_CHECK_DATA_TYPE(src_type, dst_type);

    *handleptr = DART_HANDLE_NULL;
    dart_handle_t handle = *handleptr;

    size_t nbytes = dart_gaspi_datatype_sizeof(dst_type) * nelem;

    // initialized with relative team unit id
    dart_unit_t global_dst_unit_id = gptr.unitid;

    gaspi_segment_id_t gaspi_dst_seg_id = 0;
    DART_CHECK_ERROR(check_seg_id(&gptr, &global_dst_unit_id, &gaspi_dst_seg_id, "dart_put_handle"));

    dart_global_unit_t global_my_unit_id;
    DART_CHECK_ERROR(dart_myid(&global_my_unit_id));
    if(global_my_unit_id.id == global_dst_unit_id)
    {
        DART_CHECK_ERROR(local_copy_put(&gptr, gaspi_dst_seg_id, src, nbytes));
        return DART_OK;
    }

    gaspi_queue_id_t   queue = 0;
    DART_CHECK_ERROR( dart_get_minimal_queue(&queue));

    // get gaspi segment id and bind it to dst
    gaspi_segment_id_t free_seg_id;
    DART_CHECK_ERROR(seg_stack_pop(&dart_free_coll_seg_ids, &free_seg_id));
    // cast to void* to avoid warnings in gaspi_segment_binding
    DART_CHECK_GASPI_ERROR(gaspi_segment_bind(free_seg_id, (void*) src, nbytes, 0));

    DART_CHECK_GASPI_ERROR(gaspi_write_notify(free_seg_id,
                                      0,
                                      global_dst_unit_id,
                                      gaspi_dst_seg_id,
                                      gptr.addr_or_offs.offset,
                                      nbytes,
                                      free_seg_id, // notify id
                                      free_seg_id, // notify value
                                      queue,
                                      GASPI_BLOCK));

    handle = (dart_handle_t) malloc(sizeof(struct dart_handle_struct));
    handle->queue         = queue;
    handle->local_seg_id   = free_seg_id;
    handle->remote_seg_id   = gaspi_dst_seg_id;
    *handleptr = handle;
    DART_LOG_DEBUG("dart_get_handle: handle(%p) dest:%d\n", (void*)(handle), global_dst_unit_id);

    return DART_OK;
}

dart_ret_t dart_flush(dart_gptr_t gptr)
{
    char             found_rma_req  = 0;
    gaspi_queue_id_t queue_id;

    DART_CHECK_ERROR(find_rma_request(gptr.unitid, gptr.segid, &queue_id, &found_rma_req));
    if(found_rma_req)
    {
        DART_CHECK_GASPI_ERROR(gaspi_wait(queue_id, GASPI_BLOCK));
    }
    else
    {
      DART_LOG_DEBUG("dart_flush: no queue found");
    }

    return DART_OK;
}

dart_ret_t dart_flush_all(dart_gptr_t gptr)
{
    int16_t seg_id      = gptr.segid;

    request_iterator_t iter = new_request_iter(seg_id);
    if(request_iter_is_vaild(iter)){
      while(request_iter_is_vaild(iter))
      {
          gaspi_queue_id_t qid;
          DART_CHECK_ERROR(request_iter_get_queue(iter, &qid));

          DART_CHECK_GASPI_ERROR(gaspi_wait(qid, GASPI_BLOCK));

          DART_CHECK_ERROR(request_iter_next(iter));
      }

      DART_CHECK_ERROR(destroy_request_iter(iter));
   }
    return DART_OK;
}

dart_ret_t dart_flush_local(dart_gptr_t gptr)
{
    gaspi_return_t ret = GASPI_SUCCESS;
    DART_CHECK_ERROR(dart_flush(gptr));
    if(ret != GASPI_SUCCESS)
    {
        printf("ERROR in %s at line %d in file %s\n","_dart_flush_local", __LINE__, __FILE__);
        return DART_ERR_OTHER;
    }
    return DART_OK;
}

dart_ret_t dart_flush_local_all(dart_gptr_t gptr)
{
    int16_t seg_id      = gptr.segid;

    request_iterator_t iter = new_request_iter(seg_id);
    if(request_iter_is_vaild(iter)){
      while(request_iter_is_vaild(iter))
      {
          gaspi_queue_id_t qid;
          DART_CHECK_ERROR(request_iter_get_queue(iter, &qid));

          DART_CHECK_GASPI_ERROR(gaspi_wait(qid, GASPI_BLOCK));

          DART_CHECK_ERROR(request_iter_next(iter));
      }

      DART_CHECK_ERROR(destroy_request_iter(iter));
   }
    return DART_OK;
}



dart_ret_t dart_get(
  void            * dst,
  dart_gptr_t       gptr,
  size_t            nelem,
  dart_datatype_t   src_type,
  dart_datatype_t   dst_type)
{
   DART_CHECK_DATA_TYPE(src_type, dst_type);

    size_t nbytes = dart_gaspi_datatype_sizeof(src_type) * nelem;

    // initialized with relative team unit id
    dart_unit_t global_src_unit_id = gptr.unitid;

    gaspi_segment_id_t gaspi_src_seg_id = 0;
    DART_CHECK_ERROR(check_seg_id(&gptr, &global_src_unit_id, &gaspi_src_seg_id, "dart_get"));

    dart_global_unit_t global_my_unit_id;
    DART_CHECK_ERROR(dart_myid(&global_my_unit_id));
    if(global_my_unit_id.id == global_src_unit_id)
    {
        DART_CHECK_ERROR(local_copy_get(&gptr, gaspi_src_seg_id, dst, nbytes));
        return DART_OK;
    }

    // get gaspi segment id and bind it to dst
    gaspi_segment_id_t free_seg_id;
    DART_CHECK_ERROR(seg_stack_pop(&dart_free_coll_seg_ids, &free_seg_id));
    DART_CHECK_GASPI_ERROR(gaspi_segment_bind(free_seg_id, dst, nbytes, 0));

    //communitcation request
    char found_rma_req = 0;
    gaspi_queue_id_t   queue = 0;
    DART_CHECK_ERROR(find_rma_request(gptr.unitid, gptr.segid, &queue, &found_rma_req));
    if(!found_rma_req)
    {
        DART_CHECK_ERROR(dart_get_minimal_queue(&queue));
        DART_CHECK_ERROR(add_rma_request_entry(gptr.unitid, gptr.unitid, queue));
    }
    else
    {
        DART_CHECK_GASPI_ERROR(check_queue_size(queue));
    }

    DART_CHECK_GASPI_ERROR(gaspi_read(free_seg_id,
                                      0,
                                      global_src_unit_id,
                                      gaspi_src_seg_id,
                                      gptr.addr_or_offs.offset,
                                      nbytes,
                                      queue,
                                      GASPI_BLOCK));

   return DART_OK;
}

dart_ret_t dart_put(
  dart_gptr_t       gptr,
  const void      * src,
  size_t            nelem,
  dart_datatype_t   src_type,
  dart_datatype_t   dst_type)
{
    DART_CHECK_DATA_TYPE(src_type, dst_type);

    size_t nbytes = dart_gaspi_datatype_sizeof(dst_type) * nelem;

    // initialized with relative team unit id
    dart_unit_t global_dst_unit_id = gptr.unitid;

    gaspi_segment_id_t gaspi_dst_seg_id = 0;
    DART_CHECK_ERROR(check_seg_id(&gptr, &global_dst_unit_id, &gaspi_dst_seg_id, "dart_put"));

    dart_global_unit_t global_my_unit_id;
    DART_CHECK_ERROR(dart_myid(&global_my_unit_id));
    if(global_my_unit_id.id == global_dst_unit_id)
    {
        DART_CHECK_ERROR(local_copy_put(&gptr, gaspi_dst_seg_id, (void*) src, nbytes));
        return DART_OK;
    }

    // get gaspi segment id and bind it to dst
    gaspi_segment_id_t free_seg_id;
    DART_CHECK_ERROR(seg_stack_pop(&dart_free_coll_seg_ids, &free_seg_id));
    DART_CHECK_GASPI_ERROR(gaspi_segment_bind(free_seg_id, src, nbytes, 0));

    //communitcation request
    char found_rma_req = 0;
    gaspi_queue_id_t   queue = 0;
    DART_CHECK_ERROR(find_rma_request(gptr.unitid, gptr.segid, &queue, &found_rma_req));
    if(!found_rma_req)
    {
        DART_CHECK_ERROR(dart_get_minimal_queue(&queue));
        DART_CHECK_ERROR(add_rma_request_entry(gptr.unitid, gptr.unitid, queue));
    }
    else
    {
        DART_CHECK_GASPI_ERROR(check_queue_size(queue));
    }

    DART_CHECK_GASPI_ERROR(gaspi_write(free_seg_id,
                                      0,
                                      global_dst_unit_id,
                                      gaspi_dst_seg_id,
                                      gptr.addr_or_offs.offset,
                                      nbytes,
                                      queue,
                                      GASPI_BLOCK));

   return DART_OK;
}


dart_ret_t dart_allreduce(
  const void       * sendbuf,
  void             * recvbuf,
  size_t             nelem,
  dart_datatype_t    dtype,
  dart_operation_t   op,
  dart_team_t        team)
{
  dart_team_unit_t        myid;
  size_t                  team_size;
  uint16_t                index;
  size_t                  elem_size = dart_gaspi_datatype_sizeof(dtype);
  DART_CHECK_ERROR(dart_team_myid(team, &myid));
  DART_CHECK_ERROR(dart_team_size(team, &team_size));

  if(dart_adapt_teamlist_convert(team, &index) == -1)
  {
    DART_LOG_ERROR(stderr, "dart_allreduce: can't find index of given team\n");
    return DART_ERR_OTHER;
  }
  /*
   * while DART supports 9 different datatypes when this was written
   * gaspi only supports 6. To keep DART compatible and easy to use
   * the gaspi_data_type isn't used in this case.
   */
  gaspi_reduce_state_t reduce_state = GASPI_STATE_HEALTHY;
  dart_ret_t ret = DART_OK;
  gaspi_group_t gaspi_group_id = dart_teams[index].id;
  switch (op) {
     case DART_OP_MINMAX:
     switch(dtype){
        case DART_TYPE_SHORT:
          gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_MINMAX_short,
                               reduce_state, gaspi_group_id, GASPI_BLOCK);
          break;
        case DART_TYPE_INT:
          gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_MINMAX_int,
                               reduce_state, gaspi_group_id, GASPI_BLOCK);
          break;
        case DART_TYPE_BYTE:
          gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_MINMAX_char,
                               reduce_state, gaspi_group_id, GASPI_BLOCK);
          break;
        case DART_TYPE_UINT:
          gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_MINMAX_uInt,
                               reduce_state, gaspi_group_id, GASPI_BLOCK);
          break;
        case DART_TYPE_LONG:
          gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_MINMAX_long,
                               reduce_state, gaspi_group_id, GASPI_BLOCK);
          break;
        case DART_TYPE_ULONG:
          gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_MINMAX_uLong,
                               reduce_state, gaspi_group_id, GASPI_BLOCK);
          break;
        case DART_TYPE_LONGLONG:
          gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_MINMAX_longLong,
                               reduce_state, gaspi_group_id, GASPI_BLOCK);
          break;
        case DART_TYPE_FLOAT:
          gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_MINMAX_float,
                               reduce_state, gaspi_group_id, GASPI_BLOCK);
          break;
        case DART_TYPE_DOUBLE:
          gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_MINMAX_double,
                               reduce_state, gaspi_group_id, GASPI_BLOCK);
          break;
        default: DART_LOG_ERROR("ERROR: Datatype not supported for DART_OP_MINMAX!!\n");
                 ret = DART_ERR_INVAL;
                 break;

     }
     break;
     case DART_OP_MIN:
     switch(dtype){
        case DART_TYPE_SHORT:
          gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_MIN_short,
                               reduce_state, gaspi_group_id, GASPI_BLOCK);
          break;
        case DART_TYPE_INT:
          gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_MIN_int,
                               reduce_state, gaspi_group_id, GASPI_BLOCK);
          break;
        case DART_TYPE_BYTE:
          gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_MIN_char,
                               reduce_state, gaspi_group_id, GASPI_BLOCK);
          break;
        case DART_TYPE_UINT:
          gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_MIN_uInt,
                               reduce_state, gaspi_group_id, GASPI_BLOCK);
          break;
        case DART_TYPE_LONG:
          gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_MIN_long,
                               reduce_state, gaspi_group_id, GASPI_BLOCK);
          break;
        case DART_TYPE_ULONG:
          gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_MIN_uLong,
                               reduce_state, gaspi_group_id, GASPI_BLOCK);
          break;
        case DART_TYPE_LONGLONG:
          gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_MIN_longLong,
                               reduce_state, gaspi_group_id, GASPI_BLOCK);
          break;
        case DART_TYPE_FLOAT:
          gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_MIN_float,
                               reduce_state, gaspi_group_id, GASPI_BLOCK);
          break;
        case DART_TYPE_DOUBLE:
          gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_MIN_double,
                               reduce_state, gaspi_group_id, GASPI_BLOCK);
          break;
        default: DART_LOG_ERROR("ERROR: Datatype not supported for DART_OP_MIN!!\n");
                 ret = DART_ERR_INVAL;
                 break;
       }
       break;
     case DART_OP_MAX:
     switch(dtype){
          case DART_TYPE_SHORT:
            gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                                 elem_size, gaspi_op_MAX_short,
                                 reduce_state, gaspi_group_id, GASPI_BLOCK);
            break;
          case DART_TYPE_INT:
            gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                                 elem_size, gaspi_op_MAX_int,
                                 reduce_state, gaspi_group_id, GASPI_BLOCK);
            break;
          case DART_TYPE_BYTE:
            gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                                 elem_size, gaspi_op_MAX_char,
                                 reduce_state, gaspi_group_id, GASPI_BLOCK);
            break;
          case DART_TYPE_UINT:
            gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                                 elem_size, gaspi_op_MAX_uInt,
                                 reduce_state, gaspi_group_id, GASPI_BLOCK);
            break;
          case DART_TYPE_LONG:
            gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                                 elem_size, gaspi_op_MAX_long,
                                 reduce_state, gaspi_group_id, GASPI_BLOCK);
            break;
          case DART_TYPE_ULONG:
            gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                                 elem_size, gaspi_op_MAX_uLong,
                                 reduce_state, gaspi_group_id, GASPI_BLOCK);
            break;
          case DART_TYPE_LONGLONG:
            gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                                 elem_size, gaspi_op_MAX_longLong,
                                 reduce_state, gaspi_group_id, GASPI_BLOCK);
            break;
          case DART_TYPE_FLOAT:
            gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                                 elem_size, gaspi_op_MAX_float,
                                 reduce_state, gaspi_group_id, GASPI_BLOCK);
            break;
          case DART_TYPE_DOUBLE:
            gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                                 elem_size, gaspi_op_MAX_double,
                                 reduce_state, gaspi_group_id, GASPI_BLOCK);
            break;
          default: DART_LOG_ERROR("ERROR: Datatype not supported for DART_OP_MAX!\n");
                   ret = DART_ERR_INVAL;
                   break;
       }
       break;
     case DART_OP_SUM:
     switch(dtype){
        case DART_TYPE_SHORT:
          gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_SUM_short,
                               reduce_state, gaspi_group_id, GASPI_BLOCK);
          break;
        case DART_TYPE_INT:
          gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_SUM_int,
                               reduce_state, gaspi_group_id, GASPI_BLOCK);
          break;
        case DART_TYPE_BYTE:
          gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_SUM_char,
                               reduce_state, gaspi_group_id, GASPI_BLOCK);
          break;
        case DART_TYPE_UINT:
          gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_SUM_uInt,
                               reduce_state, gaspi_group_id, GASPI_BLOCK);
          break;
        case DART_TYPE_LONG:
          gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_SUM_long,
                               reduce_state, gaspi_group_id, GASPI_BLOCK);
          break;
        case DART_TYPE_ULONG:
          gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_SUM_uLong,
                               reduce_state, gaspi_group_id, GASPI_BLOCK);
          break;
        case DART_TYPE_LONGLONG:
          gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_SUM_longLong,
                               reduce_state, gaspi_group_id, GASPI_BLOCK);
          break;
        case DART_TYPE_FLOAT:
          gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_SUM_float,
                               reduce_state, gaspi_group_id, GASPI_BLOCK);
          break;
        case DART_TYPE_DOUBLE:
          gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_SUM_double,
                               reduce_state, gaspi_group_id, GASPI_BLOCK);
          break;
        default: DART_LOG_ERROR("ERROR: Datatype not supported for DART_OP_SUM!\n");
                 ret = DART_ERR_INVAL;
                 break;
     }
     break;
     case DART_OP_PROD:
       switch(dtype){
         case DART_TYPE_SHORT:
         gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                              elem_size, gaspi_op_PROD_short,
                              reduce_state, gaspi_group_id, GASPI_BLOCK);
         break;
       case DART_TYPE_INT:
         gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                              elem_size, gaspi_op_PROD_int,
                              reduce_state, gaspi_group_id, GASPI_BLOCK);
         break;
       case DART_TYPE_BYTE:
         gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                              elem_size, gaspi_op_PROD_char,
                              reduce_state, gaspi_group_id, GASPI_BLOCK);
         break;
       case DART_TYPE_UINT:
         gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                              elem_size, gaspi_op_PROD_uInt,
                              reduce_state, gaspi_group_id, GASPI_BLOCK);
         break;
       case DART_TYPE_LONG:
         gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                              elem_size, gaspi_op_PROD_long,
                              reduce_state, gaspi_group_id, GASPI_BLOCK);
         break;
       case DART_TYPE_ULONG:
         gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                              elem_size, gaspi_op_PROD_uLong,
                              reduce_state, gaspi_group_id, GASPI_BLOCK);
         break;
       case DART_TYPE_LONGLONG:
         gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                              elem_size, gaspi_op_PROD_longLong,
                              reduce_state, gaspi_group_id, GASPI_BLOCK);
         break;
       case DART_TYPE_FLOAT:
         gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                              elem_size, gaspi_op_PROD_float,
                              reduce_state, gaspi_group_id, GASPI_BLOCK);
         break;
       case DART_TYPE_DOUBLE:
         gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                              elem_size, gaspi_op_PROD_double,
                              reduce_state, gaspi_group_id, GASPI_BLOCK);
         break;
       default: DART_LOG_ERROR("ERROR: Datatype not supported for DART_OP_PROD!\n");
                ret = DART_ERR_INVAL;
                break;
     }

     break;
     case DART_OP_BAND:
     switch(dtype){
       case DART_TYPE_BYTE:
         gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                              elem_size, gaspi_op_BAND_char,
                              reduce_state, gaspi_group_id, GASPI_BLOCK);
         break;
       case DART_TYPE_INT:
         gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                              elem_size, gaspi_op_BAND_int,
                              reduce_state, gaspi_group_id, GASPI_BLOCK);
         break;
       default: DART_LOG_ERROR("ERROR: Datatype not supported for DART_OP_BAND!\n");
                ret = DART_ERR_INVAL;
                break;
     }
     break;
     case DART_OP_LAND:
     switch(dtype){
      case DART_TYPE_INT:
         gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                              elem_size, gaspi_op_LAND_int,
                              reduce_state, gaspi_group_id, GASPI_BLOCK);
         break;
      default: DART_LOG_ERROR("ERROR: Datatype not supported for DART_OP_PROD!\n");
                ret = DART_ERR_INVAL;
                break;
     }
     break;
     case DART_OP_BOR:
     switch(dtype){
       case DART_TYPE_BYTE:
         gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                              elem_size, gaspi_op_BOR_char,
                              reduce_state, gaspi_group_id, GASPI_BLOCK);
         break;
       case DART_TYPE_INT:
         gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                              elem_size, gaspi_op_BOR_int,
                              reduce_state, gaspi_group_id, GASPI_BLOCK);
         break;
       default: DART_LOG_ERROR("ERROR: Datatype not supported for DART_OP_BAND!\n");
                ret = DART_ERR_INVAL;
                break;
     }
     break;
     case DART_OP_LOR:
     switch(dtype){
       case DART_TYPE_BYTE:
         gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                              elem_size, gaspi_op_LOR_char,
                              reduce_state, gaspi_group_id, GASPI_BLOCK);
         break;
       case DART_TYPE_INT:
         gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                              elem_size, gaspi_op_LOR_int,
                              reduce_state, gaspi_group_id, GASPI_BLOCK);
         break;
       default: DART_LOG_ERROR("ERROR: Datatype not supported for DART_OP_BAND!\n");
                ret = DART_ERR_INVAL;
                break;
     }
     break;
     case DART_OP_BXOR:
     switch(dtype){
       case DART_TYPE_BYTE:
         gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                              elem_size, gaspi_op_BXOR_char,
                              reduce_state, gaspi_group_id, GASPI_BLOCK);
         break;
       case DART_TYPE_INT:
         gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                              elem_size, gaspi_op_BXOR_int,
                              reduce_state, gaspi_group_id, GASPI_BLOCK);
         break;
       default: DART_LOG_ERROR("ERROR: Datatype not supported for DART_OP_BAND!\n");
                ret = DART_ERR_INVAL;
                break;
     }
     break;
     case DART_OP_LXOR:
     switch(dtype){
      case DART_TYPE_INT:
         gaspi_allreduce_user(sendbuf, recvbuf, nelem,
                              elem_size, gaspi_op_LAND_int,
                              reduce_state, gaspi_group_id, GASPI_BLOCK);
         break;
      default: DART_LOG_ERROR("ERROR: Datatype not supported for DART_OP_PROD!\n");
               ret = DART_ERR_INVAL;
               break;
     }
     break;
     default: DART_LOG_ERROR(stderr, "dart_allreduce: operation not supported!\n" );
              ret = DART_ERR_INVAL;
  }
  return ret;
}

dart_ret_t dart_reduce(
  const void       * sendbuf,
  void             * recvbuf,
  size_t             nelem,
  dart_datatype_t    dtype,
  dart_operation_t   op,
  dart_team_unit_t   root,
  dart_team_t        team)
{
  dart_team_unit_t        myid;
  size_t                  team_size;
  gaspi_segment_id_t      gaspi_seg_id = dart_gaspi_buffer_id;
  gaspi_pointer_t         seg_ptr      = NULL;
  uint16_t                index;
  size_t                  elem_size = dart_gaspi_datatype_sizeof(dtype);
  DART_CHECK_ERROR(dart_team_myid(team, &myid));
  DART_CHECK_ERROR(dart_team_size(team, &team_size));

  if(dart_adapt_teamlist_convert(team, &index) == -1)
  {
    DART_LOG_ERROR(stderr, "dart_reduce: can't find index of given team\n");
    return DART_ERR_OTHER;
  }

  DART_CHECK_GASPI_ERROR(gaspi_segment_ptr(gaspi_seg_id, &seg_ptr));
  gaspi_segment_id_t* segment_ids = (gaspi_segment_id_t*) seg_ptr;

  for(size_t i = 0; i < team_size; i++)
  {
      segment_ids[i]=dart_fallback_seg;
  }

  /*
   * while DART supports 9 different datatypes when this was written
   * gaspi only supports 6. To keep DART compatible and easy to use
   * the gaspi_data_type isn't used in this case.
   */

  gaspi_reduce_state_t reduce_state = GASPI_STATE_HEALTHY;
  dart_ret_t ret = DART_OK;
  gaspi_group_t gaspi_group_id = dart_teams[index].id;
  dart_unit_t gaspi_root_proc;
  DART_CHECK_ERROR(unit_l2g(index, &gaspi_root_proc, root.id));

  switch (op) {
     case DART_OP_MIN:
     switch(dtype){
        case DART_TYPE_SHORT:
          gaspi_reduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_MIN_short,
                               reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
          break;
        case DART_TYPE_INT:
          gaspi_reduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_MIN_int,
                               reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
          break;
        case DART_TYPE_BYTE:
          gaspi_reduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_MIN_char,
                               reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
          break;
        case DART_TYPE_UINT:
          gaspi_reduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_MIN_uInt,
                               reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
          break;
        case DART_TYPE_LONG:
          gaspi_reduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_MIN_long,
                               reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
          break;
        case DART_TYPE_ULONG:
          gaspi_reduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_MIN_uLong,
                               reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
          break;
        case DART_TYPE_LONGLONG:
          gaspi_reduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_MIN_longLong,
                               reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
          break;
        case DART_TYPE_FLOAT:
          gaspi_reduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_MIN_float,
                               reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
          break;
        case DART_TYPE_DOUBLE:
          gaspi_reduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_MIN_double,
                               reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
          break;
        default: DART_LOG_ERROR("ERROR: Datatype not supported for DART_OP_MIN!!\n");
                 ret = DART_ERR_INVAL;
                 break;
       }
       break;
     case DART_OP_MAX:
     switch(dtype){
          case DART_TYPE_SHORT:
            gaspi_reduce_user(sendbuf, recvbuf, nelem,
                                 elem_size, gaspi_op_MAX_short,
                                 reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
            break;
          case DART_TYPE_INT:
            gaspi_reduce_user(sendbuf, recvbuf, nelem,
                                 elem_size, gaspi_op_MAX_int,
                                 reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
            break;
          case DART_TYPE_BYTE:
            gaspi_reduce_user(sendbuf, recvbuf, nelem,
                                 elem_size, gaspi_op_MAX_char,
                                 reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
            break;
          case DART_TYPE_UINT:
            gaspi_reduce_user(sendbuf, recvbuf, nelem,
                                 elem_size, gaspi_op_MAX_uInt,
                                 reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
            break;
          case DART_TYPE_LONG:
            gaspi_reduce_user(sendbuf, recvbuf, nelem,
                                 elem_size, gaspi_op_MAX_long,
                                 reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
            break;
          case DART_TYPE_ULONG:
            gaspi_reduce_user(sendbuf, recvbuf, nelem,
                                 elem_size, gaspi_op_MAX_uLong,
                                 reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
            break;
          case DART_TYPE_LONGLONG:
            gaspi_reduce_user(sendbuf, recvbuf, nelem,
                                 elem_size, gaspi_op_MAX_longLong,
                                 reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
            break;
          case DART_TYPE_FLOAT:
            gaspi_reduce_user(sendbuf, recvbuf, nelem,
                                 elem_size, gaspi_op_MAX_float,
                                 reduce_state, gaspi_group_id, segment_ids ,gaspi_root_proc, GASPI_BLOCK);
            break;
          case DART_TYPE_DOUBLE:
            gaspi_reduce_user(sendbuf, recvbuf, nelem,
                                 elem_size, gaspi_op_MAX_double,
                                 reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
            break;
          default: DART_LOG_ERROR("ERROR: Datatype not supported for DART_OP_MAX!\n");
                   ret = DART_ERR_INVAL;
                   break;
       }
       break;
     case DART_OP_SUM:
     switch(dtype){
        case DART_TYPE_SHORT:
          gaspi_reduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_SUM_short,
                               reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
          break;
        case DART_TYPE_INT:
          gaspi_reduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_SUM_int,
                               reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
          break;
        case DART_TYPE_BYTE:
          gaspi_reduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_SUM_char,
                               reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
          break;
        case DART_TYPE_UINT:
          gaspi_reduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_SUM_uInt,
                               reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
          break;
        case DART_TYPE_LONG:
          gaspi_reduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_SUM_long,
                               reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
          break;
        case DART_TYPE_ULONG:
          gaspi_reduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_SUM_uLong,
                               reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
          break;
        case DART_TYPE_LONGLONG:
          gaspi_reduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_SUM_longLong,
                               reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
          break;
        case DART_TYPE_FLOAT:
          gaspi_reduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_SUM_float,
                               reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
          break;
        case DART_TYPE_DOUBLE:
          gaspi_reduce_user(sendbuf, recvbuf, nelem,
                               elem_size, gaspi_op_SUM_double,
                               reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
          break;
        default: DART_LOG_ERROR("ERROR: Datatype not supported for DART_OP_SUM!\n");
                 ret = DART_ERR_INVAL;
                 break;
     }
     break;
     case DART_OP_PROD:
       switch(dtype){
         case DART_TYPE_SHORT:
         gaspi_reduce_user(sendbuf, recvbuf, nelem,
                              elem_size, gaspi_op_PROD_short,
                              reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
         break;
       case DART_TYPE_INT:
         gaspi_reduce_user(sendbuf, recvbuf, nelem,
                              elem_size, gaspi_op_PROD_int,
                              reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
         break;
       case DART_TYPE_BYTE:
         gaspi_reduce_user(sendbuf, recvbuf, nelem,
                              elem_size, gaspi_op_PROD_char,
                              reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
         break;
       case DART_TYPE_UINT:
         gaspi_reduce_user(sendbuf, recvbuf, nelem,
                              elem_size, gaspi_op_PROD_uInt,
                              reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
         break;
       case DART_TYPE_LONG:
         gaspi_reduce_user(sendbuf, recvbuf, nelem,
                              elem_size, gaspi_op_PROD_long,
                              reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
         break;
       case DART_TYPE_ULONG:
         gaspi_reduce_user(sendbuf, recvbuf, nelem,
                              elem_size, gaspi_op_PROD_uLong,
                              reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
         break;
       case DART_TYPE_LONGLONG:
         gaspi_reduce_user(sendbuf, recvbuf, nelem,
                              elem_size, gaspi_op_PROD_longLong,
                              reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
         break;
       case DART_TYPE_FLOAT:
         gaspi_reduce_user(sendbuf, recvbuf, nelem,
                              elem_size, gaspi_op_PROD_float,
                              reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
         break;
       case DART_TYPE_DOUBLE:
         gaspi_reduce_user(sendbuf, recvbuf, nelem,
                              elem_size, gaspi_op_PROD_double,
                              reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
         break;
       default: DART_LOG_ERROR("ERROR: Datatype not supported for DART_OP_PROD!\n");
                ret = DART_ERR_INVAL;
                break;
     }

     break;
     case DART_OP_BAND:
     switch(dtype){
       case DART_TYPE_BYTE:
         gaspi_reduce_user(sendbuf, recvbuf, nelem,
                              elem_size, gaspi_op_BAND_char,
                              reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
         break;
       case DART_TYPE_INT:
         gaspi_reduce_user(sendbuf, recvbuf, nelem,
                              elem_size, gaspi_op_BAND_int,
                              reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
         break;
       default: DART_LOG_ERROR("ERROR: Datatype not supported for DART_OP_BAND!\n");
                ret = DART_ERR_INVAL;
                break;
     }
     break;
     case DART_OP_LAND:
     switch(dtype){
      case DART_TYPE_INT:
         gaspi_reduce_user(sendbuf, recvbuf, nelem,
                              elem_size, gaspi_op_LAND_int,
                              reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
         break;
      default: DART_LOG_ERROR("ERROR: Datatype not supported for DART_OP_PROD!\n");
                ret = DART_ERR_INVAL;
                break;
     }
     break;
     case DART_OP_BOR:
     switch(dtype){
       case DART_TYPE_BYTE:
         gaspi_reduce_user(sendbuf, recvbuf, nelem,
                              elem_size, gaspi_op_BOR_char,
                              reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
         break;
       case DART_TYPE_INT:
         gaspi_reduce_user(sendbuf, recvbuf, nelem,
                              elem_size, gaspi_op_BOR_int,
                              reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
         break;
       default: DART_LOG_ERROR("ERROR: Datatype not supported for DART_OP_BAND!\n");
                ret = DART_ERR_INVAL;
                break;
     }
     break;
     case DART_OP_LOR:
     switch(dtype){
       case DART_TYPE_BYTE:
         gaspi_reduce_user(sendbuf, recvbuf, nelem,
                              elem_size, gaspi_op_LOR_char,
                              reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
         break;
       case DART_TYPE_INT:
         gaspi_reduce_user(sendbuf, recvbuf, nelem,
                              elem_size, gaspi_op_LOR_int,
                              reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
         break;
       default: DART_LOG_ERROR("ERROR: Datatype not supported for DART_OP_BAND!\n");
                ret = DART_ERR_INVAL;
                break;
     }
     break;
     case DART_OP_BXOR:
     switch(dtype){
       case DART_TYPE_BYTE:
         gaspi_reduce_user(sendbuf, recvbuf, nelem,
                              elem_size, gaspi_op_BXOR_char,
                              reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
         break;
       case DART_TYPE_INT:
         gaspi_reduce_user(sendbuf, recvbuf, nelem,
                              elem_size, gaspi_op_BXOR_int,
                              reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
         break;
       default: DART_LOG_ERROR("ERROR: Datatype not supported for DART_OP_BAND!\n");
                ret = DART_ERR_INVAL;
                break;
     }
     break;
     case DART_OP_LXOR:
     switch(dtype){
      case DART_TYPE_INT:
         gaspi_reduce_user(sendbuf, recvbuf, nelem,
                              elem_size, gaspi_op_LAND_int,
                              reduce_state, gaspi_group_id, segment_ids, gaspi_root_proc, GASPI_BLOCK);
         break;
      default: DART_LOG_ERROR("ERROR: Datatype not supported for DART_OP_PROD!\n");
               ret = DART_ERR_INVAL;
               break;
     }
     break;
     default: DART_LOG_ERROR(stderr, "dart_reduce: operation not supported!\n" );
              ret = DART_ERR_INVAL;
  }
  //DART_CHECK_ERROR(seg_stack_push(&dart_free_coll_seg_ids, free_id));
  return ret;
}

dart_ret_t dart_recv(
  void                * recvbuf,
  size_t                nelem,
  dart_datatype_t       dtype,
  int                   tag,
  dart_global_unit_t    unit)
{
    DART_LOG_ERROR("dart_recv for gaspi not supported!");
    printf("dart_recv for gaspi not supported!\n");
    return DART_ERR_INVAL;
}

dart_ret_t dart_send(
  const void         * sendbuf,
  size_t               nelem,
  dart_datatype_t      dtype,
  int                  tag,
  dart_global_unit_t   unit)
{
    DART_LOG_ERROR("dart_send for gaspi not supported!");
    printf("dart_send for gaspi not supported!\n");
    return DART_ERR_INVAL;
}

dart_ret_t dart_sendrecv(
  const void         * sendbuf,
  size_t               send_nelem,
  dart_datatype_t      send_dtype,
  int                  send_tag,
  dart_global_unit_t   dest,
  void               * recvbuf,
  size_t               recv_nelem,
  dart_datatype_t      recv_dtype,
  int                  recv_tag,
  dart_global_unit_t   src)
{
    DART_LOG_ERROR("dart_sendrecv for gaspi not supported!");
    printf("dart_send for gaspi not supported!\n");
    return DART_ERR_INVAL;
}

//Needs ro be implemented
dart_ret_t dart_fetch_and_op(
  dart_gptr_t      gptr,
  const void *     value,
  void *           result,
  dart_datatype_t  dtype,
  dart_operation_t op)
{

  DART_LOG_ERROR("dart_fetch_and_op for gaspi not supported!");
  printf("dart_fetch_and_op for gaspi not supported!\n");
  return DART_ERR_INVAL;
}

//Needs to be implemented
dart_ret_t dart_accumulate(
  dart_gptr_t      gptr,
  const void *     value,
  size_t           nelem,
  dart_datatype_t  dtype,
  dart_operation_t op)
{
    printf("Entering dart_accumulate (gaspi)\n");

    void *     rec_value;
    auto teamid = gptr.teamid;
    auto segid = gptr.segid;

    dart_team_unit_t myrelid;
    dart_global_unit_t myid;
    dart_team_t myteamid;
    dart_myid(&myid);
    dart_team_myid(teamid, &myrelid);
    // custom reduction op`s ausschließen

    // check if dtype is basic type

    // convert dart_op to gaspi_op

    // get team id

    // get segment

    // get window?

    // prepare chunks -> need for maximum element value

    // here mpi calls mpi_accumulate
    dart_reduce(value, rec_value, nelem, dtype, op, myrelid, teamid);
    //

    return DART_OK;
}

dart_ret_t dart_accumulate_blocking_local(
    dart_gptr_t      gptr,
    const void     * values,
    size_t           nelem,
    dart_datatype_t  dtype,
    dart_operation_t op)
{
    DART_LOG_ERROR("dart_accumulate_blocking_local for gaspi not supported!");
    printf("dart_accumulate_blocking_local for gaspi not supported!\n");
    return DART_ERR_INVAL;
}

dart_ret_t dart_compare_and_swap(
    dart_gptr_t      gptr,
    const void     * value,
    const void     * compare,
    void           * result,
    dart_datatype_t  dtype)
{
    DART_LOG_ERROR("dart_compare_and_swap for gaspi not supported!");
    printf("dart_compare_and_swap for gaspi not supported!\n");
    return DART_ERR_INVAL;
}



// ===== Functions by Rodario =====
// copied from the mpi implementation of "dart_communication.c"
dart_ret_t dart_handle_free(
  dart_handle_t * handleptr)
{
  if (handleptr != NULL && *handleptr != DART_HANDLE_NULL) {
    free(*handleptr);
    *handleptr = DART_HANDLE_NULL;
  }
  return DART_OK;
}

dart_ret_t dart_alltoall(
    const void *    sendbuf,
    void *          recvbuf,
    size_t          nelem,
    dart_datatype_t dtype,
    dart_team_t     teamid)
{
    DART_LOG_ERROR("dart_alltoall for gaspi not supported!");
    printf("dart_alltoall for gaspi not supported!\n");
    return DART_ERR_INVAL;
}