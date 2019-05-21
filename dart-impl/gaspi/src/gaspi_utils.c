#include <dash/dart/gaspi/gaspi_utils.h>

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include <dash/dart/gaspi/dart_initialization.h>
#include <dash/dart/gaspi/dart_seg_stack.h>
#include <dash/dart/gaspi/dart_team_private.h>


static gaspi_segment_id_t gaspi_utils_seg_counter;

size_t dart_max_segs(){
   gaspi_number_t max_segs = 0;
   DART_CHECK_GASPI_ERROR(
      gaspi_segment_max(&max_segs);
   );
   return max_segs;
}
/* Function to resolve dart_data_type and return
 * size of type
 * !DART_TYPE_UNDEFINED and DART_TYPE_COUNT covered by
 * case default!
*/
size_t dart_gaspi_datatype_sizeof(dart_datatype_t in){
   size_t size;
   switch(in){
      case DART_TYPE_BYTE:  size = sizeof(char); break;
      case DART_TYPE_SHORT: size = sizeof(short); break;
      case DART_TYPE_INT: size = sizeof(int); break;
      case DART_TYPE_UINT: size = sizeof(unsigned int); break;
      case DART_TYPE_LONG: size = sizeof(long); break;
      case DART_TYPE_ULONG: size = sizeof(unsigned long); break;
      case DART_TYPE_LONGLONG: size = sizeof(long long); break;
      case DART_TYPE_FLOAT: size = sizeof(float); break;
      case DART_TYPE_DOUBLE: size = sizeof(double); break;
      default: size = 1; break;
   }
   return size;
}

gaspi_return_t delete_all_segments()
{
    while (gaspi_utils_seg_counter-- > 0)
    {
        DART_CHECK_GASPI_ERROR(gaspi_segment_delete(gaspi_utils_seg_counter));
    }
    return GASPI_SUCCESS;
}

gaspi_return_t create_segment(const gaspi_size_t size,
                              gaspi_segment_id_t *seg_id)
{
    gaspi_return_t retval = GASPI_SUCCESS;
    gaspi_number_t seg_max = 0;
    DART_CHECK_GASPI_ERROR(gaspi_segment_max(&seg_max));

    if (seg_max < gaspi_utils_seg_counter)
    {
        printf("Error: Can't create segment, reached max. of segments\n");
        return GASPI_ERROR;
    }

    DART_CHECK_ERROR_RET(retval, gaspi_segment_create(gaspi_utils_seg_counter, size,
                         GASPI_GROUP_ALL, GASPI_BLOCK,
                         GASPI_MEM_UNINITIALIZED));
    *seg_id = gaspi_utils_seg_counter;

    ++gaspi_utils_seg_counter;

    return retval;
}

gaspi_return_t check_queue_size(gaspi_queue_id_t queue)
{
    gaspi_number_t queue_size = 0;
    DART_CHECK_GASPI_ERROR(gaspi_queue_size(queue, &queue_size));

    gaspi_number_t queue_size_max = 0;
    DART_CHECK_GASPI_ERROR(gaspi_queue_size_max(&queue_size_max));

    if (queue_size >= queue_size_max)
    {
        DART_CHECK_GASPI_ERROR(gaspi_wait(queue, GASPI_BLOCK));
    }
    return GASPI_SUCCESS;
}

gaspi_return_t wait_for_queue_entries(gaspi_queue_id_t *queue,
                                      gaspi_number_t wanted_entries)
{
    gaspi_number_t queue_size_max;
    gaspi_number_t queue_size;
    gaspi_number_t queue_num;

    DART_CHECK_GASPI_ERROR(gaspi_queue_size_max(&queue_size_max));
    DART_CHECK_GASPI_ERROR(gaspi_queue_size(*queue, &queue_size));
    DART_CHECK_GASPI_ERROR(gaspi_queue_num(&queue_num));

    if (!(queue_size + wanted_entries <= queue_size_max))
    {
        *queue = (*queue + 1) % queue_num;
        DART_CHECK_GASPI_ERROR(gaspi_wait(*queue, GASPI_BLOCK));
    }
    return GASPI_SUCCESS;
}

gaspi_return_t blocking_waitsome(gaspi_notification_id_t id_begin,
                                 gaspi_notification_id_t id_count,
                                 gaspi_notification_id_t *id_available,
                                 gaspi_notification_t *notify_val,
                                 gaspi_segment_id_t seg)
{
    DART_CHECK_GASPI_ERROR(gaspi_notify_waitsome(seg, id_begin, id_count,
                           id_available, GASPI_BLOCK));
    DART_CHECK_GASPI_ERROR(gaspi_notify_reset(seg, *id_available, notify_val));

    return GASPI_SUCCESS;
}

gaspi_return_t flush_queues(gaspi_queue_id_t queue_begin, gaspi_queue_id_t queue_count)
{
    gaspi_number_t queue_size = 0;

    for (gaspi_queue_id_t queue = queue_begin;
            queue < (queue_count + queue_begin); ++queue)
    {
        DART_CHECK_GASPI_ERROR(gaspi_queue_size(queue, &queue_size));

        if (queue_size > 0)
        {
            DART_CHECK_GASPI_ERROR(gaspi_wait(queue, GASPI_BLOCK));
        }
    }
    return GASPI_SUCCESS;
}

/************************************************************************************
 *
 * collective operations helper functions
 *
 ************************************************************************************/
static unsigned int npot(unsigned int v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;

    return v + 1;
}

unsigned int gaspi_utils_get_bino_num(int rank, int root, int rank_count)
{
    rank -= root;
    if (rank < 0)
    {
        rank += rank_count;
    }
    return rank;
}

unsigned int gaspi_utils_get_rank(int relative_rank, int root, int rank_count)
{
    relative_rank += root;
    if (relative_rank >= rank_count)
    {
        relative_rank -= rank_count;
    }
    return relative_rank;
}

int gaspi_utils_compute_comms(int *parent, int **children, int me, int root, gaspi_rank_t size)
{
    unsigned int size_pot = npot(size);
    unsigned int d;
    unsigned int number_of_children = 0;

    /* Be your own parent, ie. the root, by default */
    *parent = me;

    me = gaspi_utils_get_bino_num(me, root, size);

    /* Calculate the number of children for me */
    for (d = 1; d; d <<= 1)
    {
        /* Actually break condition */
        if (d > size_pot)
        {
            break;
        }

        /* Check if we are actually a child of someone */
        if (me & d)
        {
            /* Yes, set the parent to our real one, and stop */
            *parent = me ^ d;
            *parent = gaspi_utils_get_rank(*parent, root, size);
            break;
        }

        /* Only count real children, of the virtual hypercube */
        if ((me ^ d) < size)
        {
            number_of_children++;
        }
    }

    /* Put the ranks of all children into a list and return */
    *children = malloc(sizeof(**children) * number_of_children);
    unsigned int child = number_of_children;

    d >>= 1;
    while (d)
    {
        if ((me ^ d) < size)
        {
            (*children)[--child] = me ^ d;
            (*children)[child] = gaspi_utils_get_rank((*children)[child], root, size);
        }
        d >>= 1;
    }

    return number_of_children;
}
