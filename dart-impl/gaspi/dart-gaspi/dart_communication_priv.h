#ifndef DART_COMMUNICATION_PRIV_H
#define DART_COMMUNICATION_PRIV_H

#include <stdio.h>
#include <GASPI.h>
#include "dart_communication.h"

struct dart_handle_struct
{
    gaspi_offset_t local_offset;
    gaspi_segment_id_t local_seg;
    gaspi_queue_id_t queue;
    void * dest_buffer;
    size_t nbytes;
};

gaspi_queue_id_t dart_handle_get_queue(dart_handle_t handle);

#endif /* DART_COMMUNICATION_PRIV_H */
