#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include <dash/dart/if/dart.h>
#include <assert.h>
#include <GASPI.h>
#include <test.h>
#include <gtest/gtest.h>

TEST(Gather, Element)
{
    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));

    dart_unit_t root_unit = 0;
    dart_unit_t myid;
    size_t size;
    TEST_DART_CALL(dart_myid(&myid));
    TEST_DART_CALL(dart_size(&size));

    int * recv_buffer = (int *) malloc(sizeof(int) * size);
    assert(recv_buffer);

    int send_buffer = myid + 42;

    TEST_DART_CALL(dart_gather(&send_buffer, recv_buffer, sizeof(int), root_unit, DART_TEAM_ALL));

    if(myid == root_unit)
    {
        for(size_t i = 0; i < size; ++i)
        {
            EXPECT_EQ(i + 42, recv_buffer[i]);
        }
    }
    free(recv_buffer);

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
}

TEST(Gather, Array)
{
    dart_unit_t root_unit = 0;
    dart_unit_t myid;
    size_t size;
    const size_t count = 4;
    TEST_DART_CALL(dart_myid(&myid));
    TEST_DART_CALL(dart_size(&size));
    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));

    int * recv_buffer = (int *) malloc(sizeof(int) * count * size);
    assert(recv_buffer);

    int * send_buffer = (int *) malloc(sizeof(int) * count);
    assert(send_buffer);

    for(size_t j = 0; j < count; ++j)
    {
        send_buffer[j] = myid + 42;
    }

    TEST_DART_CALL(dart_gather(send_buffer, recv_buffer, sizeof(int) * count, root_unit, DART_TEAM_ALL));

    if(myid == root_unit)
    {
        for(size_t j = 0; j < size; ++j)
        {
            for(size_t i = 0; i < count; ++i)
            {
                EXPECT_EQ(j + 42, recv_buffer[i + (j * count)]);
            }
        }
    }
    free(recv_buffer);
    free(send_buffer);
    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
}

TEST(Gather, Teams)
{
    const dart_unit_t root_unit = 0;
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
        for(size_t i = 0 ; i < (size / 2) ; ++i)
        {
            TEST_DART_CALL(dart_group_addmember(g, i));
        }
    }
    else
    {
        for(size_t i = (size / 2) ; i < size ; ++i)
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

    int * recv_buffer = (int *) malloc(sizeof(int) * team_size);
    assert(recv_buffer);

    const int value = (myid < (size / 2)) ? 42 : 1337;
    int send_buffer = value + team_unitid;

    TEST_DART_CALL(dart_gather(&send_buffer, recv_buffer, sizeof(int), root_unit, new_team));

    if(myid == root_unit)
    {
        for(size_t i = 0; i < team_size; ++i)
        {
            EXPECT_EQ(i + value, recv_buffer[i]);
        }
    }

    free(recv_buffer);

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
