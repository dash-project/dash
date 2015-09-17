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

    dart_team_memalloc_aligned(DART_TEAM_ALL, 1024, &g1);

    gaspi_printf("own seg id %d unitid %d\n", g1.segid, g1.unitid);

    dart_team_memfree(DART_TEAM_ALL, g1);

    dart_exit();
    return 0;
}
