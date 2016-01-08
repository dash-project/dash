#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include <dash/dart/if/dart.h>
#include <assert.h>
#include <GASPI.h>


#include <test.h>
#include <gtest/gtest.h>

TEST(Bcast, Element)
{
    const dart_unit_t root_id = 0;
    dart_unit_t myid;
    size_t size;
    TEST_DART_CALL(dart_myid(&myid));
    TEST_DART_CALL(dart_size(&size));
    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));

    int send_buf = myid + 42;

    TEST_DART_CALL(dart_bcast(&send_buf, sizeof(int), root_id, DART_TEAM_ALL));

    EXPECT_EQ(root_id + 42, send_buf);

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
}

TEST(Bcast, Array)
{
    const int count = 4;
    const dart_unit_t root_id = 0;
    dart_unit_t myid;
    size_t size;
    TEST_DART_CALL(dart_myid(&myid));
    TEST_DART_CALL(dart_size(&size));
    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));

    int * buffer = (int *) malloc(sizeof(int) * count);
    assert(buffer);

    if(root_id == myid)
    {
        for(size_t i = 0; i < count; ++i)
        {
            buffer[i] = 42 + myid;
        }
    }

    TEST_DART_CALL(dart_bcast(buffer, sizeof(int) * count, root_id, DART_TEAM_ALL));

    for(size_t i = 0; i < count; ++i)
    {
        EXPECT_EQ(root_id + 42, buffer[i]);
    }

    free(buffer);
    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
}

TEST(Bcast, TeamsElement)
{
    const dart_unit_t root_id = 1;
    dart_unit_t myid;
    size_t size;

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));

    size_t gsize;
    dart_group_t * g;

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
    dart_unit_t team_unitid;
    size_t team_size;

    TEST_DART_CALL(dart_team_myid(new_team, &team_unitid));
    TEST_DART_CALL(dart_team_size(new_team, &team_size));

    const int val_const = (myid < (size / 2)) ? 42 : 1337;
    int send_buf = 0;

    if(team_unitid == root_id)
    {
        send_buf= root_id + val_const;
    }

    TEST_DART_CALL(dart_bcast(&send_buf, sizeof(int), root_id, new_team));

    EXPECT_EQ(root_id + val_const, send_buf);

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
