#include <search.h>
#include <string.h>
#include <assert.h>

#include <dash/dart/gaspi/gaspi_utils.h>
#include <dash/dart/gaspi/dart_communication_priv.h>

tree_root * rma_request_table[DART_MAX_SEGS];

typedef struct{
    dart_unit_t key;
    gaspi_queue_id_t queue;
}request_table_entry_t;

static void * request_table_key(struct stree_node * node)
{
    request_table_entry_t * tmp = (request_table_entry_t *) node->node;
    return (void *) &(tmp->key);
}

static int64_t compare_rma_requests(void* keyA, void* keyB)
{
    dart_unit_t u1 = ((request_table_entry_t *) keyA)->key;
    dart_unit_t u2 = ((request_table_entry_t *) keyB)->key;

    return u1 - u2;
}

dart_ret_t inital_rma_request_table()
{
    for(int i = 0; i < DART_MAX_SEGS; ++i)
    {
        rma_request_table[i] = NULL;
    }
    return DART_OK;
}

dart_ret_t inital_rma_request_entry(int16_t seg_id)
{
    rma_request_table[seg_id] = NULL;
    return DART_OK;
}

dart_ret_t delete_rma_requests(int16_t seg_id)
{
    if(rma_request_table[seg_id] != NULL)
    {
        destroy_rbtree(rma_request_table[seg_id]);
        rma_request_table[seg_id] = NULL;
    }
    return DART_OK;
}

dart_ret_t destroy_rma_request_table()
{
    for(int i = 0; i < DART_MAX_SEGS; ++i)
    {
        if(rma_request_table[i] != NULL)
        {
            destroy_rbtree(rma_request_table[i]);
        }
    }
    return DART_OK;
}
/**
 * Search for an existing entry
 * exists ->  returns the queue-id and set found to 1
 * Otherwise -> set found to 0
 */
dart_ret_t find_rma_request(dart_unit_t target_unit, int16_t seg_id, gaspi_queue_id_t * qid, int32_t * found)
{
    if(rma_request_table[seg_id] != NULL)
    {
        request_table_entry_t * entry = (request_table_entry_t *) search_rbtree(*(rma_request_table[seg_id]), &target_unit);
        if(entry != NULL)
        {
            *qid = entry->queue;
            *found = 1;

            return DART_OK;
        }
    }
    *found = 0;
    return DART_OK;
}
/**
 * Before add an entry, check if the entry exists
 * because exisiting entries will be replaced
 */
dart_ret_t add_rma_request_entry(dart_unit_t target_unit, int16_t seg_id, gaspi_queue_id_t qid)
{
    if(rma_request_table[seg_id] == NULL)
    {
        rma_request_table[seg_id] = new_rbtree(request_table_key, compare_rma_requests);
    }
    request_table_entry_t * tmp_entry = (request_table_entry_t *) malloc(sizeof(request_table_entry_t));
    assert(tmp_entry);

    tmp_entry->key   = target_unit;
    tmp_entry->queue = qid;
    /*
     * TODO if the entry exists, it will be replaced !!
     * This call ships the owner of the pointer
     */
    void * existing_entry = rb_tree_insert(rma_request_table[seg_id], tmp_entry);
    if(existing_entry != NULL)
    {
        free(existing_entry);
    }

    return DART_OK;
}

request_iterator_t new_request_iter(int16_t seg_id)
{
    if(rma_request_table[seg_id] != NULL)
    {
        return new_tree_iterator(rma_request_table[seg_id]);
    }
    return NULL;
}

dart_ret_t destroy_request_iter(request_iterator_t iter)
{
    if(iter != NULL)
    {
        destroy_iterator(iter);
        return DART_OK;
    }

    return DART_ERR_INVAL;
}

uint8_t request_iter_is_vaild(request_iterator_t iter)
{
    if(iter != NULL)
    {
        return tree_iterator_has_next(iter);
    }
    return 0;
}

dart_ret_t request_iter_next(request_iterator_t iter)
{
    if(iter != NULL)
    {
        tree_iterator_next(iter);
        return DART_OK;
    }
    return DART_ERR_INVAL;
}

dart_ret_t request_iter_get_queue(request_iterator_t iter, gaspi_queue_id_t * qid)
{
    if(iter != NULL)
    {
        request_table_entry_t * entry = (request_table_entry_t *) tree_iterator_value(iter);
        *qid = entry->queue;
        return DART_OK;
    }
    return DART_ERR_INVAL;
}

