#ifndef DART_COMMUNICATION_PRIV_H
#define DART_COMMUNICATION_PRIV_H

#include <stdio.h>
#include <GASPI.h>
#include "dart.h"
#include "dart_team_private.h"

struct dart_handle_struct
{
    gaspi_offset_t local_offset;
    gaspi_segment_id_t local_seg;
    gaspi_queue_id_t queue;
    void * dest_buffer;
    size_t nbytes;
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
