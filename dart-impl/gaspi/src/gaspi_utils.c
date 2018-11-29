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

/* Returns GASPI_SUCCESS if successful or GASPI_ERROR if an error occurs or
 * if there are no more free segment ids.
 * The current implementation is not optimized. All data is gathered from all
 * process by root. After this root reduce all data sequential.
 * If reduce is successful than root returns verctor of reduced elements and
 * all other return an nullified vector.
 */
gaspi_return_t gaspi_reduce_user (
   const void* buffer_send,
	void* buffer_receive,
	const gaspi_number_t num,
	const gaspi_size_t element_size,
	gaspi_reduce_operation_t const reduce_operation,
	gaspi_reduce_state_t const reduce_state,
	const gaspi_group_t group,
   gaspi_segment_id_t* segment_ids,
   const gaspi_rank_t root,
	const gaspi_timeout_t timeout_ms)
{
   //TODO Use Lists of segemnt_ids
   gaspi_rank_t myID;
   DART_CHECK_GASPI_ERROR(gaspi_proc_rank(&myID));
   //get number of gaspi group members
   gaspi_number_t group_size;
   DART_CHECK_GASPI_ERROR(gaspi_group_size(group, &group_size));
   //all ranks in group
   gaspi_rank_t*  rank_list = (gaspi_rank_t*) malloc(group_size * sizeof(gaspi_rank_t));
   DART_CHECK_GASPI_ERROR(gaspi_group_ranks(group, rank_list));
   //check if root is in groups, 0 == false , 1 == true
   int root_is_in_group = 0;
   for(int i = 0; i < group_size; ++i){
      if(rank_list[i] == root){
         root_is_in_group = 1;
      }
   }
   if(root_is_in_group != 1){
      printf("I'm process %d and root could not be found in my group!\n", myID);
      return GASPI_ERR_INV_RANK;
   }

   /*
    * It's possible that there are segments allocated when this procedure
    * is running. Therefor it's important to check wich segment_ids are free
    * too use.
    */
   gaspi_number_t num_of_segments = 0, max_num_segments = 0;
   DART_CHECK_GASPI_ERROR(gaspi_segment_num(&num_of_segments));
   DART_CHECK_GASPI_ERROR(gaspi_segment_max(&max_num_segments));

   //if segemt number + 1 (segments used by root) exceeds max num of segment abort
   if((num_of_segments + 1) > max_num_segments){
      printf("No more room to allocate needed segemnts\n");
      return GASPI_ERR_MANY_SEG;
   }

   //find one free Id within range (0, max_num_segments)
   gaspi_segment_id_t* current_segments = (gaspi_segment_id_t*) malloc (num_of_segments * sizeof(gaspi_segment_id_t));
   gaspi_segment_id_t useable_id = 0;
   DART_CHECK_ERROR(seg_stack_pop(&dart_free_coll_seg_ids, &useable_id));

   //create segment to read from for each process using one id found above
   size_t num_bytes = element_size * num;

   //gaspi_segment_id_t const source_id = segment_ids[segment_index];
   gaspi_segment_id_t const source_id = dart_fallback_seg;              // segment_ids[segment_index];
   gaspi_size_t       const source_size = num_bytes;
   DART_CHECK_GASPI_ERROR(
      gaspi_segment_create(
      source_id, source_size, group,
      GASPI_BLOCK, GASPI_MEM_UNINITIALIZED
      )
   );
   gaspi_pointer_t source_p;
   DART_CHECK_GASPI_ERROR(
      gaspi_segment_ptr (source_id, &source_p)
   );
   void* source_pointer = (void*) source_p;
   //copy data from send_buffer to source_segment for reading operations
   memcpy(source_pointer, buffer_send, num_bytes);

   //root only operations
   gaspi_queue_id_t read_queue = 0;
   DART_CHECK_GASPI_ERROR(
      gaspi_queue_create(&read_queue, GASPI_BLOCK)
   );
   if(root == myID){
      //buffer to store all data read from remote ranks
      void* reduce_buffer = (void*) malloc(num_bytes * group_size);
      gaspi_segment_id_t const reduce_buffer_sgmt_id = useable_id;
      DART_CHECK_GASPI_ERROR(
         gaspi_segment_bind(reduce_buffer_sgmt_id, reduce_buffer, num_bytes * group_size, 0)
      );
      //read operations performed by root on all proccess in group (includes root)
      int offset_local = 0, offset_remote = 0;
      for(int i = 0;  i < group_size; ++i){
         DART_CHECK_GASPI_ERROR(
            gaspi_read(
               reduce_buffer_sgmt_id, offset_local,
               rank_list[i], source_id,
               offset_remote, num_bytes,
               read_queue, GASPI_BLOCK
            )
         );
         offset_local += num_bytes;
      }
      DART_CHECK_GASPI_ERROR(
         gaspi_wait(read_queue, GASPI_BLOCK)
      );

      //reduce elemts, result of each step is safed in first num_bytes of reduce_buffer
      for(int i = 1; i < group_size; ++i){
         DART_CHECK_GASPI_ERROR(
            reduce_operation(reduce_buffer, reduce_buffer+i*num_bytes, reduce_buffer,
                            reduce_state, num, element_size, GASPI_BLOCK)
            );
      }

   //write reduce result back to output pointer
   memcpy(buffer_receive, reduce_buffer, num_bytes);
   DART_CHECK_GASPI_ERROR(
      gaspi_segment_delete(reduce_buffer_sgmt_id)
   );
   free(reduce_buffer);
   //all other processes have to null recv
   }else{
      memset(buffer_receive, 0, num_bytes);
   }

   //release memory if all processes are done
   gaspi_barrier(group, GASPI_BLOCK);
   DART_CHECK_GASPI_ERROR(
      gaspi_segment_delete(source_id)
   );
   free(current_segments);
   free(rank_list);
   return GASPI_SUCCESS;
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

/*
 * GASPI userdefinied reduce operations.
 * TODO: Check in further GASPI versions if there is allready an implementation.
 *       If necessary remove.
 */

/*
 *reduce MINMAX operations
 *
 * From cpp refrence:  
 * A pair with the smallest value in ilist as the first element and the greatest as the second. 
 * If several elements are equivalent to the smallest, the leftmost such element is returned. 
 * If several elements are equivalent to the largest, the rightmost such element is returned.
 */

 // reduce DART_OP_MINMAX char
 gaspi_return_t gaspi_op_MINMAX_char(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(char) != element_size){
      printf("Error: element_size does not match size of char!\n");
      return GASPI_ERROR;
    }
    char* op1_tmp = (char*) op1;
    char* op2_tmp = (char*) op2;
    char* res_tmp = (char*) res;

    for(int i = 0; i < num; ++i){
       if(op1_tmp[i] < op2_tmp[i]){
          if(0 == i){
             res_tmp[(i*2)]    = op1_tmp[i];
             res_tmp[(i*2)+1]  = op2_tmp[i];
          } // all further checks are against the earlier MINMAX
          else{
             if(op1_tmp[i] < res_tmp[(i*2)]) res_tmp[(i*2)] = op1_tmp[i];
             if(op2_tmp[i] > res_tmp[(i*2)+1]) res_tmp[(i*2)+1] = op2_tmp[i];
          } 
       }
       else if(op1_tmp[i] > op2_tmp[i]){
          if(0 == i){
             res_tmp[(i*2)]    = op2_tmp[i];
             res_tmp[(i*2)+1]  = op1_tmp[i];
          } // all further checks are against the earlier MINMAX
          else{
             if(op2_tmp[i] < res_tmp[(i*2)]) res_tmp[(i*2)] = op2_tmp[i];
             if(op1_tmp[i] > res_tmp[(i*2)+1]) res_tmp[(i*2)+1] = op1_tmp[i];
          }

       }
       else{ // if op1 == op2
          if(0 == i){
             res_tmp[(i*2)]    = op1_tmp[i];
             res_tmp[(i*2)+1]  = op2_tmp[i]; 
          }
          else{
             if(op1_tmp[i] < res_tmp[i*2]) res_tmp[i*2] = op1_tmp[i];
             if(op1_tmp[i] > res_tmp[(i*2)+1]) res_tmp[(i*2)+1] = op1_tmp[i];
          }

       }
    }
    return GASPI_SUCCESS;
 }

 // reduce DART_OP_MINMAX short
 gaspi_return_t gaspi_op_MINMAX_short(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(short) != element_size){
      printf("Error: element_size does not match size of short!\n");
      return GASPI_ERROR;
    }
    short* op1_tmp = (short*) op1;
    short* op2_tmp = (short*) op2;
    short* res_tmp = (short*) res;

    for(int i = 0; i < num; ++i){
       if(op1_tmp[i] < op2_tmp[i]){
          if(0 == i){
             res_tmp[(i*2)]    = op1_tmp[i];
             res_tmp[(i*2)+1]  = op2_tmp[i];
          } // all further checks are against the earlier MINMAX
          else{
             if(op1_tmp[i] < res_tmp[(i*2)]) res_tmp[(i*2)] = op1_tmp[i];
             if(op2_tmp[i] > res_tmp[(i*2)+1]) res_tmp[(i*2)+1] = op2_tmp[i];
          } 
       }
       else if(op1_tmp[i] > op2_tmp[i]){
          if(0 == i){
             res_tmp[(i*2)]    = op2_tmp[i];
             res_tmp[(i*2)+1]  = op1_tmp[i];
          } // all further checks are against the earlier MINMAX
          else{
             if(op2_tmp[i] < res_tmp[(i*2)]) res_tmp[(i*2)] = op2_tmp[i];
             if(op1_tmp[i] > res_tmp[(i*2)+1]) res_tmp[(i*2)+1] = op1_tmp[i];
          }

       }
       else{ // if op1 == op2
          if(0 == i){
             res_tmp[(i*2)]    = op1_tmp[i];
             res_tmp[(i*2)+1]  = op2_tmp[i]; 
          }
          else{
             if(op1_tmp[i] < res_tmp[i*2]) res_tmp[i*2] = op1_tmp[i];
             if(op1_tmp[i] > res_tmp[(i*2)+1]) res_tmp[(i*2)+1] = op1_tmp[i];
          }

       }
    }
    return GASPI_SUCCESS;
 }

 //reduce DART_OP_MINMAX int
 gaspi_return_t gaspi_op_MINMAX_int(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(int) != element_size){
      printf("Error: element_size does not match size of int!\n");
      return GASPI_ERROR;
    }

    int* op1_tmp = (int*) op1;
    int* op2_tmp = (int*) op2;
    int* res_tmp = (int*) res;

    for(int i = 0; i < num; ++i){
       if(op1_tmp[i] < op2_tmp[i]){
          if(0 == i){
             res_tmp[(i*2)]    = op1_tmp[i];
             res_tmp[(i*2)+1]  = op2_tmp[i];
          } // all further checks are against the earlier MINMAX
          else{
             if(op1_tmp[i] < res_tmp[(i*2)]) res_tmp[(i*2)] = op1_tmp[i];
             if(op2_tmp[i] > res_tmp[(i*2)+1]) res_tmp[(i*2)+1] = op2_tmp[i];
          } 
       }
       else if(op1_tmp[i] > op2_tmp[i]){
          if(0 == i){
             res_tmp[(i*2)]    = op2_tmp[i];
             res_tmp[(i*2)+1]  = op1_tmp[i];
          } // all further checks are against the earlier MINMAX
          else{
             if(op2_tmp[i] < res_tmp[(i*2)]) res_tmp[(i*2)] = op2_tmp[i];
             if(op1_tmp[i] > res_tmp[(i*2)+1]) res_tmp[(i*2)+1] = op1_tmp[i];
          }

       }
       else{ // if op1 == op2
          if(0 == i){
             res_tmp[(i*2)]    = op1_tmp[i];
             res_tmp[(i*2)+1]  = op2_tmp[i]; 
          }
          else{
             if(op1_tmp[i] < res_tmp[i*2]) res_tmp[i*2] = op1_tmp[i];
             if(op1_tmp[i] > res_tmp[(i*2)+1]) res_tmp[(i*2)+1] = op1_tmp[i];
          }

       }
    }
    return GASPI_SUCCESS;
 }

 //reduce DART_OP_MINMAX uInt
 gaspi_return_t gaspi_op_MINMAX_uInt(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(unsigned int) != element_size){
      printf("Error: element_size does not match size of unsigned int!\n");
      return GASPI_ERROR;
    }
    unsigned int* op1_tmp = (unsigned int*) op1;
    unsigned int* op2_tmp = (unsigned int*) op2;
    unsigned int* res_tmp = (unsigned int*) res;

    for(int i = 0; i < num; ++i){
       if(op1_tmp[i] < op2_tmp[i]){
          if(0 == i){
             res_tmp[(i*2)]    = op1_tmp[i];
             res_tmp[(i*2)+1]  = op2_tmp[i];
          } // all further checks are against the earlier MINMAX
          else{
             if(op1_tmp[i] < res_tmp[(i*2)]) res_tmp[(i*2)] = op1_tmp[i];
             if(op2_tmp[i] > res_tmp[(i*2)+1]) res_tmp[(i*2)+1] = op2_tmp[i];
          } 
       }
       else if(op1_tmp[i] > op2_tmp[i]){
          if(0 == i){
             res_tmp[(i*2)]    = op2_tmp[i];
             res_tmp[(i*2)+1]  = op1_tmp[i];
          } // all further checks are against the earlier MINMAX
          else{
             if(op2_tmp[i] < res_tmp[(i*2)]) res_tmp[(i*2)] = op2_tmp[i];
             if(op1_tmp[i] > res_tmp[(i*2)+1]) res_tmp[(i*2)+1] = op1_tmp[i];
          }

       }
       else{ // if op1 == op2
          if(0 == i){
             res_tmp[(i*2)]    = op1_tmp[i];
             res_tmp[(i*2)+1]  = op2_tmp[i]; 
          }
          else{
             if(op1_tmp[i] < res_tmp[i*2]) res_tmp[i*2] = op1_tmp[i];
             if(op1_tmp[i] > res_tmp[(i*2)+1]) res_tmp[(i*2)+1] = op1_tmp[i];
          }

       }
    }
    return GASPI_SUCCESS;
 }

 // reduce DART_OP_MINMAX long
 gaspi_return_t gaspi_op_MINMAX_long(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(long) != element_size){
      printf("Error: element_size does not match size of long!\n");
      return GASPI_ERROR;
    }
    long* op1_tmp = (long*) op1;
    long* op2_tmp = (long*) op2;
    long* res_tmp = (long*) res;

    for(int i = 0; i < num; ++i){
       if(op1_tmp[i] < op2_tmp[i]){
          if(0 == i){
             res_tmp[(i*2)]    = op1_tmp[i];
             res_tmp[(i*2)+1]  = op2_tmp[i];
          } // all further checks are against the earlier MINMAX
          else{
             if(op1_tmp[i] < res_tmp[(i*2)]) res_tmp[(i*2)] = op1_tmp[i];
             if(op2_tmp[i] > res_tmp[(i*2)+1]) res_tmp[(i*2)+1] = op2_tmp[i];
          } 
       }
       else if(op1_tmp[i] > op2_tmp[i]){
          if(0 == i){
             res_tmp[(i*2)]    = op2_tmp[i];
             res_tmp[(i*2)+1]  = op1_tmp[i];
          } // all further checks are against the earlier MINMAX
          else{
             if(op2_tmp[i] < res_tmp[(i*2)]) res_tmp[(i*2)] = op2_tmp[i];
             if(op1_tmp[i] > res_tmp[(i*2)+1]) res_tmp[(i*2)+1] = op1_tmp[i];
          }

       }
       else{ // if op1 == op2
          if(0 == i){
             res_tmp[(i*2)]    = op1_tmp[i];
             res_tmp[(i*2)+1]  = op2_tmp[i]; 
          }
          else{
             if(op1_tmp[i] < res_tmp[i*2]) res_tmp[i*2] = op1_tmp[i];
             if(op1_tmp[i] > res_tmp[(i*2)+1]) res_tmp[(i*2)+1] = op1_tmp[i];
          }

       }
    }
    return GASPI_SUCCESS;
 }
 
 // reduce DART_OP_MINMAX uLong
 gaspi_return_t gaspi_op_MINMAX_uLong(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(unsigned long) != element_size){
      printf("Error: element_size does not match size of unsigned long!\n");
      return GASPI_ERROR;
    }
    unsigned long* op1_tmp = (unsigned long*) op1;
    unsigned long* op2_tmp = (unsigned long*) op2;
    unsigned long* res_tmp = (unsigned long*) res;

    for(int i = 0; i < num; ++i){
       if(op1_tmp[i] < op2_tmp[i]){
          if(0 == i){
             res_tmp[(i*2)]    = op1_tmp[i];
             res_tmp[(i*2)+1]  = op2_tmp[i];
          } // all further checks are against the earlier MINMAX
          else{
             if(op1_tmp[i] < res_tmp[(i*2)]) res_tmp[(i*2)] = op1_tmp[i];
             if(op2_tmp[i] > res_tmp[(i*2)+1]) res_tmp[(i*2)+1] = op2_tmp[i];
          } 
       }
       else if(op1_tmp[i] > op2_tmp[i]){
          if(0 == i){
             res_tmp[(i*2)]    = op2_tmp[i];
             res_tmp[(i*2)+1]  = op1_tmp[i];
          } // all further checks are against the earlier MINMAX
          else{
             if(op2_tmp[i] < res_tmp[(i*2)]) res_tmp[(i*2)] = op2_tmp[i];
             if(op1_tmp[i] > res_tmp[(i*2)+1]) res_tmp[(i*2)+1] = op1_tmp[i];
          }

       }
       else{ // if op1 == op2
          if(0 == i){
             res_tmp[(i*2)]    = op1_tmp[i];
             res_tmp[(i*2)+1]  = op2_tmp[i]; 
          }
          else{
             if(op1_tmp[i] < res_tmp[i*2]) res_tmp[i*2] = op1_tmp[i];
             if(op1_tmp[i] > res_tmp[(i*2)+1]) res_tmp[(i*2)+1] = op1_tmp[i];
          }

       }
    }
    return GASPI_SUCCESS;
 }

 //reduce DART_OP_MINMAX longLong
 gaspi_return_t gaspi_op_MINMAX_longLong(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(long long) != element_size){
      printf("Error: element_size does not match size of long long!\n");
      return GASPI_ERROR;
    }
    long long* op1_tmp = (long long*) op1;
    long long* op2_tmp = (long long*) op2;
    long long* res_tmp = (long long*) res;

    for(int i = 0; i < num; ++i){
       if(op1_tmp[i] < op2_tmp[i]){
          if(0 == i){
             res_tmp[(i*2)]    = op1_tmp[i];
             res_tmp[(i*2)+1]  = op2_tmp[i];
          } // all further checks are against the earlier MINMAX
          else{
             if(op1_tmp[i] < res_tmp[(i*2)]) res_tmp[(i*2)] = op1_tmp[i];
             if(op2_tmp[i] > res_tmp[(i*2)+1]) res_tmp[(i*2)+1] = op2_tmp[i];
          } 
       }
       else if(op1_tmp[i] > op2_tmp[i]){
          if(0 == i){
             res_tmp[(i*2)]    = op2_tmp[i];
             res_tmp[(i*2)+1]  = op1_tmp[i];
          } // all further checks are against the earlier MINMAX
          else{
             if(op2_tmp[i] < res_tmp[(i*2)]) res_tmp[(i*2)] = op2_tmp[i];
             if(op1_tmp[i] > res_tmp[(i*2)+1]) res_tmp[(i*2)+1] = op1_tmp[i];
          }

       }
       else{ // if op1 == op2
          if(0 == i){
             res_tmp[(i*2)]    = op1_tmp[i];
             res_tmp[(i*2)+1]  = op2_tmp[i]; 
          }
          else{
             if(op1_tmp[i] < res_tmp[i*2]) res_tmp[i*2] = op1_tmp[i];
             if(op1_tmp[i] > res_tmp[(i*2)+1]) res_tmp[(i*2)+1] = op1_tmp[i];
          }

       }
    }
    return GASPI_SUCCESS;
 }

 // reduce DART_OP_MINMAX float
 gaspi_return_t gaspi_op_MINMAX_float(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(float) != element_size){
      printf("Error: element_size does not match size of float!\n");
      return GASPI_ERROR;
    }
    float* op1_tmp = (float*) op1;
    float* op2_tmp = (float*) op2;
    float* res_tmp = (float*) res;

    for(int i = 0; i < num; ++i){
       if(op1_tmp[i] < op2_tmp[i]){
          if(0 == i){
             res_tmp[(i*2)]    = op1_tmp[i];
             res_tmp[(i*2)+1]  = op2_tmp[i];
          } // all further checks are against the earlier MINMAX
          else{
             if(op1_tmp[i] < res_tmp[(i*2)]) res_tmp[(i*2)] = op1_tmp[i];
             if(op2_tmp[i] > res_tmp[(i*2)+1]) res_tmp[(i*2)+1] = op2_tmp[i];
          } 
       }
       else if(op1_tmp[i] > op2_tmp[i]){
          if(0 == i){
             res_tmp[(i*2)]    = op2_tmp[i];
             res_tmp[(i*2)+1]  = op1_tmp[i];
          } // all further checks are against the earlier MINMAX
          else{
             if(op2_tmp[i] < res_tmp[(i*2)]) res_tmp[(i*2)] = op2_tmp[i];
             if(op1_tmp[i] > res_tmp[(i*2)+1]) res_tmp[(i*2)+1] = op1_tmp[i];
          }

       }
       else{ // if op1 == op2
          if(0 == i){
             res_tmp[(i*2)]    = op1_tmp[i];
             res_tmp[(i*2)+1]  = op2_tmp[i]; 
          }
          else{
             if(op1_tmp[i] < res_tmp[i*2]) res_tmp[i*2] = op1_tmp[i];
             if(op1_tmp[i] > res_tmp[(i*2)+1]) res_tmp[(i*2)+1] = op1_tmp[i];
          }

       }
    }
    return GASPI_SUCCESS;
 }

 // reduce DART_OP_MINMAX double
 gaspi_return_t gaspi_op_MINMAX_double(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(double) != element_size){
      printf("Error: element_size does not match size of double!\n");
      return GASPI_ERROR;
    }
    double* op1_tmp = (double*) op1;
    double* op2_tmp = (double*) op2;
    double* res_tmp = (double*) res;

    for(int i = 0; i < num; ++i){
       if(op1_tmp[i] < op2_tmp[i]){
          if(0 == i){
             res_tmp[(i*2)]    = op1_tmp[i];
             res_tmp[(i*2)+1]  = op2_tmp[i];
          } // all further checks are against the earlier MINMAX
          else{
             if(op1_tmp[i] < res_tmp[(i*2)]) res_tmp[(i*2)] = op1_tmp[i];
             if(op2_tmp[i] > res_tmp[(i*2)+1]) res_tmp[(i*2)+1] = op2_tmp[i];
          } 
       }
       else if(op1_tmp[i] > op2_tmp[i]){
          if(0 == i){
             res_tmp[(i*2)]    = op2_tmp[i];
             res_tmp[(i*2)+1]  = op1_tmp[i];
          } // all further checks are against the earlier MINMAX
          else{
             if(op2_tmp[i] < res_tmp[(i*2)]) res_tmp[(i*2)] = op2_tmp[i];
             if(op1_tmp[i] > res_tmp[(i*2)+1]) res_tmp[(i*2)+1] = op1_tmp[i];
          }

       }
       else{ // if op1 == op2
          if(0 == i){
             res_tmp[(i*2)]    = op1_tmp[i];
             res_tmp[(i*2)+1]  = op2_tmp[i]; 
          }
          else{
             if(op1_tmp[i] < res_tmp[i*2]) res_tmp[i*2] = op1_tmp[i];
             if(op1_tmp[i] > res_tmp[(i*2)+1]) res_tmp[(i*2)+1] = op1_tmp[i];
          }

       }
    }
    return GASPI_SUCCESS;
 }

 
