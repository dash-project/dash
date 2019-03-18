#ifndef DART_COMMUNICATION_PRIV_H
#define DART_COMMUNICATION_PRIV_H

#include <stdio.h>
#include <GASPI.h>
#include <dash/dart/if/dart.h>
#include <dash/dart/gaspi/rbtree.h>
#include "dart_team_private.h"

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

dart_ret_t unit_g2l (uint16_t index, dart_unit_t abs_id, dart_unit_t *rel_id);
dart_ret_t unit_l2g (uint16_t index, dart_unit_t *abs_id, dart_unit_t rel_id);
dart_ret_t dart_get_minimal_queue(gaspi_queue_id_t * qid);
gaspi_queue_id_t dart_handle_get_queue(dart_handle_t handle);

dart_ret_t check_seg_id(dart_gptr_t* gptr, dart_unit_t* global_unit_id, gaspi_segment_id_t* gaspi_seg_id, const char* location);
dart_ret_t local_copy_get(dart_gptr_t* gptr, gaspi_segment_id_t gaspi_src_segment_id, void* dest, size_t nbytes);
dart_ret_t local_copy_put(dart_gptr_t* gptr, gaspi_segment_id_t gaspi_dst_segment_id, const void* src, size_t nbytes);


#endif /* DART_COMMUNICATION_PRIV_H */