dart_ret_t unit_l2g (uint16_t index, dart_unit_t *abs_id, dart_unit_t rel_id)
{
    if (index == 0)
    {
        *abs_id = rel_id;
    }
    else
    {
        *abs_id = dart_teams[index].group.l2g[rel_id];
    }

    return DART_OK;
}

dart_ret_t unit_g2l (uint16_t index, dart_unit_t abs_id, dart_unit_t *rel_id)
{
    if (index == 0)
    {
        *rel_id = abs_id;
    }
    else
    {
        *rel_id = dart_teams[index].group.g2l[abs_id];
    }

    return DART_OK;
}

int dart_gaspi_cmp_ranks(const void * a, const void * b)
{
    const int c = *((gaspi_rank_t *) a);
    const int d = *((gaspi_rank_t *) b);
    return c - d;
}

dart_ret_t dart_get_minimal_queue(gaspi_queue_id_t * qid)
{
    gaspi_number_t qsize;
    gaspi_number_t queue_num_max;
    gaspi_number_t min_queue_size;
    gaspi_number_t queue_size_max;

    DART_CHECK_ERROR(gaspi_queue_size_max(&queue_size_max));
    DART_CHECK_ERROR(gaspi_queue_num(&queue_num_max));

    min_queue_size = queue_size_max;

    for(gaspi_queue_id_t q = 0 ; q < queue_num_max ; ++q)
    {
        DART_CHECK_ERROR(gaspi_queue_size(q, &qsize));
        if(qsize == 0)
        {
            *qid = q;
            return DART_OK;
        }
        if(min_queue_size > qsize)
        {
            min_queue_size = qsize;
            *qid = q;
        }
    }
    /*
     * If there is no empty queue -> perform wait operation to empty one queue
     */
    if(min_queue_size == queue_size_max)
    {
        DART_CHECK_ERROR(gaspi_wait(*qid, GASPI_BLOCK));
    }

    return DART_OK;
}

gaspi_return_t
gaspi_allgather(const gaspi_segment_id_t send_segid,
                const gaspi_offset_t     send_offset,
                const gaspi_segment_id_t recv_segid,
                const gaspi_offset_t     recv_offset,
                const gaspi_size_t       byte_size,
                const gaspi_group_t      group)
{
    gaspi_rank_t            rank;
    gaspi_return_t          retval    = GASPI_SUCCESS;
    gaspi_queue_id_t        queue     = 0;

    DART_CHECK_ERROR_RET(retval, gaspi_barrier(group, GASPI_BLOCK));

    gaspi_proc_rank(&rank);

    gaspi_number_t group_size;

    DART_CHECK_ERROR_RET(retval, gaspi_group_size(group, &group_size));

    gaspi_rank_t * ranks = (gaspi_rank_t *) malloc(sizeof(gaspi_rank_t) * group_size);
    assert(ranks);
    DART_CHECK_ERROR_RET(retval, gaspi_group_ranks(group, ranks));

    int rel_rank = -1;
    for(unsigned int i = 0; i < group_size; ++i)
    {
        if ( ranks[i] == rank )
        {
            rel_rank = i;
            break;
        }
    }

    if(rel_rank == -1)
    {
        fprintf(stderr, "Error: rank %d is no member of group %d", rank, group);
    }

    for (unsigned int i = 0; i < group_size; i++)
    {
        if ( ranks[i] == rank )
        {
            continue;
        }

        check_queue_size(queue);

        DART_CHECK_ERROR_RET(retval, gaspi_write_notify(send_segid,
                             send_offset,
                             ranks[i],
                             recv_segid,
                             recv_offset + (rel_rank * byte_size),
                             byte_size,
                             (gaspi_notification_id_t) rel_rank,
                             42,
                             queue,
                             GASPI_BLOCK));
        gaspi_wait(queue, GASPI_BLOCK);
    }
    int missing = group_size - 1;
    gaspi_notification_id_t id_available;
    gaspi_notification_t    id_val;
    gaspi_pointer_t recv_ptr;
    gaspi_pointer_t send_ptr;

    DART_CHECK_ERROR_RET(retval, gaspi_segment_ptr(send_segid, &send_ptr));
    DART_CHECK_ERROR_RET(retval, gaspi_segment_ptr(recv_segid, &recv_ptr));

    recv_ptr = (void *) ((char *) recv_ptr + recv_offset + (rel_rank * byte_size));
    memcpy(recv_ptr, send_ptr, byte_size);

    while(missing-- > 0)
    {
        blocking_waitsome(0, group_size, &id_available, &id_val, recv_segid);
        if(id_val != 42)
        {
            fprintf(stderr, "Error: Get wrong notify in allgather on rank %d\n", rank);
        }
    }

    free(ranks);

    DART_CHECK_ERROR_RET(retval, gaspi_barrier(group, GASPI_BLOCK));

    return retval;
}

