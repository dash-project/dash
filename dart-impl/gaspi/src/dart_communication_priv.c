#include <search.h>
#include <string.h>
#include <assert.h>

#include <dash/dart/gaspi/gaspi_utils.h>
#include <dash/dart/gaspi/dart_communication_priv.h>
#include <dash/dart/gaspi/dart_translation.h>

tree_root * rma_request_table[DART_MAX_SEGS];


typedef struct{
    dart_unit_t             key;
    gaspi_queue_id_t        queue;
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
dart_ret_t find_rma_request(dart_unit_t target_unit, int16_t seg_id, gaspi_queue_id_t * qid, char * found)
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
        *abs_id = dart_teams[index].group->l2g[rel_id];
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
        *rel_id = dart_teams[index].group->g2l[abs_id];
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

dart_ret_t check_seg_id(dart_gptr_t* gptr, dart_unit_t* global_unit_id, gaspi_segment_id_t* gaspi_seg_id, const char* location)
{
    if(gptr->segid)
    {
        //dart_team_unit_l2g(gptr.teamid, target_unit, &global_target_unit);
        DART_CHECK_ERROR(unit_l2g(gptr->flags, global_unit_id, gptr->unitid));
        if(dart_adapt_transtable_get_gaspi_seg_id(gptr->segid, gptr->unitid, gaspi_seg_id) == -1)
        {
            fprintf(stderr, "Can't find given segment id in %s\n", location);
            return DART_ERR_NOTFOUND;
        }
    }

    return DART_OK;
}

dart_ret_t local_copy_get(dart_gptr_t* gptr, gaspi_segment_id_t gaspi_src_segment_id, void* dest, size_t nbytes)
{
    gaspi_pointer_t src_seg_ptr = NULL;
    DART_CHECK_GASPI_ERROR(gaspi_segment_ptr(gaspi_src_segment_id, &src_seg_ptr));

    src_seg_ptr = (void *) ((char *) src_seg_ptr + gptr->addr_or_offs.offset);

    memcpy(dest, src_seg_ptr, nbytes);

    return DART_OK;
}

dart_ret_t local_copy_put(dart_gptr_t* gptr, gaspi_segment_id_t gaspi_dst_segment_id, const void* src, size_t nbytes)
{
    gaspi_pointer_t dst_seg_ptr = NULL;
    DART_CHECK_GASPI_ERROR(gaspi_segment_ptr(gaspi_dst_segment_id, &dst_seg_ptr));

    dst_seg_ptr = (void *) ((char *)  dst_seg_ptr + gptr->addr_or_offs.offset);

    memcpy(dst_seg_ptr, src, nbytes);

    return DART_OK;
}

void set_multiple_block(converted_type_t* conv_types, size_t num_blocks)
{
    conv_types->num_blocks = num_blocks;
    conv_types->kind = DART_BLOCK_MULTIPLE;
    conv_types->multiple = (multiple_t){(offset_pair_t*) malloc(sizeof(offset_pair_t) * num_blocks),
                                        (offset_pair_t*) malloc(sizeof(offset_pair_t) * num_blocks)};
}

void set_single_block(converted_type_t* conv_types, size_t num_blocks, offset_pair_t offset_pair, size_t nbytes)
{
    conv_types->num_blocks = num_blocks;
    conv_types->kind = DART_BLOCK_SINGLE;
    conv_types->single = (single_t){offset_pair, nbytes};
}

dart_ret_t dart_convert_type(dart_datatype_struct_t* dts_src,
                             dart_datatype_struct_t* dts_dst,
                             size_t nelem,
                             size_t nbytes_elem,
                             converted_type_t* conv_types)
{
    //*conv_types = (converted_type_t){0, true, NULL, NULL, NULL};
    if (datatype_iscontiguous(dts_src) && datatype_iscontiguous(dts_dst))
    {
        set_single_block(conv_types, 1, (offset_pair_t){0,0}, nelem * nbytes_elem);
        //*conv_types = (converted_type_t) {1, DART_BLOCK_SINGLE, (single_t)({(offset_pair_t)({0, 0}), nelem})};
    }
    if(datatype_iscontiguous(dts_src) || datatype_iscontiguous(dts_dst))
    {
        if(datatype_iscontiguous(dts_src))
        {
            if(datatype_isstrided(dts_dst))
            {
                size_t num_blocks = nelem / dts_dst->num_elem;
                size_t num_elem_byte = dts_dst->num_elem * nbytes_elem;

                set_single_block(conv_types, num_blocks, (offset_pair_t){num_elem_byte,dts_dst->strided.stride * nbytes_elem}, num_elem_byte);

                return DART_OK;
            }

            if(datatype_isindexed(dts_dst))
            {
                size_t num_blocks = dts_dst->indexed.num_blocks;
                set_multiple_block(conv_types, num_blocks);
                size_t offset_src = 0;
                for(int i = 0; i < num_blocks; ++i)
                {
                    size_t num_elem_byte = dts_dst->indexed.blocklens[i] * nbytes_elem;
                    conv_types->multiple.nbytes[i] = num_elem_byte;
                    conv_types->multiple.offsets[i] = (offset_pair_t){offset_src, dts_dst->indexed.offsets[i] * nbytes_elem};
                    offset_src += num_elem_byte;
                }

                return DART_OK;
            }

            return DART_ERR_INVAL;

        }
        else
        {
            if(datatype_isstrided(dts_src))
            {
                size_t num_blocks = nelem / dts_src->num_elem;
                size_t num_elem_byte = dts_src->num_elem * nbytes_elem;

                set_single_block(conv_types, num_blocks, (offset_pair_t){dts_src->strided.stride * nbytes_elem, num_elem_byte}, num_elem_byte);

                return DART_OK;
            }

            if(datatype_isindexed(dts_src))
            {
                size_t num_blocks = dts_src->indexed.num_blocks;
                set_multiple_block(conv_types, num_blocks);
                size_t offset_dst = 0;
                for(int i = 0; i < num_blocks; ++i)
                {
                    size_t num_elem_byte = dts_src->indexed.blocklens[i] * nbytes_elem;
                    conv_types->multiple.nbytes[i] = num_elem_byte;
                    conv_types->multiple.offsets[i] = (offset_pair_t){dts_src->indexed.offsets[i] * nbytes_elem, offset_dst};
                    offset_dst += num_elem_byte;
                }

                return DART_OK;
            }

            return DART_ERR_INVAL;
        }
    }

    return DART_ERR_INVAL;
}