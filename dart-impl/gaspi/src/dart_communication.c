
#include <dash/dart/gaspi/dart_gaspi.h>

#include <dash/dart/if/dart_communication.h>
#include <dash/dart/gaspi/dart_team_private.h>
#include <dash/dart/gaspi/dart_translation.h>
#include <dash/dart/gaspi/dart_mem.h>
#include <dash/dart/gaspi/dart_communication_priv.h>
#include <string.h>
/*********************** Notify Value ************************/
gaspi_notification_t dart_notify_value = 42;


/********************** Only for testing *********************/
gaspi_queue_id_t dart_handle_get_queue(dart_handle_t handle)
{
    return handle->queue;
}

dart_ret_t dart_scatter(void *sendbuf, void *recvbuf, size_t nbytes, dart_unit_t root, dart_team_t team)
{
    dart_unit_t             myid;
    size_t                  team_size;
    gaspi_notification_id_t first_id;
    gaspi_notification_t    old_value;
    gaspi_segment_id_t      gaspi_seg_id = dart_gaspi_buffer_id;
    gaspi_notification_id_t remote_id    = 0;
    gaspi_pointer_t         seg_ptr      = NULL;
    gaspi_queue_id_t        queue        = 0;
    gaspi_offset_t          local_offset = 0;
    uint16_t                index;

    DART_CHECK_ERROR(dart_team_myid(team, &myid));
    DART_CHECK_ERROR(dart_team_size(team, &team_size));

    if(dart_adapt_teamlist_convert(team, &index) == -1)
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

    DART_CHECK_ERROR(dart_barrier(team));
    DART_CHECK_GASPI_ERROR(gaspi_segment_ptr(gaspi_seg_id, &seg_ptr));

    if(myid == root)
    {
        memcpy( seg_ptr, sendbuf, nbytes * team_size );

        for (dart_unit_t unit = 0; unit < team_size; ++unit)
        {
            if ( unit == myid )
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

        memcpy(recvbuf, (void *) ((char *) seg_ptr + (myid * nbytes)), nbytes);
    }
    else
    {
        DART_CHECK_GASPI_ERROR(gaspi_notify_waitsome(gaspi_seg_id, remote_id, 1, &first_id, GASPI_BLOCK));
        DART_CHECK_GASPI_ERROR(gaspi_notify_reset(gaspi_seg_id, first_id, &old_value));

        memcpy(recvbuf, seg_ptr, nbytes);
    }

    DART_CHECK_ERROR(dart_barrier(team));

    if((nbytes * team_size) > DART_GASPI_BUFFER_SIZE)
    {
        DART_CHECK_GASPI_ERROR(gaspi_segment_delete(gaspi_seg_id));
        dart_fallback_seg_is_allocated = false;
    }


    return DART_OK;
}

dart_ret_t dart_gather(void *sendbuf, void *recvbuf, size_t nbytes, dart_unit_t root, dart_team_t team)
{
    uint16_t                index;
    dart_unit_t             relative_id;
    size_t                  team_size;
    gaspi_notification_id_t first_id;
    gaspi_notification_t    old_value;
    gaspi_segment_id_t      gaspi_seg_id  = dart_gaspi_buffer_id;
    gaspi_notification_t    notify_value  = 42;
    gaspi_pointer_t         seg_ptr       = NULL;
    gaspi_queue_id_t        queue         = 0;
    gaspi_offset_t          remote_offset = 0;

    if(dart_adapt_teamlist_convert(team, &index) == -1)
    {
        fprintf(stderr, "dart_gather: can't find index of given team\n");
        return DART_ERR_OTHER;
    }

    DART_CHECK_ERROR(dart_team_myid(team, &relative_id));
    DART_CHECK_ERROR(dart_team_size(team, &team_size));

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

    DART_CHECK_ERROR(dart_barrier(team));

    if(relative_id != root)
    {
        dart_unit_t abs_root_id;
        DART_CHECK_ERROR(unit_l2g(index, &abs_root_id, root));

        memcpy(seg_ptr, sendbuf, nbytes);
        remote_offset = relative_id * nbytes;

        DART_CHECK_GASPI_ERROR(wait_for_queue_entries(&queue, 2));
        DART_CHECK_GASPI_ERROR(gaspi_write_notify(gaspi_seg_id,
                                                  0UL,
                                                  abs_root_id,
                                                  gaspi_seg_id,
                                                  remote_offset,
                                                  nbytes,
                                                  relative_id,
                                                  notify_value,
                                                  queue,
                                                  GASPI_BLOCK));
    }
    else
    {
        gaspi_offset_t recv_buffer_offset = relative_id * nbytes;
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

    DART_CHECK_ERROR(dart_barrier(team));

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
 */
dart_ret_t dart_bcast(void *buf, size_t nbytes, dart_unit_t root, dart_team_t team)
{
    const gaspi_notification_id_t notify_id    = 0;
    gaspi_queue_id_t              queue        = 0;
    gaspi_pointer_t               seg_ptr      = NULL;
    const gaspi_notification_t    notify_val   = 42;
    gaspi_segment_id_t            gaspi_seg_id = dart_gaspi_buffer_id;
    gaspi_notification_id_t       first_id;
    gaspi_notification_t          old_value;
    uint16_t                      index;
    dart_unit_t                   myid;
    dart_unit_t                   root_abs;
    dart_unit_t                   team_myid;
    size_t                        team_size;
    int                           parent;
    int                         * children = NULL;
    int                           children_count;

    int result = dart_adapt_teamlist_convert(team, &index);
    if(result == -1)
    {
        fprintf(stderr, "dart_bcast: can't find index of given team\n");
        return DART_ERR_INVAL;
    }

    DART_CHECK_ERROR(unit_l2g(index, &root_abs, root));
    DART_CHECK_ERROR(dart_myid(&myid));
    DART_CHECK_ERROR(dart_team_myid(team, &team_myid));
    DART_CHECK_ERROR(dart_team_size(team, &team_size));
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

    if(myid == root_abs)
    {
        memcpy(seg_ptr, buf, nbytes);
    }

    children_count = gaspi_utils_compute_comms(&parent, &children, team_myid, root, team_size);

    DART_CHECK_ERROR(dart_barrier(team));

    dart_unit_t abs_parent;
    DART_CHECK_ERROR(unit_l2g(index, &abs_parent, parent));
    /*
     * parents + children wait for upper parents data
     */
    if (myid != abs_parent)
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
    DART_CHECK_ERROR(dart_barrier(team));

    if(myid != root_abs)
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

dart_ret_t dart_allgather(void *sendbuf, void *recvbuf, size_t nbytes, dart_team_t team)
{
    gaspi_queue_id_t     queue = 0;
    gaspi_notification_t notify_value = 42;
    gaspi_segment_id_t   gaspi_seg_id = dart_gaspi_buffer_id;
    gaspi_pointer_t      seg_ptr      = NULL;
    dart_unit_t          relative_id;
    gaspi_offset_t       offset;
    size_t               teamsize;
    uint16_t             index;

    DART_CHECK_ERROR(dart_team_myid(team, &relative_id));
    DART_CHECK_ERROR(dart_team_size(team, &teamsize));
    DART_CHECK_ERROR(dart_barrier(team));

    int result = dart_adapt_teamlist_convert(team, &index);
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


    offset = nbytes * relative_id;

    DART_CHECK_GASPI_ERROR(gaspi_segment_ptr(gaspi_seg_id, &seg_ptr));

    memcpy((void *) ((char *)seg_ptr + offset), sendbuf, nbytes);

    for (dart_unit_t unit = 0; unit < teamsize; unit++)
    {
        if(unit == relative_id) continue;

        dart_unit_t unit_abs_id;
        DART_CHECK_ERROR(unit_l2g(index, &unit_abs_id, unit));

        DART_CHECK_GASPI_ERROR(wait_for_queue_entries(&queue, 2));
        DART_CHECK_GASPI_ERROR(gaspi_write_notify(gaspi_seg_id,
                                                  offset,
                                                  unit_abs_id,
                                                  gaspi_seg_id,
                                                  offset,
                                                  nbytes,
                                                  (gaspi_notification_id_t) relative_id,
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
    DART_CHECK_ERROR(dart_barrier(team));

    if((nbytes * teamsize) > DART_GASPI_BUFFER_SIZE)
    {
        DART_CHECK_GASPI_ERROR(gaspi_segment_delete(gaspi_seg_id));
        dart_fallback_seg_is_allocated = false;
    }

    return DART_OK;
}

dart_ret_t dart_barrier (dart_team_t teamid)
{
    uint16_t       index;
    gaspi_group_t  gaspi_group_id;

    int result = dart_adapt_teamlist_convert (teamid, &index);
    if (result == -1)
    {
        return DART_ERR_INVAL;
    }
    gaspi_group_id = dart_teams[index].id;
    DART_CHECK_GASPI_ERROR(gaspi_barrier(gaspi_group_id, GASPI_BLOCK));

    return DART_OK;
}
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
    dart_unit_t myid, rel_myid;
    if(handle == NULL)
    {
        fprintf(stderr, "dart_notify_handle : Handle structure not initialized\n");
        return DART_ERR_OTHER;
    }

    DART_CHECK_ERROR(dart_myid(&myid));
    DART_CHECK_ERROR(unit_g2l(handle->index, myid, &rel_myid));
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
    dart_unit_t        myid;
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

    DART_CHECK_ERROR(unit_g2l(index, myid, &my_rel_id));
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
    size_t                  team_size    = dart_teams[index].group.nmem;

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

dart_ret_t dart_get_blocking(void *dest, dart_gptr_t src, size_t nbytes)
{
    gaspi_queue_id_t   queue;
    dart_unit_t        my_unit;
    /*
     * local site
     * temporary buffer
     */
    dart_gptr_t        dest_gptr;
    void              *dest_ptr;
    gaspi_offset_t     dest_offset;
    gaspi_segment_id_t dest_gaspi_seg = dart_mempool_seg_localalloc;
    /*
     * remote site
     */
    gaspi_segment_id_t src_gaspi_seg  = dart_mempool_seg_localalloc;
    gaspi_offset_t     src_offset     = src.addr_or_offs.offset;
    int16_t            src_seg_id     = src.segid;
    uint16_t           src_index      = src.flags;
    dart_unit_t        target_unit    = src.unitid;

    DART_CHECK_ERROR(dart_myid(&my_unit));

    if(src_seg_id)
    {
        dart_unit_t rel_target_unit;
        DART_CHECK_ERROR(unit_g2l(src_index, target_unit, &rel_target_unit));
        if(dart_adapt_transtable_get_gaspi_seg_id(src_seg_id, rel_target_unit, &src_gaspi_seg) == -1)
        {
            fprintf(stderr, "Can't find given source segment id in dart_get_blocking\n");
            return DART_ERR_NOTFOUND;
        }
    }

    if(my_unit == target_unit)
    {
        gaspi_pointer_t src_seg_ptr = NULL;
        DART_CHECK_GASPI_ERROR(gaspi_segment_ptr(src_gaspi_seg, &src_seg_ptr));

        src_seg_ptr = (void *) ((char *) src_seg_ptr + src_offset);

        memcpy(dest, src_seg_ptr, nbytes);

        return DART_OK;
    }

    DART_CHECK_ERROR(dart_memalloc(nbytes, &dest_gptr));

    dest_offset = dest_gptr.addr_or_offs.offset;

    DART_CHECK_ERROR_GOTO(dart_error_label, dart_get_minimal_queue(&queue));

    DART_CHECK_GASPI_ERROR_GOTO(dart_error_label, gaspi_read(dest_gaspi_seg,
                                                             dest_offset,
                                                             target_unit,
                                                             src_gaspi_seg,
                                                             src_offset,
                                                             nbytes,
                                                             queue,
                                                             GASPI_BLOCK));

    DART_CHECK_GASPI_ERROR_GOTO(dart_error_label, gaspi_wait(queue, GASPI_BLOCK));

    DART_CHECK_ERROR_GOTO(dart_error_label, dart_gptr_getaddr(dest_gptr, &dest_ptr));

    memcpy(dest, dest_ptr, nbytes);

    DART_CHECK_ERROR(dart_memfree(dest_gptr));

    return DART_OK;

dart_error_label:
    DART_CHECK_ERROR(dart_memfree(dest_gptr));
    return DART_ERR_OTHER;
}

dart_ret_t dart_get_gptr(dart_gptr_t dest, dart_gptr_t src, size_t nbytes)
{
    gaspi_queue_id_t   queue;
    /*
     * local site
     */
    gaspi_segment_id_t dest_gaspi_seg = dart_mempool_seg_localalloc;
    gaspi_offset_t     dest_offset    = dest.addr_or_offs.offset;
    uint16_t           dest_index     = dest.flags;
    int16_t            dest_seg_id    = dest.segid;
    dart_unit_t        my_unit        = dest.unitid;
    /*
     * remote site
     */
    gaspi_segment_id_t src_gaspi_seg  = dart_mempool_seg_localalloc;
    gaspi_offset_t     src_offset     = src.addr_or_offs.offset;
    int16_t            src_seg_id     = src.segid;
    uint16_t           src_index      = src.flags;
    dart_unit_t        target_unit    = src.unitid;
    /*
     * local site
     */
    if(dest_seg_id)
    {
        dart_unit_t rel_my_unit;
        DART_CHECK_ERROR(unit_g2l(dest_index, my_unit, &rel_my_unit));
        if(dart_adapt_transtable_get_gaspi_seg_id(dest_seg_id, rel_my_unit, &dest_gaspi_seg) == -1)
        {
            fprintf(stderr, "Can't find given destination segment id in dart_get_blocking\n");
            return DART_ERR_NOTFOUND;
        }
    }
    /*
     * remote site
     */
    if(src_seg_id)
    {
        dart_unit_t rel_target_unit;
        DART_CHECK_ERROR(unit_g2l(src_index, target_unit, &rel_target_unit));
        if(dart_adapt_transtable_get_gaspi_seg_id(src_seg_id, rel_target_unit, &src_gaspi_seg) == -1)
        {
            fprintf(stderr, "Can't find given source segment id in dart_get_blocking\n");
            return DART_ERR_NOTFOUND;
        }
    }
    int32_t found = 0;
    DART_CHECK_ERROR(find_rma_request(target_unit, src_seg_id, &queue, &found));
    if(!found)
    {
        DART_CHECK_ERROR(dart_get_minimal_queue(&queue));
        DART_CHECK_ERROR(add_rma_request_entry(target_unit, src_seg_id, queue));
    }
    else
    {
        DART_CHECK_GASPI_ERROR(check_queue_size(queue));
    }

    DART_CHECK_GASPI_ERROR(gaspi_read(dest_gaspi_seg,
                                      dest_offset,
                                      target_unit,
                                      src_gaspi_seg,
                                      src_offset,
                                      nbytes,
                                      queue,
                                      GASPI_BLOCK));
    return DART_OK;
}


dart_ret_t dart_put_gptr(dart_gptr_t dest, dart_gptr_t src, size_t nbytes)
{
    gaspi_queue_id_t   queue;
    /*
     * local site
     */
    gaspi_segment_id_t src_gaspi_seg  = dart_mempool_seg_localalloc;
    gaspi_offset_t     src_offset     = src.addr_or_offs.offset;
    int16_t            src_seg_id     = src.segid;
    uint16_t           src_index      = src.flags;
    dart_unit_t        my_unit        = src.unitid;
    /*
     * remote site
     */
    gaspi_segment_id_t dest_gaspi_seg = dart_mempool_seg_localalloc;
    gaspi_offset_t     dest_offset    = dest.addr_or_offs.offset;
    int16_t            dest_seg_id    = dest.segid;
    uint16_t           dest_index     = dest.flags;
    dart_unit_t        target_unit    = dest.unitid;
    /*
     * local site
     */
    if(src_seg_id)
    {
        dart_unit_t rel_my_unit;
        DART_CHECK_ERROR(unit_g2l(src_index, my_unit, &rel_my_unit));
        if(dart_adapt_transtable_get_gaspi_seg_id(src_seg_id, rel_my_unit, &src_gaspi_seg) == -1)
        {
            fprintf(stderr, "Can't find given source segment id in dart_put_gptr\n");
            return DART_ERR_NOTFOUND;
        }
    }
    /*
     * remote site
     */
    if(dest_seg_id)
    {
        dart_unit_t rel_target_unit;
        DART_CHECK_ERROR(unit_g2l(dest_index, target_unit, &rel_target_unit));
        if(dart_adapt_transtable_get_gaspi_seg_id(dest_seg_id, rel_target_unit, &dest_gaspi_seg) == -1)
        {
            fprintf(stderr, "Can't find given destination segment id in dart_put_gptr\n");
            return DART_ERR_NOTFOUND;
        }
    }
    int32_t found = 0;
    DART_CHECK_ERROR(find_rma_request(target_unit, dest_seg_id, &queue, &found));
    if(!found)
    {
        DART_CHECK_ERROR(dart_get_minimal_queue(&queue));
        DART_CHECK_ERROR(add_rma_request_entry(target_unit, dest_seg_id, queue));
    }
    else
    {
        DART_CHECK_GASPI_ERROR(check_queue_size(queue));
    }

    DART_CHECK_GASPI_ERROR(gaspi_write(src_gaspi_seg,
                                       src_offset,
                                       target_unit,
                                       dest_gaspi_seg,
                                       dest_offset,
                                       nbytes,
                                       queue,
                                       GASPI_BLOCK));
    return DART_OK;
}

dart_ret_t dart_get_gptr_blocking(dart_gptr_t dest, dart_gptr_t src, size_t nbytes)
{
    gaspi_queue_id_t   queue;
    /*
     * local site
     */
    gaspi_segment_id_t dest_gaspi_seg = dart_mempool_seg_localalloc;
    gaspi_offset_t     dest_offset    = dest.addr_or_offs.offset;
    uint16_t           dest_index     = dest.flags;
    int16_t            dest_seg_id    = dest.segid;
    dart_unit_t        my_unit        = dest.unitid;
    /*
     * remote site
     */
    gaspi_segment_id_t src_gaspi_seg  = dart_mempool_seg_localalloc;
    gaspi_offset_t     src_offset     = src.addr_or_offs.offset;
    int16_t            src_seg_id     = src.segid;
    uint16_t           src_index      = src.flags;
    dart_unit_t        target_unit    = src.unitid;
    /*
     * local site
     */
    if(dest_seg_id)
    {
        dart_unit_t rel_my_unit;
        DART_CHECK_ERROR(unit_g2l(dest_index, my_unit, &rel_my_unit));
        if(dart_adapt_transtable_get_gaspi_seg_id(dest_seg_id, rel_my_unit, &dest_gaspi_seg) == -1)
        {
            fprintf(stderr, "Can't find given destination segment id in dart_get_blocking\n");
            return DART_ERR_NOTFOUND;
        }
    }
    /*
     * remote site
     */
    if(src_seg_id)
    {
        dart_unit_t rel_target_unit;
        DART_CHECK_ERROR(unit_g2l(src_index, target_unit, &rel_target_unit));
        if(dart_adapt_transtable_get_gaspi_seg_id(src_seg_id, rel_target_unit, &src_gaspi_seg) == -1)
        {
            fprintf(stderr, "Can't find given source segment id in dart_get_blocking\n");
            return DART_ERR_NOTFOUND;
        }
    }
    /*
     * target is own unit
     */
    if(my_unit == target_unit)
    {
        gaspi_pointer_t src_seg_ptr = NULL;
        gaspi_pointer_t dest_seg_ptr = NULL;

        DART_CHECK_GASPI_ERROR(gaspi_segment_ptr(src_gaspi_seg, &src_seg_ptr));
        DART_CHECK_GASPI_ERROR(gaspi_segment_ptr(dest_gaspi_seg, &dest_seg_ptr));

        src_seg_ptr  = (void *)((char *) src_seg_ptr  + src_offset);
        dest_seg_ptr = (void *)((char *) dest_seg_ptr + dest_offset);

        memcpy(dest_seg_ptr, src_seg_ptr, nbytes);

        return DART_OK;
    }

    DART_CHECK_ERROR(dart_get_minimal_queue(&queue));

    DART_CHECK_GASPI_ERROR(gaspi_read(dest_gaspi_seg,
                                      dest_offset,
                                      target_unit,
                                      src_gaspi_seg,
                                      src_offset,
                                      nbytes,
                                      queue,
                                      GASPI_BLOCK));

    DART_CHECK_GASPI_ERROR(gaspi_wait(queue, GASPI_BLOCK));

    return DART_OK;
}

dart_ret_t dart_create_handle(dart_handle_t * handle)
{
    *handle = (dart_handle_t) malloc (sizeof (struct dart_handle_struct));
    assert(*handle);
    return DART_OK;
}

dart_ret_t dart_delete_handle(dart_handle_t * handle)
{
    free(*handle);
    return DART_OK;
}

dart_ret_t dart_get_gptr_handle(dart_gptr_t dest, dart_gptr_t src, size_t nbytes, dart_handle_t handle)
{
    gaspi_queue_id_t   queue;
    /*
     * local site
     */
    gaspi_segment_id_t dest_gaspi_seg = dart_mempool_seg_localalloc;
    gaspi_offset_t     dest_offset    = dest.addr_or_offs.offset;
    uint16_t           dest_index     = dest.flags;
    int16_t            dest_seg_id    = dest.segid;
    dart_unit_t        my_unit        = dest.unitid;
    /*
     * remote site
     */
    gaspi_segment_id_t src_gaspi_seg  = dart_mempool_seg_localalloc;
    gaspi_offset_t     src_offset     = src.addr_or_offs.offset;
    int16_t            src_seg_id     = src.segid;
    uint16_t           src_index      = src.flags;
    dart_unit_t        target_unit    = src.unitid;

    if(handle == NULL)
    {
        fprintf(stderr, "dart_get_gptr_handle : Handle structure not initialized\n");
        return DART_ERR_OTHER;
    }
    /*
     * local site
     */
    if(dest_seg_id)
    {

        dart_unit_t rel_my_unit;
        DART_CHECK_ERROR(unit_g2l(dest_index, my_unit, &rel_my_unit));
        if(dart_adapt_transtable_get_gaspi_seg_id(dest_seg_id, rel_my_unit, &dest_gaspi_seg) == -1)
        {
            fprintf(stderr, "Can't find given destination segment id in dart_get_gptr_handle\n");
            return DART_ERR_NOTFOUND;
        }
    }
    /*
     * remote site
     */
    if(src_seg_id)
    {
        dart_unit_t rel_target_unit;
        DART_CHECK_ERROR(unit_g2l(src_index, target_unit, &rel_target_unit));
        if(dart_adapt_transtable_get_gaspi_seg_id(src_seg_id, rel_target_unit, &src_gaspi_seg) == -1)
        {
            fprintf(stderr, "Can't find given source segment id in dart_get_gptr_handle\n");
            return DART_ERR_NOTFOUND;
        }
    }

    DART_CHECK_ERROR(dart_get_minimal_queue(&queue));

    handle->queue       = queue;
    handle->remote_seg  = src_gaspi_seg;
    handle->target_unit = target_unit;

    DART_CHECK_GASPI_ERROR(gaspi_read(dest_gaspi_seg,
                                      dest_offset,
                                      target_unit,
                                      src_gaspi_seg,
                                      src_offset,
                                      nbytes,
                                      queue,
                                      GASPI_BLOCK));

    return DART_OK;
}

dart_ret_t dart_put_gptr_handle(dart_gptr_t dest, dart_gptr_t src, size_t nbytes, dart_handle_t handle)
{
    gaspi_queue_id_t   queue;
    /*
     * local site
     */
    gaspi_segment_id_t src_gaspi_seg  = dart_mempool_seg_localalloc;
    gaspi_offset_t     src_offset     = src.addr_or_offs.offset;
    int16_t            src_seg_id     = src.segid;
    uint16_t           src_index      = src.flags;
    dart_unit_t        my_unit        = src.unitid;
    /*
     * remote site
     */
    gaspi_segment_id_t dest_gaspi_seg = dart_mempool_seg_localalloc;
    gaspi_offset_t     dest_offset    = dest.addr_or_offs.offset;
    uint16_t           dest_index     = dest.flags;
    int16_t            dest_seg_id    = dest.segid;
    dart_unit_t        target_unit    = dest.unitid;

    if(handle == NULL)
    {
        fprintf(stderr, "dart_put_gptr_handle : Handle structure not initialized\n");
        return DART_ERR_OTHER;
    }
    /*
     * local site
     */
    if(src_seg_id)
    {
        dart_unit_t rel_my_unit;
        DART_CHECK_ERROR(unit_g2l(src_index, my_unit, &rel_my_unit));
        if(dart_adapt_transtable_get_gaspi_seg_id(src_seg_id, rel_my_unit, &src_gaspi_seg) == -1)
        {
            fprintf(stderr, "Can't find given source segment id in dart_get_gptr_handle\n");
            return DART_ERR_NOTFOUND;
        }
    }
    /*
     * remote site
     */
    if(dest_seg_id)
    {
        dart_unit_t rel_target_unit;
        DART_CHECK_ERROR(unit_g2l(dest_index, target_unit, &rel_target_unit));
        if(dart_adapt_transtable_get_gaspi_seg_id(dest_seg_id, rel_target_unit, &dest_gaspi_seg) == -1)
        {
            fprintf(stderr, "Can't find given destination segment id in dart_put_gptr\n");
            return DART_ERR_NOTFOUND;
        }
    }

    DART_CHECK_ERROR(dart_get_minimal_queue(&queue));

    DART_CHECK_GASPI_ERROR(gaspi_write(src_gaspi_seg,
                                       src_offset,
                                       target_unit,
                                       dest_gaspi_seg,
                                       dest_offset,
                                       nbytes,
                                       queue,
                                       GASPI_BLOCK));

    handle->index       = src_index;
    handle->queue       = queue;
    handle->remote_seg  = dest_gaspi_seg;
    handle->target_unit = target_unit;

    return DART_OK;
}

dart_ret_t dart_wait_local (dart_handle_t handle)
{
    if (handle == NULL)
    {
        return DART_ERR_INVAL;
    }

    DART_CHECK_GASPI_ERROR(gaspi_wait(handle->queue, GASPI_BLOCK));
    return DART_OK;
}

dart_ret_t dart_waitall_local(dart_handle_t * handle, size_t n)
{
    if (handle == NULL)
    {
        return DART_ERR_INVAL;
    }
    for(size_t i = 0 ; i < n ; ++i)
    {
        DART_CHECK_ERROR(dart_wait_local(handle[i]));
    }

    return DART_OK;
}

dart_ret_t dart_test_local (dart_handle_t handle, int32_t* is_finished)
{
    if(handle == NULL)
    {
        return DART_ERR_INVAL;
    }

    gaspi_return_t retval = gaspi_wait(handle->queue, GASPI_TEST);

    if(retval == GASPI_SUCCESS)
    {
        *is_finished = 1;
        return DART_OK;
    }
    else if(retval == GASPI_ERROR)
    {
        fprintf(stderr, "Error: gaspi_wait failed in dart_test_local\n");
        return DART_ERR_OTHER;
    }

    *is_finished = 0;
    return DART_OK;
}

dart_ret_t dart_testall_local (dart_handle_t *handle, size_t n, int32_t* is_finished)
{
    for(size_t i = 0 ; i < n ; ++i)
    {
        DART_CHECK_ERROR(dart_test_local(handle[i], is_finished));
        if(! *is_finished )
        {
            return DART_OK;
        }
    }
    return DART_OK;
}

dart_ret_t _dart_flush_local(dart_gptr_t gptr, gaspi_timeout_t timeout, gaspi_return_t * ret)
{
    int16_t          seg_id      = gptr.segid;
    dart_unit_t      remote_rank = gptr.unitid;
    int32_t          found       = 0;
    gaspi_queue_id_t qid;


    DART_CHECK_ERROR(find_rma_request(remote_rank, seg_id, &qid, &found));
    if(found)
    {
        *ret = gaspi_wait(qid, timeout);
    }
    else
    {
        gaspi_printf("No queue found\n");
    }

    return DART_OK;
}

dart_ret_t dart_flush_local(dart_gptr_t gptr)
{
    gaspi_return_t ret = GASPI_SUCCESS;
    DART_CHECK_ERROR(_dart_flush_local(gptr, GASPI_BLOCK, &ret));
    if(ret != GASPI_SUCCESS)
    {
        gaspi_printf("ERROR in %s at line %d in file %s\n","_dart_flush_local", __LINE__, __FILE__);
        return DART_ERR_OTHER;
    }
    return DART_OK;
}

dart_ret_t dart_flush_local_all(dart_gptr_t gptr)
{
    int16_t seg_id      = gptr.segid;

    request_iterator_t iter = new_request_iter(seg_id);

    while(request_iter_is_vaild(iter))
    {
        gaspi_queue_id_t qid;
        DART_CHECK_ERROR(request_iter_get_queue(iter, &qid));

        DART_CHECK_GASPI_ERROR(gaspi_wait(qid, GASPI_BLOCK));

        DART_CHECK_ERROR(request_iter_next(iter));
    }

    DART_CHECK_ERROR(destroy_request_iter(iter));
    return DART_OK;
}
