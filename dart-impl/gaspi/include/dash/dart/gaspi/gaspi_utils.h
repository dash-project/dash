#ifndef GASPI_UTILS_H
#define GASPI_UTILS_H

#include <dash/dart/if/dart_types.h>
#include <stdlib.h>
#include <stdio.h>
#include <GASPI.h>

#define DART_MAX_SEGS 256

/* Function to resolve dart_data_type and return
 * size of type
*/
size_t dart_gaspi_datatype_sizeof(dart_datatype_t in);
size_t dart_max_segs();

gaspi_return_t gaspi_reduce_user (
   const void* buffer_send,
	void* const buffer_receive,
	const gaspi_number_t num,
	const gaspi_size_t element_size,
	gaspi_reduce_operation_t const reduce_operation,
	gaspi_reduce_state_t const reduce_state,
	const gaspi_group_t group,
   gaspi_segment_id_t* segment_ids,
   const gaspi_rank_t root,
	const gaspi_timeout_t timeout_ms);

gaspi_return_t create_segment(const gaspi_size_t size,
                              gaspi_segment_id_t *seg_id);

gaspi_return_t delete_all_segments();

gaspi_return_t check_queue_size(gaspi_queue_id_t queue);

gaspi_return_t wait_for_queue_entries(gaspi_queue_id_t *queue,
                                      gaspi_number_t wanted_entries);

gaspi_return_t blocking_waitsome(gaspi_notification_id_t id_begin,
                                 gaspi_notification_id_t id_count,
                                 gaspi_notification_id_t *id_available,
                                 gaspi_notification_t *notify_val,
                                 gaspi_segment_id_t seg);

gaspi_return_t flush_queues(gaspi_queue_id_t queue_begin, gaspi_queue_id_t queue_count);

int gaspi_utils_compute_comms(int *parent, int **children, int me, int root, gaspi_rank_t size);

#define DART_CHECK_ERROR_GOTO_TEMPL(expected, error_code, ret_type, label, func...)          \
  do {                                                                                       \
    const ret_type retval = func;                                                            \
    if (retval != expected) {                                                                \
      printf("ERROR in %s : %s on line %i return value %i\n", #func,                   \
                   __FILE__, __LINE__, retval);                                              \
      goto label;                                                                            \
    }                                                                                        \
  }while (0)

#define DART_CHECK_ERROR_GOTO(label, func...) \
    DART_CHECK_ERROR_GOTO_TEMPL(DART_OK, DART_ERR_OTHER, dart_ret_t, label, func)

#define DART_CHECK_GASPI_ERROR_GOTO(label, func...) \
    DART_CHECK_ERROR_GOTO_TEMPL(GASPI_SUCCESS, GASPI_ERROR, gaspi_return_t, label, func)

#define DART_CHECK_ERROR_TEMPL(expected, error_code, ret_type, func...)          \
  do {                                                                           \
    const ret_type retval = func;                                                \
    if (retval != expected) {                                                    \
      printf("ERROR in %s : %s on line %i return value %i\n", #func,       \
                   __FILE__, __LINE__, retval);                                  \
      return error_code;                                                         \
    }                                                                            \
  }while (0)

#define DART_CHECK_ERROR(func...) DART_CHECK_ERROR_TEMPL(DART_OK, DART_ERR_OTHER, dart_ret_t, func)
#define DART_CHECK_ERROR(func...) DART_CHECK_ERROR_TEMPL(DART_OK, DART_ERR_OTHER, dart_ret_t, func)

#define DART_CHECK_GASPI_ERROR(func...) DART_CHECK_ERROR_TEMPL(GASPI_SUCCESS, GASPI_ERROR, gaspi_return_t, func)

#define DART_CHECK_ERROR_RET(ret, func...)                                       \
    do {                                                                         \
      ret = func;                                                                \
      if (ret != GASPI_SUCCESS) {                                                \
        printf("ERROR in %s : %s on line %i return value %i\n", #func,     \
                     __FILE__, __LINE__, ret);                                   \
      }                                                                          \
    }while (0)

/*
* Headers for reduce operations used for gaspi_reduce_user and gaspi_allreduce_user
*/

//MINMAX operations for each supported data type
gaspi_return_t gaspi_op_MINMAX_char(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);
                        
gaspi_return_t gaspi_op_MINMAX_short(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_MINMAX_int(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_MINMAX_uInt(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_MINMAX_long(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_MINMAX_uLong(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_MINMAX_longLong(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_MINMAX_float(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_MINMAX_double(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);                                                                                                                                                                                                                

//MAX operations for each supported data type
gaspi_return_t gaspi_op_MAX_char(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_MAX_short(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_MAX_int(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_MAX_uInt(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_MAX_long(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_MAX_uLong(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_MAX_longLong(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_MAX_float(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_MAX_double(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

//MIN operations for each supported data type
gaspi_return_t gaspi_op_MIN_char(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_MIN_short(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_MIN_int(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_MIN_uInt(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_MIN_long(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_MIN_uLong(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_MIN_longLong(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_MIN_float(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_MIN_double(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

//Sum operations for each supported data type
gaspi_return_t gaspi_op_SUM_char(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_SUM_short(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_SUM_int(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_SUM_uInt(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_SUM_long(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_SUM_uLong(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_SUM_longLong(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_SUM_float(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_SUM_double(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

//Product operations for each supported data type
gaspi_return_t gaspi_op_PROD_char(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_PROD_short(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_PROD_int(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_PROD_uInt(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_PROD_long(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_PROD_uLong(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_PROD_longLong(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_PROD_float(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_PROD_double(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

/*
 *  Binary AND Operations
 *  supports integer and byte.
 */
gaspi_return_t gaspi_op_BAND_int(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_BAND_char(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

/*
 *  Logical AND Operations
 *  supports integer
 */
gaspi_return_t gaspi_op_LAND_int(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

/*
 *  Binary OR Operations
 *  supports integer and byte.
 */
gaspi_return_t gaspi_op_BOR_int(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                         gaspi_state_t state, gaspi_number_t num,
                         gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_BOR_char(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

/*
 *  Logical OR Operations
 *  supports integer and byte.
 */
gaspi_return_t gaspi_op_LOR_int(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_LOR_char(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

/*
 *  Binary XOR Operations
 *  supports integer and byte.
 */

gaspi_return_t gaspi_op_BXOR_int(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

gaspi_return_t gaspi_op_BXOR_char(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

/*
 *  Locgical XOR Operations
 *  supports integer.
 */

gaspi_return_t gaspi_op_LXOR_int(gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res,
                        gaspi_state_t state, gaspi_number_t num,
                        gaspi_size_t element_size, gaspi_timeout_t timeout);

dart_ret_t  dart_type_create_strided(dart_datatype_t   basetype_id, size_t stride,
                                     size_t blocklen, dart_datatype_t * newtype);

dart_ret_t  dart_type_create_indexed(dart_datatype_t   basetype, size_t count,
                                     const size_t blocklen[], const size_t offset[],
                                     dart_datatype_t * newtype);

dart_ret_t dart_type_destroy(dart_datatype_t *dart_type_ptr);

#endif // GASPI_UTILS_H
