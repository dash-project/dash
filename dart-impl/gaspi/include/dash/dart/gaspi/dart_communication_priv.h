#ifndef DART_COMMUNICATION_PRIV_H
#define DART_COMMUNICATION_PRIV_H

#include <stdio.h>
#include <GASPI.h>
#include <dash/dart/if/dart.h>
#include <dash/dart/gaspi/dart_types.h>
#include <dash/dart/gaspi/rbtree.h>
#include "dart_team_private.h"

#include <dash/dart/base/logging.h>


#define CHECK_EQUAL_BASETYPE(_src_type, _dst_type)                            \
  do {                                                                        \
    if (!datatype_samebase(_src_type, _dst_type)){            \
      DART_LOG_ERROR("%s ! Cannot convert base-types",                        \
          __func__);                                                          \
      return DART_ERR_INVAL;                                                  \
    }                                                                         \
  } while (0)

#define CHECK_NUM_ELEM(_src_type, _dst_type, _num_elem)                       \
  do {                                                                        \
    size_t src_num_elem = datatype_num_elem(_src_type);                       \
    size_t dst_num_elem = datatype_num_elem(_dst_type);                       \
    if ((_num_elem % src_num_elem) != 0 || (_num_elem % dst_num_elem) != 0) { \
      DART_LOG_ERROR(                                                         \
          "%s ! Type-mismatch would lead to truncation (%zu elems)",          \
          __func__,  _num_elem);                                              \
      return DART_ERR_INVAL;                                                  \
    }                                                                         \
  } while (0)

#define CHECK_TYPE_CONSTRAINTS(_src_type, _dst_type, _num_elem)               \
  CHECK_EQUAL_BASETYPE(_src_type, _dst_type);                                 \
  CHECK_NUM_ELEM(_src_type, _dst_type, _num_elem);

#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))






// local gaspi segment id entry
typedef struct local_gseg_id_entry
{
    gaspi_segment_id_t          local_gseg_id;
    struct local_gseg_id_entry* next;
} local_gseg_id_entry_t;

typedef struct{
    dart_unit_t            key;
    gaspi_queue_id_t       queue;
    local_gseg_id_entry_t* begin_seg_ids;
    local_gseg_id_entry_t* end_seg_ids;
}request_table_entry_t;

typedef tree_iterator* request_iterator_t;
request_iterator_t new_request_iter(int16_t gaspi_seg);
dart_ret_t destroy_request_iter(request_iterator_t iter);
uint8_t request_iter_is_vaild(request_iterator_t iter);
dart_ret_t request_iter_get_entry(request_iterator_t iter, request_table_entry_t** request_entry);
dart_ret_t request_iter_next(request_iterator_t iter);

dart_ret_t inital_rma_request_table();
dart_ret_t destroy_rma_request_table();
dart_ret_t find_rma_request(dart_unit_t target_unit, int16_t seg_id, request_table_entry_t** request_entry);
dart_ret_t add_rma_request_entry(dart_unit_t target_unit, int16_t seg_id, gaspi_segment_id_t local_gseg_id, request_table_entry_t** request_entry);
dart_ret_t inital_rma_request_entry(int16_t seg_id);
dart_ret_t delete_rma_requests(int16_t seg_id);

dart_ret_t free_segment_ids(request_table_entry_t* request_entry);

typedef enum {
  GASPI_WRITE = 0,
  GASPI_READ
} communication_kind_t;

typedef enum {
  GASPI_LOCAL = 0,
  GASPI_GLOBAL
} access_kind_t;

struct dart_handle_struct
{
    communication_kind_t comm_kind;
    // also used for local notification id and value
    gaspi_segment_id_t local_seg_id;
    // used for put remote completion
    gaspi_notification_id_t notify_remote;
    gaspi_queue_id_t queue;
};

typedef enum {
  DART_BLOCK_SINGLE = 0,
  DART_BLOCK_MULTIPLE
} block_kind_t;


typedef struct chunk_info
{
    /// the source offsets of each block
    size_t         src;
    /// the destination offsets of each block
    size_t         dst;
    /// number of bytes to read/write per block
    size_t          nbyte;
} chunk_info_t;

typedef struct converted_types
{
    /// the number of blocks
    size_t           num_blocks;
    /// tags if offsets and nbytes are the same for each block or not
    block_kind_t     kind;
    /// single entry (e.g. contiguous or stride) or multiple entries (e.g. indexed)
    union
    {
        chunk_info_t single;

        chunk_info_t* multiple;
    };
} converted_type_t;

dart_ret_t unit_g2l (uint16_t index, dart_unit_t abs_id, dart_unit_t *rel_id);
dart_ret_t unit_l2g (uint16_t index, dart_unit_t *abs_id, dart_unit_t rel_id);
dart_ret_t dart_get_minimal_queue(gaspi_queue_id_t * qid);
gaspi_queue_id_t dart_handle_get_queue(dart_handle_t handle);

dart_ret_t glob_unit_gaspi_seg(dart_gptr_t* gptr, dart_unit_t* global_unit_id, gaspi_segment_id_t* gaspi_seg_id, const char* location);

void free_converted_type(converted_type_t* conv_type);
dart_ret_t dart_convert_type(dart_datatype_struct_t* dts_src,
                             dart_datatype_struct_t* dts_dst,
                             size_t nelem,
                             converted_type_t* conv_type);


gaspi_return_t local_get(dart_gptr_t* gptr, gaspi_segment_id_t gaspi_src_segment_id, void* dst, converted_type_t* conv_type);
gaspi_return_t local_put(dart_gptr_t* gptr, gaspi_segment_id_t gaspi_dst_segment_id, const void* src, converted_type_t* conv_type);