/*
 * reduce max operations
 */

 //reduce Op MAX char
 gaspi_return_t gaspi_op_MAX_char(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(char) != element_size){
      printf("Error: element_size does not match size of char!\n");
      return GASPI_ERROR;
    }
    for(int i = 0; i < num; ++i){
      if(((char*)op1)[i] > ((char*)op2)[i]){
          ((char*)res)[i] = ((char*)op1)[i];
      }else{
          ((char*)res)[i] = ((char*)op2)[i];
      }
    }
    return GASPI_SUCCESS;
 }
 //reduce Op MAX short
 gaspi_return_t gaspi_op_MAX_short(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(short) != element_size){
      printf("Error: element_size does not match size of short!\n");
      return GASPI_ERROR;
    }
    for(int i = 0; i < num; ++i){
      if(((short*)op1)[i] > ((short*)op2)[i]){
          ((short*)res)[i] = ((short*)op1)[i];
      }else{
          ((short*)res)[i] = ((short*)op2)[i];
      }
    }
    return GASPI_SUCCESS;
 }

 //reduce Op MAX int
 gaspi_return_t gaspi_op_MAX_int(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(int) != element_size){
      printf("Error: element_size does not match size of int!\n");
      return GASPI_ERROR;
    }
    for(int i = 0; i < num; ++i){
      if(((int*)op1)[i] > ((int*)op2)[i]){
          ((int*)res)[i] = ((int*)op1)[i];
      }else{
          ((int*)res)[i] = ((int*)op2)[i];
      }
    }
    return GASPI_SUCCESS;
 }

 //reduce Op MAX unsigned int
 gaspi_return_t gaspi_op_MAX_uInt(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(unsigned int) != element_size){
      printf("Error: element_size does not match size of unsigned int!\n");
      return GASPI_ERROR;
    }
    for(int i = 0; i < num; ++i){
      if(((unsigned int*)op1)[i] > ((unsigned int*)op2)[i]){
          ((unsigned int*)res)[i] = ((unsigned int*)op1)[i];
      }else{
          ((unsigned int*)res)[i] = ((unsigned int*)op2)[i];
      }
    }
    return GASPI_SUCCESS;
 }

 //reduce Op MAX long
 gaspi_return_t gaspi_op_MAX_long(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(long) != element_size){
      printf("Error: element_size does not match size of long!\n");
      return GASPI_ERROR;
    }
    for(int i = 0; i < num; ++i){
      if(((long*)op1)[i] > ((long*)op2)[i]){
          ((long*)res)[i] = ((long*)op1)[i];
      }else{
          ((long*)res)[i] = ((long*)op2)[i];
      }
    }
    return GASPI_SUCCESS;
 }

 //reduce Op MAX unsigned long
 gaspi_return_t gaspi_op_MAX_uLong(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(unsigned long) != element_size){
      printf("Error: element_size does not match size of unsigned long!\n");
      return GASPI_ERROR;
    }
    for(int i = 0; i < num; ++i){
      if(((unsigned long*)op1)[i] > ((unsigned long*)op2)[i]){
          ((unsigned long*)res)[i] = ((unsigned long*)op1)[i];
      }else{
          ((unsigned long*)res)[i] = ((unsigned long*)op2)[i];
      }
    }
    return GASPI_SUCCESS;
 }

 //reduce Op MAX long long
 gaspi_return_t gaspi_op_MAX_longLong(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(long long) != element_size){
      printf("Error: element_size does not match size of long long!\n");
      return GASPI_ERROR;
    }
    for(int i = 0; i < num; ++i){
      if(((long long*)op1)[i] > ((long long*)op2)[i]){
          ((long long*)res)[i] = ((long long*)op1)[i];
      }else{
          ((long long*)res)[i] = ((long long*)op2)[i];
      }
    }
    return GASPI_SUCCESS;
 }

 //reduce Op MAX float
 gaspi_return_t gaspi_op_MAX_float(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(float) != element_size){
      printf("Error: element_size does not match size of float!\n");
      return GASPI_ERROR;
    }
    for(int i = 0; i < num; ++i){
      if(((float*)op1)[i] > ((float*)op2)[i]){
          ((float*)res)[i] = ((float*)op1)[i];
      }else{
          ((float*)res)[i] = ((float*)op2)[i];
      }
    }
    return GASPI_SUCCESS;
 }

 //reduce Op MAX double
 gaspi_return_t gaspi_op_MAX_double(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(double) != element_size){
      printf("Error: element_size does not match size of double!\n");
      return GASPI_ERROR;
    }
    for(int i = 0; i < num; ++i){
      if(((double*)op1)[i] > ((double*)op2)[i]){
          ((double*)res)[i] = ((double*)op1)[i];
      }else{
          ((double*)res)[i] = ((double*)op2)[i];
      }
    }
    return GASPI_SUCCESS;
 }
