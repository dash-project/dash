#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include <test.h>
#include <dart.h>
#include <assert.h>

int main(int argc, char* argv[])
{
    dart_unit_t myid;
    size_t size;
    size_t gsize;
    dart_group_t * g;

    CHECK(dart_init(&argc, &argv));
    CHECK(dart_myid(&myid));
    CHECK(dart_size(&size));

    CHECK(dart_group_sizeof(&gsize));

    g = (dart_group_t *) malloc(gsize);
    assert(g);

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
    dart_team_t new_team = DART_TEAM_NULL;
    CHECK(dart_team_create(DART_TEAM_ALL, g, &new_team));
    dart_unit_t rel_unit_id;
    CHECK(dart_team_myid(new_team, &rel_unit_id));
    size_t team_size;
    CHECK(dart_team_size(new_team, &team_size));

    fprintf(stderr, "<%d> new team id %d, relative unit id %d -> size %d\n",myid, new_team, rel_unit_id,team_size);

    dart_unit_t gid;
    CHECK(dart_team_unit_l2g(new_team, rel_unit_id, &gid));

    fprintf(stderr, "global id %d -> local id %d\n", gid, rel_unit_id);
    CHECK(dart_barrier(new_team));
    CHECK(dart_team_destroy(new_team));

    CHECK(dart_group_fini(g));
    free(g);
    CHECK(dart_exit());
    return 0;
}
