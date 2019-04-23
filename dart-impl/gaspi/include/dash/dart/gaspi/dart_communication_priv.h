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


typedef struct offset_pair
{
    /// the source offsets of each block
    size_t         src;
    /// the destination offsets of each block
    size_t         dst;
} offset_pair_t;

typedef struct single
{
            /// source and destination offsets of each block
            offset_pair_t   offset;
            /// number of bytes to read/write per block
            size_t          nbyte;
} single_t;

typedef struct multiple
{
            /// source and destination offsets of each block
            offset_pair_t*   offsets;
            /// number of bytes to read/write per block
            size_t*          nbytes;
}  multiple_t;

typedef struct converted_types
{
    /// the number of blocks
    size_t           num_blocks;
    /// tags if offsets and nbytes are the same for each block or not
    block_kind_t     kind;
    /// single entry (e.g. contiguous or stride) or multiple entries (e.g. indexed)
    union
    {
        single_t single;

        multiple_t multiple;
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


dart_ret_t local_get(dart_gptr_t* gptr, gaspi_segment_id_t gaspi_src_segment_id, void* dst, converted_type_t* conv_type);
dart_ret_t local_put(dart_gptr_t* gptr, gaspi_segment_id_t gaspi_dst_segment_id, const void* src, converted_type_t* conv_type);

gaspi_return_t remote_get(dart_gptr_t* gptr, gaspi_rank_t src_unit, gaspi_segment_id_t src_seg_id, gaspi_segment_id_t dst_seg_id, void* dst, gaspi_queue_id_t* queue, converted_type_t* conv_type);
gaspi_return_t remote_put(dart_gptr_t* gptr, gaspi_rank_t dst_unit, gaspi_segment_id_t dst_seg_id, gaspi_segment_id_t src_seg_id, void* src, gaspi_queue_id_t* queue, converted_type_t* conv_type);

gaspi_return_t put_completion_test(gaspi_rank_t dst_unit, gaspi_queue_id_t queue);

dart_ret_t dart_test_impl(dart_handle_t* handleptr, int32_t * is_finished, gaspi_notification_id_t notify_id_to_check);
dart_ret_t dart_test_all_impl(dart_handle_t handleptr[], size_t num_handles, int32_t * is_finished, access_kind_t access_kind);

#endif /* DART_COMMUNICATION_PRIV_H */
