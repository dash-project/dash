#include <dash/dart/gaspi/dart_globmem_priv.h>
#include <dash/dart/gaspi/dart_mem.h>
#include <dash/dart/gaspi/dart_team_private.h>
#include <dash/dart/gaspi/dart_gaspi.h>
#include <dash/dart/gaspi/dart_translation.h>
#include <dash/dart/gaspi/dart_communication_priv.h>

#include <dash/dart/base/locality.h>

#include <limits.h>

gaspi_rank_t dart_gaspi_rank_num;
gaspi_rank_t dart_gaspi_rank;
/**************** global auxiliary memory ****************/
/**
 * for internal communication
 */
gaspi_pointer_t dart_gaspi_buffer_ptr;
const gaspi_segment_id_t dart_gaspi_buffer_id = 0;
/*********************************************************/
const gaspi_segment_id_t dart_fallback_seg = 2;
bool dart_fallback_seg_is_allocated;
/***************** non-collective memory *****************/
#define DART_BUDDY_ORDER 24
/* Gaspi segment number for non-collective memory */
const gaspi_segment_id_t dart_mempool_seg_localalloc = 1;
/* Point to the base address of memory region for local allocation. */
char * dart_mempool_localalloc;
/* Help to do memory management work for local allocation/free */
struct dart_buddy* dart_localpool;
/******************* collective memory *******************/
const gaspi_segment_id_t dart_coll_seg_id_begin = 3;
const size_t dart_coll_seg_count = 28;
/*********************************************************/
static int _init_by_dart = 0;
static int _count_init = 0;
static int _dart_initialized = 0;


dart_ret_t dart_init(int *argc, char ***argv)
{
    gaspi_return_t ret = gaspi_proc_init(GASPI_BLOCK);

    if(ret)
        ++_count_init;
    DART_CHECK_ERROR(gaspi_proc_rank(&dart_gaspi_rank));
    DART_CHECK_ERROR(gaspi_proc_num(&dart_gaspi_rank_num));
    
    /* Initialize the teamlist. */
    dart_adapt_teamlist_init();

    /* Create a global translation table for all the collective global memory */
    dart_adapt_transtable_create();

    dart_memid = 1;
    dart_next_availteamid = DART_TEAM_ALL;

    uint16_t index;
    int result = dart_adapt_teamlist_alloc(DART_TEAM_ALL, &index);
    if (result == -1)
    {
        return DART_ERR_OTHER;
    }

    dart_teams[index].id = GASPI_GROUP_ALL;
    DART_CHECK_ERROR(dart_group_create(&(dart_teams[index].group)));

    for(gaspi_rank_t r = 0; r < dart_gaspi_rank_num ; ++r)
    {
        dart_global_unit_t id;
        id.id = r;
        DART_CHECK_ERROR(dart_group_addmember(dart_teams[index].group, id));
    }

    dart_next_availteamid++;
    /*
     * non-collective memory initialization
     */
    dart_localpool = dart_buddy_new (DART_BUDDY_ORDER);

    DART_CHECK_ERROR(gaspi_segment_create(dart_mempool_seg_localalloc,
                                          DART_MAX_LENGTH,
                                          GASPI_GROUP_ALL,
                                          GASPI_BLOCK,
                                          GASPI_MEM_INITIALIZED));
    gaspi_pointer_t seg_ptr;
    DART_CHECK_ERROR(gaspi_segment_ptr(dart_mempool_seg_localalloc, &seg_ptr));
    dart_mempool_localalloc = (char *) seg_ptr;

    DART_CHECK_ERROR(inital_rma_request_entry(0));
    /*
     * global auxiliary memory segement per process
     * for internal communication and dart collective operations
     */
    DART_CHECK_ERROR(gaspi_segment_create(dart_gaspi_buffer_id,
                                          DART_GASPI_BUFFER_SIZE,
                                          GASPI_GROUP_ALL,
                                          GASPI_BLOCK,
                                          GASPI_MEM_INITIALIZED));

    DART_CHECK_ERROR(gaspi_segment_ptr(dart_gaspi_buffer_id, &dart_gaspi_buffer_ptr));
    /*
     * Create the segment id stack
     */
    DART_CHECK_ERROR(seg_stack_init(&dart_free_coll_seg_ids, dart_coll_seg_count));
    /*
     * Set free segment ids in the stack
     */
    DART_CHECK_ERROR(seg_stack_fill(&dart_free_coll_seg_ids, dart_coll_seg_id_begin, dart_coll_seg_count));
    /*
     * dart fallback segment is not allocated at default
     */
    dart_fallback_seg_is_allocated = false;
    dart__base__locality__init();

    return DART_OK;
}

dart_ret_t dart_exit()
{
    DART_CHECK_ERROR(gaspi_barrier(GASPI_GROUP_ALL, GASPI_BLOCK));

    DART_CHECK_ERROR(gaspi_segment_delete(dart_gaspi_buffer_id));

    DART_CHECK_ERROR(gaspi_segment_delete(dart_mempool_seg_localalloc));

    uint16_t index;
    int result = dart_adapt_teamlist_convert(DART_TEAM_ALL, &index);
    if (result == -1)
    {
        return DART_ERR_INVAL;
    }

    DART_CHECK_ERROR(dart_group_destroy(&(dart_teams[index].group)));

    dart_buddy_delete(dart_localpool);

    DART_CHECK_ERROR(dart_adapt_transtable_destroy());

    DART_CHECK_ERROR(dart_adapt_teamlist_destroy());

    DART_CHECK_ERROR(seg_stack_finish(&dart_free_coll_seg_ids));

    if(_count_init == 0)
        DART_CHECK_ERROR(gaspi_proc_term(GASPI_BLOCK));
    --_count_init;
    
    return DART_OK;
}

void dart_abort(int errorcode)
{
  //DART_LOG_INFO("dart_abort: aborting DART run with error code %i", errorcode);
  gaspi_rank_t myRank;
  gaspi_proc_rank(&myRank);
  gaspi_proc_kill(myRank, GASPI_BLOCK);
  /* just in case gaspi_proc_kill does not abort process*/
  abort();
}

dart_ret_t dart_type_create_custom(
    size_t            num_bytes,
    dart_datatype_t * newtype)
{
    if (newtype == NULL) {
        DART_LOG_ERROR("newtype pointer may not be NULL!");
        return DART_ERR_INVAL;
    }

    *newtype = DART_TYPE_UNDEFINED;
    // Frage: in mpi wird hier auf INT_MAX geprüft, hat gaspi die selben limits?

    //dart_datatype_struct_t *new_struct; << needs custom gaspi datatype / struct



    return DART_OK;
}

// In gaspi under dart_mpi_op.c
dart_ret_t dart_op_create(
  dart_operator_t    op,
  void             * userdata,
  bool               commute,
  dart_datatype_t    dtype,
  bool               dtype_is_tmp,
  dart_operation_t * new_op)
  {
    // needs gaspi possibility to create custom operation
    return DART_OK;
  }

dart_ret_t dart_op_destroy(dart_operation_t *op)
{
    // needs gaspi possibility to create custom operation
    return DART_OK;
}