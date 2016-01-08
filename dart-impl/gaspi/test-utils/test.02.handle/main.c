#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <handle_queue.h>


void print_handle(struct dart_handle_struct h)
{
    printf("Segment %d\n", h.local_seg);
    printf("Offset  %d\n", h.local_offset);
    printf("nbytes  %d\n", h.nbytes);
    printf("Queue   %d\n", h.queue);
}

int main (int argc , char ** argv)
{
    size_t handle_count = 10;
    struct dart_handle_struct h;
    struct dart_handle_struct d;

    queue_t q;

    init_handle_queue(&q);

    for(int i = 0; i < handle_count ; ++i)
    {
        h.local_seg = i;
        h.local_offset = i;
        h.queue = i;
        h.nbytes = i;

        enqueue_handle(&q, h);
    }

    for(int i = 0; i < handle_count ; ++i)
    {
        front_handle(&q, &d);
        print_handle(d);
        dequeue_handle(&q);
    }

    destroy_handle_queue(&q);
    return 0;
}
