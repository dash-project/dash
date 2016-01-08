#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include <dash/dart/if/dart.h>
#include <assert.h>
#include <GASPI.h>


#include <test.h>
#include <gtest/gtest.h>

const int transfer_val_count = 100;
const int transfer_val_begin = 42;

TEST(Get_Blocking, team_mem)
{
    dart_unit_t myid;
    size_t size;
    dart_gptr_t gptr_src;

    TEST_DART_CALL(dart_myid(&myid));
    TEST_DART_CALL(dart_size(&size));

    const dart_unit_t next_unit = (myid + 1 + size) % size;

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
    TEST_DART_CALL(dart_team_memalloc_aligned(DART_TEAM_ALL, transfer_val_count * sizeof(int), &gptr_src));

    void * src_ptr = NULL;
    TEST_DART_CALL(dart_gptr_setunit(&gptr_src, myid));
    TEST_DART_CALL(dart_gptr_getaddr(gptr_src, &src_ptr));

    for(size_t i = 0 ; i < transfer_val_count ; ++i)
    {
        ((int *) src_ptr)[i] = myid + i;
    }

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));

    TEST_DART_CALL(dart_gptr_setunit(&gptr_src, next_unit));

    int * recv_buffer = (int *) malloc(transfer_val_count * sizeof(int));
    assert(recv_buffer);

    TEST_DART_CALL(dart_get_blocking(recv_buffer, gptr_src, transfer_val_count * sizeof(int)));

    for(size_t i = 0 ; i < transfer_val_count ; ++i)
    {
        EXPECT_EQ(next_unit + i, recv_buffer[i]);
    }

    free(recv_buffer);
    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
    TEST_DART_CALL(dart_team_memfree(DART_TEAM_ALL, gptr_src));
}

TEST(Get_Blocking, local_access)
{
    dart_unit_t myid;
    size_t size;
    dart_gptr_t gptr_src;

    TEST_DART_CALL(dart_myid(&myid));
    TEST_DART_CALL(dart_size(&size));

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));

    TEST_DART_CALL(dart_team_memalloc_aligned(DART_TEAM_ALL, transfer_val_count * sizeof(int), &gptr_src));

    void * g_own_ptr  = NULL;

    dart_gptr_t gptr_own = gptr_src;

    TEST_DART_CALL(dart_gptr_setunit(&gptr_own, myid));
    TEST_DART_CALL(dart_gptr_getaddr(gptr_own, &g_own_ptr));
    for(size_t i = 0 ; i < transfer_val_count ; ++i)
    {
        ((int *) g_own_ptr)[i] = transfer_val_begin + i;
    }
    int * recv_buffer = (int *) malloc(sizeof(int) * transfer_val_count);
    assert(recv_buffer);

    TEST_DART_CALL(dart_get_blocking(recv_buffer, gptr_own, transfer_val_count * sizeof(int)));

    for(size_t i = 0 ; i < transfer_val_count ; ++i)
    {
        EXPECT_EQ((int) transfer_val_begin + i, recv_buffer[i]);
    }

    free(recv_buffer);
    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
    TEST_DART_CALL(dart_team_memfree(DART_TEAM_ALL, gptr_src));
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);

    // Gets hold of the event listener list.
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