/*
 * Reduce Min operations
 */

  //reduce Op MIN char
  gaspi_return_t gaspi_op_MIN_char(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                          gaspi_state_t state, gaspi_number_t num,
                          gaspi_size_t element_size, gaspi_timeout_t timeout)
  {
     if(sizeof(char) != element_size){
        printf("Error: element_size does not match size of char!\n");
        return GASPI_ERROR;
     }
     for(int i = 0; i < num; ++i){
        if(((char*)op1)[i] < ((char*)op2)[i]){
           ((char*)res)[i] = ((char*)op1)[i];
        }else{
           ((char*)res)[i] = ((char*)op2)[i];
        }
     }
     return GASPI_SUCCESS;
  }
  //reduce Op MIN short
  gaspi_return_t gaspi_op_MIN_short(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                          gaspi_state_t state, gaspi_number_t num,
                          gaspi_size_t element_size, gaspi_timeout_t timeout)
  {
     if(sizeof(short) != element_size){
        printf("Error: element_size does not match size of short!\n");
        return GASPI_ERROR;
     }
     for(int i = 0; i < num; ++i){
        if(((short*)op1)[i] < ((short*)op2)[i]){
           ((short*)res)[i] = ((short*)op1)[i];
        }else{
           ((short*)res)[i] = ((short*)op2)[i];
        }
     }
     return GASPI_SUCCESS;
  }

  //reduce Op MIN int
  gaspi_return_t gaspi_op_MIN_int(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                          gaspi_state_t state, gaspi_number_t num,
                          gaspi_size_t element_size, gaspi_timeout_t timeout)
  {
     if(sizeof(int) != element_size){
        printf("Error: element_size does not match size of int!\n");
        return GASPI_ERROR;
     }
     for(int i = 0; i < num; ++i){
        if(((int*)op1)[i] < ((int*)op2)[i]){
           ((int*)res)[i] = ((int*)op1)[i];
        }else{
           ((int*)res)[i] = ((int*)op2)[i];
        }
     }
     return GASPI_SUCCESS;
  }

  //reduce Op MIN unsigned int
  gaspi_return_t gaspi_op_MIN_uInt(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                          gaspi_state_t state, gaspi_number_t num,
                          gaspi_size_t element_size, gaspi_timeout_t timeout)
  {
     if(sizeof(unsigned int) != element_size){
        printf("Error: element_size does not match size of unsigned int!\n");
        return GASPI_ERROR;
     }
     for(int i = 0; i < num; ++i){
        if(((unsigned int*)op1)[i] < ((unsigned int*)op2)[i]){
           ((unsigned int*)res)[i] = ((unsigned int*)op1)[i];
        }else{
           ((unsigned int*)res)[i] = ((unsigned int*)op2)[i];
        }
     }
     return GASPI_SUCCESS;
  }

  //reduce Op MIN long
  gaspi_return_t gaspi_op_MIN_long(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                          gaspi_state_t state, gaspi_number_t num,
                          gaspi_size_t element_size, gaspi_timeout_t timeout)
  {
     if(sizeof(long) != element_size){
        printf("Error: element_size does not match size of unsigned long!\n");
        return GASPI_ERROR;
     }
     for(int i = 0; i < num; ++i){
        if(((long*)op1)[i] < ((long*)op2)[i]){
           ((long*)res)[i] = ((long*)op1)[i];
        }else{
           ((long*)res)[i] = ((long*)op2)[i];
        }
     }
     return GASPI_SUCCESS;
  }

  //reduce Op MIN unsigned long
  gaspi_return_t gaspi_op_MIN_uLong(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                          gaspi_state_t state, gaspi_number_t num,
                          gaspi_size_t element_size, gaspi_timeout_t timeout)
  {
     if(sizeof(unsigned long) != element_size){
        printf("Error: element_size does not match size of unsigned long!\n");
        return GASPI_ERROR;
     }
     for(int i = 0; i < num; ++i){
        if(((unsigned long*)op1)[i] < ((unsigned long*)op2)[i]){
           ((unsigned long*)res)[i] = ((unsigned long*)op1)[i];
        }else{
           ((unsigned long*)res)[i] = ((unsigned long*)op2)[i];
        }
     }
     return GASPI_SUCCESS;
  }

  //reduce Op MIN long long
  gaspi_return_t gaspi_op_MIN_longLong(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                          gaspi_state_t state, gaspi_number_t num,
                          gaspi_size_t element_size, gaspi_timeout_t timeout)
  {
     if(sizeof(long long) != element_size){
        printf("Error: element_size does not match size of long long!\n");
        return GASPI_ERROR;
     }
     for(int i = 0; i < num; ++i){
        if(((long long*)op1)[i] < ((long long*)op2)[i]){
           ((long long*)res)[i] = ((long long*)op1)[i];
       }else{
           ((long long*)res)[i] = ((long long*)op2)[i];
       }
     }
     return GASPI_SUCCESS;
  }

  //reduce Op MIN float
  gaspi_return_t gaspi_op_MIN_float(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                          gaspi_state_t state, gaspi_number_t num,
                          gaspi_size_t element_size, gaspi_timeout_t timeout)
  {
     if(sizeof(float) != element_size){
        printf("Error: element_size does not match size of float!\n");
        return GASPI_ERROR;
     }
     for(int i = 0; i < num; ++i){
        if(((float*)op1)[i] < ((float*)op2)[i]){
           ((float*)res)[i] = ((float*)op1)[i];
        }else{
           ((float*)res)[i] = ((float*)op2)[i];
        }
     }
     return GASPI_SUCCESS;
  }

  //reduce Op MIN double
  gaspi_return_t gaspi_op_MIN_double(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                          gaspi_state_t state, gaspi_number_t num,
                          gaspi_size_t element_size, gaspi_timeout_t timeout)
  {
     if(sizeof(double) != element_size){
        printf("Error: element_size does not match size of float!\n");
        return GASPI_ERROR;
     }
     for(int i = 0; i < num; ++i){
        if(((double*)op1)[i] < ((double*)op2)[i]){
           ((double*)res)[i] = ((double*)op1)[i];
        }else{
           ((double*)res)[i] = ((double*)op2)[i];
        }
     }
     return GASPI_SUCCESS;
  }

 /*
  * Gaspi SUM reduce opartions
  */
 //reduce Op SUM char
 gaspi_return_t gaspi_op_SUM_char(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                         gaspi_state_t state, gaspi_number_t num,
                         gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(char) != element_size){
       printf("Error: element_size does not match size of char!\n");
       return GASPI_ERROR;
    }
    for(int i = 0; i < num; ++i){
      ((char*)res)[i] = ((char*)op1)[i] + ((char*)op2)[i];
    }
    return GASPI_SUCCESS;
 }
 //reduce Op SUM short
 gaspi_return_t gaspi_op_SUM_short(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                         gaspi_state_t state, gaspi_number_t num,
                         gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(short) != element_size){
       printf("Error: element_size does not match size of short!\n");
       return GASPI_ERROR;
    }
    for(int i = 0; i < num; ++i){
      ((short*)res)[i] = ((short*)op1)[i] + ((short*)op2)[i];
    }
    return GASPI_SUCCESS;
 }

 //reduce Op SUM int
 gaspi_return_t gaspi_op_SUM_int(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                         gaspi_state_t state, gaspi_number_t num,
                         gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(int) != element_size){
       printf("Error: element_size does not match size of int!\n");
       return GASPI_ERROR;
    }
    for(int i = 0; i < num; ++i){
      ((int*)res)[i] = ((int*)op1)[i] + ((int*)op2)[i];
    }
    return GASPI_SUCCESS;
 }

 //reduce Op SUM unsigned int
 gaspi_return_t gaspi_op_SUM_uInt(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                         gaspi_state_t state, gaspi_number_t num,
                         gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(unsigned int) != element_size){
       printf("Error: element_size does not match size of unsigned int!\n");
       return GASPI_ERROR;
    }
    for(int i = 0; i < num; ++i){
      ((unsigned int*)res)[i] = ((unsigned int*)op1)[i] + ((unsigned int*)op2)[i];
    }
    return GASPI_SUCCESS;
 }

 //reduce Op SUM long
 gaspi_return_t gaspi_op_SUM_long(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                         gaspi_state_t state, gaspi_number_t num,
                         gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(long) != element_size){
       printf("Error: element_size does not match size of unsigned long!\n");
       return GASPI_ERROR;
    }
    for(int i = 0; i < num; ++i){
      ((long*)res)[i] = ((long*)op1)[i] + ((long*)op2)[i];
    }
    return GASPI_SUCCESS;
 }

 //reduce Op SUM unsigned long
 gaspi_return_t gaspi_op_SUM_uLong(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                         gaspi_state_t state, gaspi_number_t num,
                         gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(unsigned long) != element_size){
       printf("Error: element_size does not match size of unsigned long!\n");
       return GASPI_ERROR;
    }
    for(int i = 0; i < num; ++i){
      ((unsigned long*)res)[i] = ((unsigned long*)op1)[i] + ((unsigned long*)op2)[i];
    }
    return GASPI_SUCCESS;
 }

 //reduce Op SUM long long
 gaspi_return_t gaspi_op_SUM_longLong(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                         gaspi_state_t state, gaspi_number_t num,
                         gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(long long) != element_size){
       printf("Error: element_size does not match size of long long!\n");
       return GASPI_ERROR;
    }
    for(int i = 0; i < num; ++i){
      ((long long*)res)[i] = ((long long*)op1)[i] + ((long long*)op2)[i];
    }
    return GASPI_SUCCESS;
 }

 //reduce Op SUM float
 gaspi_return_t gaspi_op_SUM_float(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                         gaspi_state_t state, gaspi_number_t num,
                         gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(float) != element_size){
       printf("Error: element_size does not match size of float!\n");
       return GASPI_ERROR;
    }
    for(int i = 0; i < num; ++i){
      ((float*)res)[i] = ((float*)op1)[i] + ((float*)op2)[i];
    }
    return GASPI_SUCCESS;
 }

 //reduce Op SUM double
 gaspi_return_t gaspi_op_SUM_double(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                         gaspi_state_t state, gaspi_number_t num,
                         gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(double) != element_size){
       printf("Error: element_size does not match size of float!\n");
       return GASPI_ERROR;
    }
    for(int i = 0; i < num; ++i){
      ((double*)res)[i] = ((double*)op1)[i] + ((double*)op2)[i];
    }
    return GASPI_SUCCESS;
 }

