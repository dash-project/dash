#ifndef DART_COMMUNICATION_PRIV_H
#define DART_COMMUNICATION_PRIV_H

#include <stdio.h>
#include <GASPI.h>
#include <dash/dart/if/dart.h>
#include <dash/dart/gaspi/dart_types.h>
#include <dash/dart/gaspi/rbtree.h>
#include "dart_team_private.h"


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

typedef tree_iterator* request_iterator_t;
request_iterator_t new_request_iter(int16_t gaspi_seg);
dart_ret_t destroy_request_iter(request_iterator_t iter);
uint8_t request_iter_is_vaild(request_iterator_t iter);
dart_ret_t request_iter_get_queue(request_iterator_t iter, gaspi_queue_id_t * qid);
dart_ret_t request_iter_next(request_iterator_t iter);

dart_ret_t inital_rma_request_table();
dart_ret_t destroy_rma_request_table();
dart_ret_t find_rma_request(dart_unit_t target_unit, int16_t seg_id, gaspi_queue_id_t * qid, char * found);
dart_ret_t add_rma_request_entry(dart_unit_t target_unit, int16_t seg_id, gaspi_queue_id_t qid);
dart_ret_t inital_rma_request_entry(int16_t seg_id);
dart_ret_t delete_rma_requests(int16_t seg_id);

struct dart_handle_struct
{
    gaspi_segment_id_t local_seg_id;
    gaspi_segment_id_t remote_seg_id;
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

struct type_complex_meta
{
    size_t nbytes_read;
    size_t loop_inc_j;
    size_t loop_count;
    uint64_t offset_src;
    uint64_t offset_dst;
    size_t offset_inc_src;
    size_t offset_inc_dst;
};

dart_ret_t unit_g2l (uint16_t index, dart_unit_t abs_id, dart_unit_t *rel_id);
dart_ret_t unit_l2g (uint16_t index, dart_unit_t *abs_id, dart_unit_t rel_id);
dart_ret_t dart_get_minimal_queue(gaspi_queue_id_t * qid);
gaspi_queue_id_t dart_handle_get_queue(dart_handle_t handle);

dart_ret_t check_seg_id(dart_gptr_t* gptr, dart_unit_t* global_unit_id, gaspi_segment_id_t* gaspi_seg_id, const char* location);
dart_ret_t local_copy_get(dart_gptr_t* gptr, gaspi_segment_id_t gaspi_src_segment_id, void* dst, converted_type_t* conv_type);
dart_ret_t local_copy_put(dart_gptr_t* gptr, gaspi_segment_id_t gaspi_dst_segment_id, const void* src, converted_type_t* conv_types);

dart_ret_t dart_convert_type(dart_datatype_struct_t* dts_src,
                             dart_datatype_struct_t* dts_dst,
                             size_t nelem,
                             size_t nbytes_elem,
                             converted_type_t* conv_types);

#endif /* DART_COMMUNICATION_PRIV_H */
