#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include <dash/dart/if/dart.h>
#include <assert.h>
#include <GASPI.h>
#include <test.h>
#include <gtest/gtest.h>

TEST(Allgather, Element)
{
    size_t size;
    dart_unit_t myid;

    TEST_DART_CALL(dart_myid(&myid));
    TEST_DART_CALL(dart_size(&size));

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));

    int * recv_buffer = (int *) malloc(sizeof(int) * size);
    assert(recv_buffer);

    int send_buffer = myid + 42;

    TEST_DART_CALL(dart_allgather(&send_buffer, recv_buffer, sizeof(int), DART_TEAM_ALL));

    for(size_t i = 0 ; i < size ; ++i)
    {
        EXPECT_EQ(i + 42, recv_buffer[i]);
    }

    free(recv_buffer);

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
}

TEST(Allgather, Array)
{
    size_t transfer_count = 4;
    dart_unit_t myid;
    size_t size;

    TEST_DART_CALL(dart_myid(&myid));
    TEST_DART_CALL(dart_size(&size));

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));

    int * send_buffer = (int *) malloc(sizeof(int) * transfer_count);
    assert(send_buffer);

    int * recv_buffer = (int *) malloc(sizeof(int) * transfer_count * size);
    assert(recv_buffer);

    for(int i = 0 ; i < transfer_count ; ++i)
    {
        send_buffer[i] = myid + 42;
    }

    TEST_DART_CALL(dart_allgather(send_buffer, recv_buffer, transfer_count * sizeof(int), DART_TEAM_ALL));

    for(size_t i = 0 ; i < size ; ++i)
    {
        for(size_t j = 0 ; j < transfer_count ; ++j)
        {
            EXPECT_EQ(i + 42, recv_buffer[(i * transfer_count) + j]);
        }
    }

    free(send_buffer);
    free(recv_buffer);

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
}

TEST(Allgather, Teams)
{
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
    int send_buffer = team_unitid + val_const;

    int * recv_buffer = (int *) malloc(sizeof(int) * team_size);
    assert(recv_buffer);

    TEST_DART_CALL(dart_allgather(&send_buffer, recv_buffer, sizeof(int), new_team));

    for(size_t i = 0; i < team_size; ++i)
    {
        EXPECT_EQ(val_const + i, recv_buffer[i]);
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
