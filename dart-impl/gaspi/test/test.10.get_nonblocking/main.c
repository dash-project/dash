#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include <test.h>
#include <dart.h>
#include <assert.h>
#include <GASPI.h>

int main(int argc, char* argv[])
{
    dart_unit_t myid;
    dart_gptr_t g1;
    size_t size;
    void * ptr;

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

    if(myid == 0)
    {
        int recv = 0;
        CHECK(dart_gptr_setunit(&g1, myid + 1));
        CHECK(dart_get(&recv, g1, sizeof(int)));
        gaspi_printf("received value %d\n",recv);
        CHECK(dart_flush_local(g1));
        gaspi_printf("received value %d\n",recv);
    }

    CHECK(dart_barrier(DART_TEAM_ALL));
    CHECK(dart_memfree(g1));

    dart_gptr_t g2;
    CHECK(dart_team_memalloc_aligned(DART_TEAM_ALL, 1024, &g2));

    if(myid == 1)
    {
        void * g2_ptr = NULL;

        CHECK(dart_gptr_setunit(&g2, myid));

        CHECK(dart_gptr_getaddr(g2, &g2_ptr));

        *((int *) g2_ptr) = 1337;
    }

    CHECK(dart_barrier(DART_TEAM_ALL));

    if(myid == 0)
    {
        int recv = 0;
        CHECK(dart_gptr_setunit(&g2, myid + 1));
        CHECK(dart_get(&recv, g2, sizeof(int)));
        gaspi_printf("received value %d\n",recv);
        CHECK(dart_flush_local(g2));
        gaspi_printf("received value %d\n",recv);
    }

    CHECK(dart_barrier(DART_TEAM_ALL));

    CHECK(dart_team_memfree(DART_TEAM_ALL, g2));
    CHECK(dart_exit());
    return 0;
}
