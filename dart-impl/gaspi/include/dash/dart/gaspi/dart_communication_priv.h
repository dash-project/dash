#ifndef DART_COMMUNICATION_PRIV_H
#define DART_COMMUNICATION_PRIV_H

#include <stdio.h>
#include <GASPI.h>
#include <dash/dart/if/dart.h>
#include "dart_team_private.h"

dart_ret_t inital_rma_request_table();
dart_ret_t destroy_rma_request_table();
dart_ret_t find_rma_request(dart_unit_t target_unit, gaspi_segment_id_t seg_id, gaspi_queue_id_t * qid, int32_t * found);
dart_ret_t add_rma_request_entry(dart_unit_t target_unit, gaspi_segment_id_t seg_id, gaspi_queue_id_t qid);

struct dart_handle_struct
{
    gaspi_segment_id_t local_seg;
    gaspi_segment_id_t remote_seg;
    gaspi_queue_id_t queue;
};


gaspi_return_t
gaspi_bcast(const gaspi_segment_id_t seg_id,
            const gaspi_offset_t     offset,
            const gaspi_size_t       bytesize,
            const gaspi_rank_t       root,
            const gaspi_group_t      group);

gaspi_return_t
gaspi_allgather(const gaspi_segment_id_t send_segid,
                const gaspi_offset_t     send_offset,
                const gaspi_segment_id_t recv_segid,
                const gaspi_offset_t     recv_offset,
                const gaspi_size_t       byte_size,
                const gaspi_group_t      group);

dart_ret_t unit_g2l (uint16_t index, dart_unit_t abs_id, dart_unit_t *rel_id);
dart_ret_t unit_l2g (uint16_t index, dart_unit_t *abs_id, dart_unit_t rel_id);
dart_ret_t dart_get_minimal_queue(gaspi_queue_id_t * qid);
gaspi_queue_id_t dart_handle_get_queue(dart_handle_t handle);

#endif /* DART_COMMUNICATION_PRIV_H */
