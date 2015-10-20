#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include <dash/dart/if/dart.h>
#include <assert.h>
#include <GASPI.h>


#include <test.h>
#include <dart_translation.h>

void test_global_ptr(gaspi_segment_id_t seg, dart_gptr_t * gptr)
{
    void * ptr = NULL;
    CHECK(dart_gptr_getaddr(*gptr, &ptr));
    if(ptr !=  NULL)
    {
        int * iptr = (int *) ptr;
        *iptr = 42;
        gaspi_pointer_t seg_ptr = NULL;
        gaspi_segment_ptr(seg, &seg_ptr);
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
        CHECK(dart_gptr_setaddr(gptr, &(iptr[1])));
        if(gptr->addr_or_offs.offset != sizeof(int))
        {
            fprintf(stderr, "Error: Wrong offset: dart_gptr_setaddr failed\n");
        }
    }
}

void test_create_group(dart_group_t * g, dart_unit_t myid, size_t size)
{
    CHECK(dart_group_init(g));

    if(myid < (size / 2))
    {
        for(dart_unit_t i = 0 ; i < (size / 2) ; ++i)
        {
            CHECK(dart_group_addmember(g, i));
        }
    }
    else
    {
        for(dart_unit_t i = (size / 2) ; i < size ; ++i)
        {
            CHECK(dart_group_addmember(g, i));
        }
    }
}

int main(int argc, char* argv[])
{
    dart_gptr_t g1;
    dart_unit_t myid;
    size_t size;
    gaspi_segment_id_t seg_begin;

    CHECK(dart_init(&argc, &argv));
    CHECK(dart_myid(&myid));
    CHECK(dart_size(&size));

    CHECK(dart_team_memalloc_aligned(DART_TEAM_ALL, 1024, &g1));

    gaspi_printf("own seg id %d unitid %d\n", g1.segid, g1.unitid);

    if(myid == g1.unitid)
    {
        dart_adapt_transtable_get_local_gaspi_seg_id(g1.segid, &seg_begin);
        test_global_ptr(seg_begin, &g1);
    }
    dart_gptr_t g2;
    CHECK(dart_team_memalloc_aligned(DART_TEAM_ALL, 1024, &g2));

    gaspi_printf("own seg id %d unitid %d\n", g2.segid, g2.unitid);

    if(myid == g2.unitid)
    {
        dart_adapt_transtable_get_local_gaspi_seg_id(g2.segid, &seg_begin);
        test_global_ptr(seg_begin, &g2);
    }

    CHECK(dart_team_memfree(DART_TEAM_ALL, g2));
    CHECK(dart_team_memfree(DART_TEAM_ALL, g1));

    size_t gsize;
    dart_team_t new_team;
    dart_gptr_t team_gptr;
    CHECK(dart_group_sizeof(&gsize));
    dart_group_t * g = (dart_group_t *) malloc(gsize);
    assert(g);

    test_create_group(g, myid, size);

    CHECK(dart_team_create(DART_TEAM_ALL, g, &new_team));

    CHECK(dart_team_memalloc_aligned(new_team, 1024, &team_gptr));

    if(myid == team_gptr.unitid)
    {
        dart_adapt_transtable_get_local_gaspi_seg_id(team_gptr.segid, &seg_begin);
        test_global_ptr(seg_begin, &team_gptr);
    }

    CHECK(dart_team_memfree(new_team, team_gptr));
    CHECK(dart_team_destroy(new_team));
    CHECK(dart_group_fini(g));
    free(g);
    CHECK(dart_exit());
    return 0;
}
