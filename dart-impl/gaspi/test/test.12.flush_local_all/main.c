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
    ((int *) ptr)[0] = 42;
    ((int *) ptr)[1] = 43;
    ((int *) ptr)[2] = 44;
    ((int *) ptr)[3] = 45;

    dart_gptr_t g3 = g1;

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
        int recv[4] = {0, 0, 0, 0};
        CHECK(dart_gptr_setunit(&g1, myid + 1));
        CHECK(dart_get(&(recv[0]), g1, sizeof(int)));
      	CHECK(dart_gptr_incaddr(&g1, sizeof(int)));
        CHECK(dart_get(&(recv[1]), g1, sizeof(int)));
      	CHECK(dart_gptr_incaddr(&g1, sizeof(int)));
        CHECK(dart_get(&(recv[2]), g1, sizeof(int)));
        CHECK(dart_gptr_incaddr(&g1, sizeof(int)));
        CHECK(dart_get(&(recv[3]), g1, sizeof(int)));
      	CHECK(dart_flush_local_all(g1));
        gaspi_printf("received value %d\n",recv[0]);
      	gaspi_printf("received value %d\n",recv[1]);
      	gaspi_printf("received value %d\n",recv[2]);
        gaspi_printf("received value %d\n",recv[3]);
    }

    CHECK(dart_barrier(DART_TEAM_ALL));
    CHECK(dart_memfree(g3));

    dart_gptr_t g2;
    CHECK(dart_team_memalloc_aligned(DART_TEAM_ALL, 1024, &g2));
    dart_gptr_t g4 = g2;
    if(myid == 1)
    {
        void * g2_ptr = NULL;

        CHECK(dart_gptr_setunit(&g2, myid));

        CHECK(dart_gptr_getaddr(g2, &g2_ptr));

        ((int *) g2_ptr)[0] = 1337;
        ((int *) g2_ptr)[1] = 1338;
        ((int *) g2_ptr)[2] = 1339;
        ((int *) g2_ptr)[3] = 1340;
    }

    CHECK(dart_barrier(DART_TEAM_ALL));

    if(myid == 0)
    {
        int recv[4] = {0, 0, 0, 0};
        CHECK(dart_gptr_setunit(&g2, myid + 1));
        CHECK(dart_get(&(recv[0]), g2, sizeof(int)));

        CHECK(dart_gptr_incaddr(&g2, sizeof(int)));
        CHECK(dart_get(&(recv[1]), g2, sizeof(int)));

        CHECK(dart_gptr_incaddr(&g2, sizeof(int)));
        CHECK(dart_get(&(recv[2]), g2, sizeof(int)));

        CHECK(dart_gptr_incaddr(&g2, sizeof(int)));
        CHECK(dart_get(&(recv[3]), g2, sizeof(int)));

        CHECK(dart_flush_local_all(g2));
        gaspi_printf("received value %d\n",recv[0]);
        gaspi_printf("received value %d\n",recv[1]);
        gaspi_printf("received value %d\n",recv[2]);
        gaspi_printf("received value %d\n",recv[3]);
    }

    CHECK(dart_barrier(DART_TEAM_ALL));

    CHECK(dart_team_memfree(DART_TEAM_ALL, g4));
    CHECK(dart_exit());
    return 0;
}
