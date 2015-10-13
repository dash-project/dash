#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include <test.h>
#include <dart.h>
#include <assert.h>
#include <GASPI.h>

#include <dart_translation.h>
#include <dart_communication_priv.h>

int main(int argc, char* argv[])
{
    dart_unit_t myid;
    dart_gptr_t g1;
    size_t size;
    void * ptr;
    gaspi_segment_id_t notify_seg = 1;
    const gaspi_notification_id_t notify_id = 1;
    const gaspi_notification_t notify_value = 42;

    CHECK(dart_init(&argc, &argv));
    CHECK(dart_myid(&myid));
    CHECK(dart_size(&size));
    CHECK(dart_memalloc(1024, &g1));
    CHECK(dart_gptr_getaddr(g1, &ptr));
    *((int *) ptr) = 42;

    gaspi_pointer_t gaspi_ptr;
    gaspi_segment_ptr(1, &gaspi_ptr);

    int * gaspi_int_ptr = (int *) ((char *) gaspi_ptr + g1.addr_or_offs.offset);
    if(*((int *) ptr) != *gaspi_int_ptr)
    {
        fprintf(stderr,"Error: Wrong value in segment or offset invalid\n");
    }

    CHECK(dart_barrier(DART_TEAM_ALL));

    if(myid == 1)
    {
        int recv = 1337;
        gaspi_queue_id_t qid;
        dart_get_minimal_queue(&qid);
        CHECK(dart_gptr_setunit(&g1, 0));
        CHECK(dart_put(g1, &recv, sizeof(int)));
        gaspi_notify(notify_seg, 0, notify_id, notify_value, qid, GASPI_BLOCK);

        CHECK(dart_flush_local(g1));
    }
    else if(myid == 0)
    {
        gaspi_notification_id_t id_received;
        gaspi_notification_t val;
        gaspi_notify_waitsome(notify_seg, 0, 4, &id_received, GASPI_BLOCK);
        gaspi_notify_reset(notify_seg, id_received, &val);
        if(val != notify_value || id_received != notify_id)
        {
            gaspi_printf("Get wrong notification\n");
        }
        gaspi_printf("Received value %d\n", *gaspi_int_ptr);
    }

    CHECK(dart_barrier(DART_TEAM_ALL));
    CHECK(dart_memfree(g1));

    dart_gptr_t g2;
    CHECK(dart_team_memalloc_aligned(DART_TEAM_ALL, 1024, &g2));

    dart_adapt_transtable_get_local_gaspi_seg_id(g2.segid, &notify_seg);

    gaspi_pointer_t g2_ptr;
    gaspi_segment_ptr(notify_seg, &g2_ptr);

    if(myid == 1)
    {
        int recv = 4200;
        gaspi_queue_id_t qid;
        dart_get_minimal_queue(&qid);
        CHECK(dart_gptr_setunit(&g2, 0));

        CHECK(dart_put(g2, &recv, sizeof(int)));
        gaspi_notify(notify_seg, 0, notify_id, notify_value, qid, GASPI_BLOCK);

        CHECK(dart_flush_local(g2));
    }
    else if(myid == 0)
    {
        gaspi_notification_id_t id_received;
        gaspi_notification_t val;
        gaspi_notify_waitsome(notify_seg, 0, 4, &id_received, GASPI_BLOCK);
        gaspi_notify_reset(notify_seg, id_received, &val);
        if(val != notify_value || id_received != notify_id)
        {
            gaspi_printf("Get wrong notification\n");
        }
        int * g2_iptr = (int *) ((char *) g2_ptr + g2.addr_or_offs.offset);
        gaspi_printf("Received value %d\n", *g2_iptr);
    }

    CHECK(dart_barrier(DART_TEAM_ALL));
    CHECK(dart_team_memfree(DART_TEAM_ALL, g2));

    CHECK(dart_exit());
    return 0;
}
