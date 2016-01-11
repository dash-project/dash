#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include <dash/dart/if/dart.h>
#include <assert.h>
#include <GASPI.h>

#include "../test.h"

void test_create_group(dart_group_t * g, dart_unit_t myid, size_t size)
{
    TEST_DART_CALL(dart_group_init(g));

    if(myid < (size / 2))
    {
        for(dart_unit_t i = 0 ; i < (size / 2) ; ++i)
        {
            TEST_DART_CALL(dart_group_addmember(g, i));
        }
    }
    else
    {
        for(dart_unit_t i = (size / 2) ; i < size ; ++i)
        {
            TEST_DART_CALL(dart_group_addmember(g, i));
        }
    }
}

int main(int argc, char* argv[])
{
    dart_gptr_t g1;
    dart_unit_t myid;
    size_t size;
    gaspi_segment_id_t seg_begin;

    TEST_DART_CALL(dart_init(&argc, &argv));
    TEST_DART_CALL(dart_myid(&myid));
    TEST_DART_CALL(dart_size(&size));

    TEST_DART_CALL(dart_team_memalloc_aligned(DART_TEAM_ALL, 1024, &g1));

    gaspi_printf("own seg id %d unitid %d\n", g1.segid, g1.unitid);

    dart_gptr_t g2;
    TEST_DART_CALL(dart_team_memalloc_aligned(DART_TEAM_ALL, 1024, &g2));

    gaspi_printf("own seg id %d unitid %d\n", g2.segid, g2.unitid);

    TEST_DART_CALL(dart_team_memfree(DART_TEAM_ALL, g2));
    TEST_DART_CALL(dart_team_memfree(DART_TEAM_ALL, g1));

    size_t gsize;
    dart_team_t new_team;
    dart_gptr_t team_gptr;
    TEST_DART_CALL(dart_group_sizeof(&gsize));
    dart_group_t * g = (dart_group_t *) malloc(gsize);
    assert(g);

    test_create_group(g, myid, size);

    TEST_DART_CALL(dart_team_create(DART_TEAM_ALL, g, &new_team));

    TEST_DART_CALL(dart_team_memalloc_aligned(new_team, 1024, &team_gptr));

    TEST_DART_CALL(dart_team_memfree(new_team, team_gptr));
    TEST_DART_CALL(dart_team_destroy(new_team));
    TEST_DART_CALL(dart_group_fini(g));
    free(g);
    TEST_DART_CALL(dart_exit());
    return 0;
}