/*
 * Gaspi PROD reduce opartions
 */
 //reduce Op PROD char
 gaspi_return_t gaspi_op_PROD_char(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                         gaspi_state_t state, gaspi_number_t num,
                         gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(char) != element_size){
       printf("Error: element_size does not match size of char!\n");
       return GASPI_ERROR;
    }
    for(int i = 0; i < num; ++i){
      ((char*)res)[i] = ((char*)op1)[i] * ((char*)op2)[i];
    }
    return GASPI_SUCCESS;
 }
 //reduce Op PROD short
 gaspi_return_t gaspi_op_PROD_short(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                         gaspi_state_t state, gaspi_number_t num,
                         gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(short) != element_size){
       printf("Error: element_size does not match size of short!\n");
       return GASPI_ERROR;
    }
    for(int i = 0; i < num; ++i){
      ((short*)res)[i] = ((short*)op1)[i] * ((short*)op2)[i];
    }
    return GASPI_SUCCESS;
 }

 //reduce Op PROD int
 gaspi_return_t gaspi_op_PROD_int(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                         gaspi_state_t state, gaspi_number_t num,
                         gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(int) != element_size){
       printf("Error: element_size does not match size of int!\n");
       return GASPI_ERROR;
    }
    for(int i = 0; i < num; ++i){
      ((int*)res)[i] = ((int*)op1)[i] * ((int*)op2)[i];
    }
    return GASPI_SUCCESS;
 }

 //reduce Op PROD unsigned int
 gaspi_return_t gaspi_op_PROD_uInt(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                         gaspi_state_t state, gaspi_number_t num,
                         gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(unsigned int) != element_size){
       printf("Error: element_size does not match size of unsigned int!\n");
       return GASPI_ERROR;
    }
    for(int i = 0; i < num; ++i){
      ((unsigned int*)res)[i] = ((unsigned int*)op1)[i] * ((unsigned int*)op2)[i];
    }
    return GASPI_SUCCESS;
 }

 //reduce Op PROD long
 gaspi_return_t gaspi_op_PROD_long(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                         gaspi_state_t state, gaspi_number_t num,
                         gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(long) != element_size){
       printf("Error: element_size does not match size of unsigned long!\n");
       return GASPI_ERROR;
    }
    for(int i = 0; i < num; ++i){
      ((long*)res)[i] = ((long*)op1)[i] * ((long*)op2)[i];
    }
    return GASPI_SUCCESS;
 }

 //reduce Op PROD unsigned long
 gaspi_return_t gaspi_op_PROD_uLong(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                         gaspi_state_t state, gaspi_number_t num,
                         gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(unsigned long) != element_size){
       printf("Error: element_size does not match size of unsigned long!\n");
       return GASPI_ERROR;
    }
    for(int i = 0; i < num; ++i){
      ((unsigned long*)res)[i] = ((unsigned long*)op1)[i] * ((unsigned long*)op2)[i];
    }
    return GASPI_SUCCESS;
 }

 //reduce Op PROD long long
 gaspi_return_t gaspi_op_PROD_longLong(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                         gaspi_state_t state, gaspi_number_t num,
                         gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(long long) != element_size){
       printf("Error: element_size does not match size of long long!\n");
       return GASPI_ERROR;
    }
    for(int i = 0; i < num; ++i){
      ((long long*)res)[i] = ((long long*)op1)[i] * ((long long*)op2)[i];
    }
    return GASPI_SUCCESS;
 }

 //reduce Op PROD float
 gaspi_return_t gaspi_op_PROD_float(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                         gaspi_state_t state, gaspi_number_t num,
                         gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(float) != element_size){
       printf("Error: element_size does not match size of float!\n");
       return GASPI_ERROR;
    }
    for(int i = 0; i < num; ++i){
      ((float*)res)[i] = ((float*)op1)[i] * ((float*)op2)[i];
    }
    return GASPI_SUCCESS;
 }

 //reduce Op PROD double
 gaspi_return_t gaspi_op_PROD_double(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                         gaspi_state_t state, gaspi_number_t num,
                         gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(double) != element_size){
       printf("Error: element_size does not match size of float!\n");
       return GASPI_ERROR;
    }
    for(int i = 0; i < num; ++i){
      ((double*)res)[i] = ((double*)op1)[i] * ((double*)op2)[i];
    }
    return GASPI_SUCCESS;
 }

 /*
 *  Binary AND Operations
 *  supports integer and byte.
 */
 gaspi_return_t gaspi_op_BAND_int(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                         gaspi_state_t state, gaspi_number_t num,
                         gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(int) != element_size){
       printf("Error: element_size does not match size of int!\n");
       return GASPI_ERROR;
    }
    for(int i = 0; i < num; ++i){
      ((int*)res)[i] = ((int*)op1)[i] & ((int*)op2)[i];
    }
    return GASPI_SUCCESS;
 }

 gaspi_return_t gaspi_op_BAND_char(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                         gaspi_state_t state, gaspi_number_t num,
                         gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(char) != element_size){
       printf("Error: element_size does not match size of char!\n");
       return GASPI_ERROR;
    }
    for(int i = 0; i < num; ++i){
      ((char*)res)[i] = ((char*)op1)[i] & ((char*)op2)[i];
    }
    return GASPI_SUCCESS;
 }
 /*
 *  Logical AND Operations
 *  supports integer
 */
 gaspi_return_t gaspi_op_LAND_int(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                         gaspi_state_t state, gaspi_number_t num,
                         gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(int) != element_size){
       printf("Error: element_size does not match size of int!\n");
       return GASPI_ERROR;
    }
    for(int i = 0; i < num; ++i){
      ((int*)res)[i] = ((int*)op1)[i] && ((int*)op2)[i];
    }
    return GASPI_SUCCESS;
 }

 /*
 *  Binary OR Operations
 *  supports integer and byte.
 */

 gaspi_return_t gaspi_op_BOR_int(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                         gaspi_state_t state, gaspi_number_t num,
                         gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(int) != element_size){
       printf("Error: element_size does not match size of int!\n");
       return GASPI_ERROR;
    }
    for(int i = 0; i < num; ++i){
      ((int*)res)[i] = ((int*)op1)[i] | ((int*)op2)[i];
    }
    return GASPI_SUCCESS;
 }

 gaspi_return_t gaspi_op_BOR_char(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                         gaspi_state_t state, gaspi_number_t num,
                         gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(int) != element_size){
       printf("Error: element_size does not match size of char!\n");
       return GASPI_ERROR;
    }
    for(int i = 0; i < num; ++i){
      ((char*)res)[i] = ((char*)op1)[i] | ((char*)op2)[i];
    }
    return GASPI_SUCCESS;
 }

 /*
 *  Logical OR Operations
 *  supports integer and byte.
 */

 gaspi_return_t gaspi_op_LOR_int(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                         gaspi_state_t state, gaspi_number_t num,
                         gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(int) != element_size){
       printf("Error: element_size does not match size of int!\n");
       return GASPI_ERROR;
    }
    for(int i = 0; i < num; ++i){
      ((int*)res)[i] = ((int*)op1)[i] || ((int*)op2)[i];
    }
    return GASPI_SUCCESS;
 }

 gaspi_return_t gaspi_op_LOR_char(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                         gaspi_state_t state, gaspi_number_t num,
                         gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(int) != element_size){
       printf("Error: element_size does not match size of char!\n");
       return GASPI_ERROR;
    }
    for(int i = 0; i < num; ++i){
      ((char*)res)[i] = ((char*)op1)[i] || ((char*)op2)[i];
    }
    return GASPI_SUCCESS;
 }

 /*
 *  Binary XOR Operations
 *  supports integer and byte.
 */

 gaspi_return_t gaspi_op_BXOR_int(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                         gaspi_state_t state, gaspi_number_t num,
                         gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(int) != element_size){
       printf("Error: element_size does not match size of int!\n");
       return GASPI_ERROR;
    }
    for(int i = 0; i < num; ++i){
      ((int*)res)[i] = ((int*)op1)[i] ^ ((int*)op2)[i];
    }
    return GASPI_SUCCESS;
 }

 gaspi_return_t gaspi_op_BXOR_char(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                         gaspi_state_t state, gaspi_number_t num,
                         gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(int) != element_size){
       printf("Error: element_size does not match size of char!\n");
       return GASPI_ERROR;
    }
    for(int i = 0; i < num; ++i){
      ((char*)res)[i] = ((char*)op1)[i] ^ ((char*)op2)[i];
    }
    return GASPI_SUCCESS;
 }

 /*
 *  Locgical XOR Operations
 *  supports integer.
 */
 gaspi_return_t gaspi_op_LXOR_int(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                         gaspi_state_t state, gaspi_number_t num,
                         gaspi_size_t element_size, gaspi_timeout_t timeout)
 {
    if(sizeof(int) != element_size){
       printf("Error: element_size does not match size of int!\n");
       return GASPI_ERROR;
    }
    for(int i = 0; i < num; ++i){
      ((int*)res)[i] = (!((int*)op1)[i] != !((int*)op2)[i]);
    }
    return GASPI_SUCCESS;
 }

//TODO implementation
 dart_ret_t
 dart_type_create_strided(
   dart_datatype_t   basetype_id,
   size_t            stride,
   size_t            blocklen,
   dart_datatype_t * newtype)
 {
   fprintf(stderr, "dart-gaspi: %s function not supported\n", __func__);
   return DART_ERR_NOTFOUND;
 }

//TODO implementation
dart_ret_t
dart_type_create_indexed(
   dart_datatype_t   basetype,
   size_t            count,
   const size_t      blocklen[],
   const size_t      offset[],
   dart_datatype_t * newtype)
{
   fprintf(stderr, "dart-gaspi: %s function not supported\n", __func__);
   return DART_ERR_NOTFOUND;
}

//TODO implementation
dart_ret_t dart_type_destroy(dart_datatype_t *dart_type_ptr){
   fprintf(stderr, "dart-gaspi: %s function not supported\n", __func__);
   return DART_OK;
}