gaspi_return_t remote_get(dart_gptr_t* gptr, gaspi_rank_t src_unit, gaspi_segment_id_t src_seg_id, gaspi_segment_id_t dst_seg_id, void* dst, gaspi_queue_id_t* queue, converted_type_t* conv_type);
gaspi_return_t remote_put(dart_gptr_t* gptr, gaspi_rank_t dst_unit, gaspi_segment_id_t dst_seg_id, gaspi_segment_id_t src_seg_id, void* src, gaspi_queue_id_t* queue, converted_type_t* conv_type);

gaspi_return_t put_completion_test(gaspi_rank_t dst_unit, gaspi_queue_id_t queue);

dart_ret_t dart_test_impl(dart_handle_t* handleptr, int32_t * is_finished, gaspi_notification_id_t notify_id_to_check);
dart_ret_t dart_test_all_impl(dart_handle_t handleptr[], size_t num_handles, int32_t * is_finished, access_kind_t access_kind);

dart_ret_t error_cleanup(converted_type_t* conv_type);
dart_ret_t error_cleanup_seg(gaspi_segment_id_t used_segment_id, converted_type_t* conv_type);

#define DART_CHECK_ERROR_CLEAN_TEMPL(expected, error_code, ret_type, conv_type, func...)  \
  do {                                                                                       \
    const ret_type retval = func;                                                            \
    if (retval != expected) {                                                                \
      printf("ERROR in %s : %s on line %i return value %i\n", #func,                         \
                   __FILE__, __LINE__, retval);                                              \
      error_cleanup(&conv_type);                                                             \
                                                                                             \
      return error_code;                                                                     \
    }                                                                                        \
  }while (0)

#define DART_CHECK_ERROR_CLEAN_TEMPL_SEG(expected, error_code, ret_type, seg_id, conv_type, func...)  \
  do {                                                                                       \
    const ret_type retval = func;                                                            \
    if (retval != expected) {                                                                \
      printf("ERROR in %s : %s on line %i return value %i\n", #func,                         \
                   __FILE__, __LINE__, retval);                                              \
      error_cleanup_seg(seg_id, &conv_type);                                                     \
                                                                                             \
      return error_code;                                                                     \
    }                                                                                        \
  }while (0)

#define DART_CHECK_ERROR_CLEAN(conv_type, func...) \
    DART_CHECK_ERROR_CLEAN_TEMPL(DART_OK, DART_ERR_OTHER, dart_ret_t, conv_type, func)

#define DART_CHECK_ERROR_CLEAN_SEG(seg_id, conv_type, func...) \
    DART_CHECK_ERROR_CLEAN_TEMPL_SEG(DART_OK, DART_ERR_OTHER, dart_ret_t, seg_id, conv_type, func)

#define DART_CHECK_GASPI_ERROR_CLEAN(conv_type, func...) \
    DART_CHECK_ERROR_CLEAN_TEMPL(GASPI_SUCCESS, GASPI_ERROR, gaspi_return_t, conv_type, func)

#define DART_CHECK_GASPI_ERROR_CLEAN_SEG(seg_id, conv_type, func...) \
    DART_CHECK_ERROR_CLEAN_TEMPL_SEG(GASPI_SUCCESS, GASPI_ERROR, gaspi_return_t, seg_id, conv_type, func)

/*
* Headers for reduce operations used for gaspi_reduce_user and gaspi_allreduce_user
*/

#define DART_NAME_OP(_op_type, _name) gaspi_op_##_op_type##_##_name

#define DART_DECLARE_OP(_op_type, _name) \
gaspi_return_t DART_NAME_OP(_op_type, _name) (            \
  gaspi_pointer_t op1, gaspi_pointer_t op2, gaspi_pointer_t res, \
  gaspi_reduce_state_t state, gaspi_number_t num,\
   gaspi_size_t element_size, gaspi_timeout_t timeout)

#define DART_DECLARE_OP_INT(_name) \
   DART_DECLARE_OP(_name, short); \
   DART_DECLARE_OP(_name, int); \
   DART_DECLARE_OP(_name, uInt); \
   DART_DECLARE_OP(_name, long); \
   DART_DECLARE_OP(_name, uLong); \
   DART_DECLARE_OP(_name, longLong); \
   DART_DECLARE_OP(_name, uLongLong);

#define DART_DECLARE_OP_INT_BYTE(_name) \
   DART_DECLARE_OP(_name, char); \
   DART_DECLARE_OP_INT(_name)

#define DART_DECLARE_OP_ALL(_name) \
   DART_DECLARE_OP_INT_BYTE(_name) \
   DART_DECLARE_OP(_name, float); \
   DART_DECLARE_OP(_name, double); \
   DART_DECLARE_OP(_name, longDouble);

DART_DECLARE_OP_ALL(MINMAX)
DART_DECLARE_OP_ALL(MIN)
DART_DECLARE_OP_ALL(MAX)
DART_DECLARE_OP_ALL(SUM)
DART_DECLARE_OP_ALL(PROD)

DART_DECLARE_OP_INT(LAND)
DART_DECLARE_OP_INT(LOR)
DART_DECLARE_OP_INT(LXOR)

DART_DECLARE_OP_INT_BYTE(BAND)
DART_DECLARE_OP_INT_BYTE(BOR)
DART_DECLARE_OP_INT_BYTE(BXOR)



#endif /* DART_COMMUNICATION_PRIV_H */