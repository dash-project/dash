#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include <dash/dart/if/dart.h>
#include <assert.h>
#include <GASPI.h>


#include "../test.h"

TEST(Team, Create)
{
    dart_unit_t myid;
    size_t size;
    size_t gsize;
    dart_group_t * g;

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
    TEST_DART_CALL(dart_myid(&myid));
    TEST_DART_CALL(dart_size(&size));

    TEST_DART_CALL(dart_group_sizeof(&gsize));

    g = (dart_group_t *) malloc(gsize);
    ASSERT_TRUE(g);

    TEST_DART_CALL(dart_group_init(g));

    for(dart_unit_t i = 0 ; i < size ; ++i)
    {
        TEST_DART_CALL(dart_group_addmember(g, i));
    }

    dart_team_t new_team = DART_TEAM_NULL;
    TEST_DART_CALL(dart_team_create(DART_TEAM_ALL, g, &new_team));
    ASSERT_TRUE(new_team != DART_TEAM_NULL);

    dart_unit_t rel_unit_id;
    TEST_DART_CALL(dart_team_myid(new_team, &rel_unit_id));
    ASSERT_TRUE(rel_unit_id == myid);


    size_t team_size;
    TEST_DART_CALL(dart_team_size(new_team, &team_size));
    ASSERT_TRUE(team_size == size);

    dart_unit_t gid;
    TEST_DART_CALL(dart_team_unit_l2g(new_team, rel_unit_id, &gid));
    ASSERT_TRUE(gid == myid);

    TEST_DART_CALL(dart_barrier(new_team));
    TEST_DART_CALL(dart_team_destroy(new_team));

    TEST_DART_CALL(dart_group_fini(g));
    free(g);
    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
}

TEST(Team, Create_HalfAll)
{
    dart_unit_t myid;
    size_t size;
    size_t gsize;
    dart_group_t * g;

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
    TEST_DART_CALL(dart_myid(&myid));
    TEST_DART_CALL(dart_size(&size));

    TEST_DART_CALL(dart_group_sizeof(&gsize));

    g = (dart_group_t *) malloc(gsize);
    ASSERT_TRUE(g);

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
    dart_team_t new_team = DART_TEAM_NULL;
    TEST_DART_CALL(dart_team_create(DART_TEAM_ALL, g, &new_team));
    ASSERT_TRUE(new_team != DART_TEAM_NULL);
    dart_unit_t rel_unit_id;
    TEST_DART_CALL(dart_team_myid(new_team, &rel_unit_id));
    size_t team_size;
    TEST_DART_CALL(dart_team_size(new_team, &team_size));
    dart_unit_t gid;
    TEST_DART_CALL(dart_team_unit_l2g(new_team, rel_unit_id, &gid));
    TEST_DART_CALL(dart_barrier(new_team));
    TEST_DART_CALL(dart_team_destroy(new_team));

    TEST_DART_CALL(dart_group_fini(g));
    free(g);
    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);

    //~ // Gets hold of the event listener list.
    ::testing::TestEventListeners& listeners = ::testing::UnitTest::GetInstance()->listeners();
    // Delete default event listener
    delete listeners.Release(listeners.default_result_printer());
    // Append own listener
    listeners.Append(new GaspiPrinter);

    dart_init(&argc, &argv);
    int ret = RUN_ALL_TESTS();
    dart_exit();

    return ret;
}
