#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include <test.h>
#include <dart.h>
#include <assert.h>

#include <GASPI.h>

int main(int argc, char* argv[])
{
    dart_gptr_t g1;
    dart_unit_t myid;
    size_t size;

    CHECK(dart_init(&argc, &argv));
    CHECK(dart_myid(&myid));
    CHECK(dart_size(&size));

    CHECK(dart_team_memalloc_aligned(DART_TEAM_ALL, 1024, &g1));

    gaspi_printf("own seg id %d unitid %d\n", g1.segid, g1.unitid);

    if(myid == g1.unitid)
    {
        void * ptr = NULL;
        CHECK(dart_gptr_getaddr(g1, &ptr));
        if(ptr !=  NULL)
        {
            int * iptr = (int *) ptr;
            *iptr = 42;
            gaspi_pointer_t seg_ptr = NULL;
            gaspi_segment_ptr(30, &seg_ptr);
            if(seg_ptr != NULL)
            {
                if(*((int *) seg_ptr) != *iptr)
                {
                    fprintf(stderr, "Error: Wrong value in segment\n");
                }
            }
            else
            {
                fprintf(stderr, "Error: Address of global pointer is invalid\n");
            }
            CHECK(dart_gptr_setaddr(&g1, &(iptr[1])));
            if(g1.addr_or_offs.offset != sizeof(int))
            {
                fprintf(stderr, "Error: Wrong offset: dart_gptr_setaddr failed\n");
            }
        }
    }

    CHECK(dart_team_memfree(DART_TEAM_ALL, g1));

    CHECK(dart_exit());
    return 0;
}