gaspi_return_t get_rel_unit(gaspi_rank_t * group_ranks, gaspi_group_t g, gaspi_rank_t * rel_unit, gaspi_rank_t abs_unit)
{
    if(g == GASPI_GROUP_ALL)
    {
        *rel_unit = abs_unit;
    }
    else
    {
        gaspi_number_t gsize;
        DART_CHECK_GASPI_ERROR(gaspi_group_size(g, &gsize));
        gaspi_rank_t * item = (gaspi_rank_t *) bsearch(&abs_unit, group_ranks, gsize, sizeof(gaspi_rank_t), dart_gaspi_cmp_ranks);
        if(item != NULL)
        {
            *rel_unit = item - group_ranks;
        }
        else
        {
            return GASPI_ERROR;
        }
    }
    return GASPI_SUCCESS;
}

gaspi_return_t get_abs_unit(gaspi_rank_t * group_ranks, gaspi_group_t g, gaspi_rank_t rel_unit, gaspi_rank_t * abs_unit)
{
    if(g == GASPI_GROUP_ALL)
    {
        *abs_unit = rel_unit;
    }
    else
    {
        *abs_unit = group_ranks[rel_unit];
    }
    return GASPI_SUCCESS;
}
/**
 * group-collective blocking broadcast-operation
 * @param seg_id:   same segement for all participating processes
 * @param offset:   same offset for all participating processes
 * @param bytesize: Size in bytes of the data
 * @param root:     Rank number of the root processes
 * @param group:    specify the gaspi-group
 */
gaspi_return_t gaspi_bcast(const gaspi_segment_id_t seg_id,
                           const gaspi_offset_t     offset,
                           const gaspi_size_t       bytesize,
                           const gaspi_rank_t       root,
                           const gaspi_group_t      group)
{
    int children_count;
    int child;
    gaspi_rank_t rank;
    gaspi_number_t rankcount;
    gaspi_return_t retval = GASPI_SUCCESS;
    gaspi_pointer_t p_segment = NULL;
    gaspi_notification_id_t notify_id = 0;
    gaspi_queue_id_t queue = 0;
    gaspi_notification_id_t first_id;
    gaspi_notification_t old_value;

    int parent;
    int *children = NULL;

    DART_CHECK_GASPI_ERROR(gaspi_proc_rank(&rank));
    DART_CHECK_GASPI_ERROR(gaspi_group_size(group, &rankcount));

    gaspi_rank_t * group_ranks = (gaspi_rank_t *) malloc(sizeof(gaspi_rank_t) * rankcount);
    assert(group_ranks);

    DART_CHECK_GASPI_ERROR(gaspi_group_ranks(group, group_ranks));

    DART_CHECK_GASPI_ERROR(gaspi_segment_ptr(seg_id, &p_segment));
    p_segment = p_segment + offset;


    gaspi_rank_t rel_myrank;
    gaspi_rank_t rel_root;

    DART_CHECK_GASPI_ERROR(get_rel_unit(group_ranks, group, &rel_myrank, rank));
    DART_CHECK_GASPI_ERROR(get_rel_unit(group_ranks, group, &rel_root  , root));

    children_count = gaspi_utils_compute_comms(&parent, &children, rel_myrank, rel_root, rankcount);

    DART_CHECK_GASPI_ERROR(gaspi_barrier(group, GASPI_BLOCK));

    gaspi_rank_t abs_parent;
    DART_CHECK_GASPI_ERROR(get_abs_unit(group_ranks, group, parent, &abs_parent));
    /*
     * parents + children wait for upper parents data
     */
    if (rank != abs_parent)
    {
        blocking_waitsome(notify_id, 1, &first_id, &old_value, seg_id);
    }
    /*
     * write to all childs
     */
    for (child = 0; child < children_count; child++)
    {
        gaspi_rank_t abs_child;
        DART_CHECK_GASPI_ERROR(get_abs_unit(group_ranks, group, children[child], &abs_child));

        check_queue_size(queue);
        DART_CHECK_GASPI_ERROR(gaspi_write_notify(seg_id,
                                                  offset,
                                                  abs_child,
                                                  seg_id,
                                                  offset,
                                                  bytesize,
                                                  notify_id,
                                                  42,
                                                  queue,
                                                  GASPI_BLOCK));
    }

    free(children);
    free(group_ranks);
    DART_CHECK_GASPI_ERROR(gaspi_barrier(group, GASPI_BLOCK));
    return retval;
}

