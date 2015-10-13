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
    dart_lock_t lock;
    size_t size;


    CHECK(dart_init(&argc, &argv));
    CHECK(dart_myid(&myid));
    CHECK(dart_size(&size));

    CHECK(dart_team_lock_init(DART_TEAM_ALL, &lock));
    gaspi_printf("Hello Lock\n");
    CHECK(dart_team_lock_free(DART_TEAM_ALL, &lock));

    CHECK(dart_exit());
    return 0;
}
