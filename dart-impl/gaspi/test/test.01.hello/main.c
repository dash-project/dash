#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include <test.h>
#include <dart.h>
#include <assert.h>

int main(int argc, char* argv[])
{
    char buf[80];
    pid_t pid;
    dart_unit_t myid;
    size_t size;

    CHECK(dart_init(&argc, &argv));
    CHECK(dart_myid(&myid));
    CHECK(dart_size(&size));

    gethostname(buf, 80);
    pid = getpid();
    fprintf(stderr,"Hello World, I'm unit %d of %d, pid=%d host=%s\n", myid, size, pid, buf);

    size_t gsize;
    CHECK(dart_group_sizeof(&gsize));
    dart_group_t *all = (dart_group_t *) malloc(gsize);
    assert(all);
    CHECK(dart_group_init(all));
    CHECK(dart_team_get_group(DART_TEAM_ALL, all));

    size_t all_size;
    CHECK(dart_group_size(all, &all_size));
    fprintf(stderr, "all size %d\n", all_size);
    dart_unit_t * all_units = (dart_unit_t *) malloc(sizeof(dart_unit_t) * all_size);
    assert(all_units);

    CHECK(dart_group_getmembers(all, all_units));

    if(myid == 0)
    {
        for(int i = 0 ; i < all_size ; ++i)
        {
            fprintf(stderr, "all group member %d\n", all_units[i]);
        }
    }
    free(all_units);
    CHECK(dart_group_fini(all));
    free(all);
    CHECK(dart_exit());
    //~ free(all_units);
    return 0;
}
