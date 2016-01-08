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

TEST(Get_Blocking, different_segment)
{
    dart_unit_t myid;
    size_t size;
    dart_gptr_t gptr_src;
    dart_gptr_t gptr_dest;

    TEST_DART_CALL(dart_myid(&myid));
    TEST_DART_CALL(dart_size(&size));

    const dart_unit_t next_unit = (myid + 1 + size) % size;

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));

    TEST_DART_CALL(dart_memalloc(transfer_val_count * sizeof(int), &gptr_dest));
    TEST_DART_CALL(dart_team_memalloc_aligned(DART_TEAM_ALL, transfer_val_count * sizeof(int), &gptr_src));

    void * g_ptr = NULL;
    dart_gptr_t gptr_own = gptr_src;
    TEST_DART_CALL(dart_gptr_setunit(&gptr_own, myid));
    TEST_DART_CALL(dart_gptr_getaddr(gptr_own, &g_ptr));

    for(size_t i = 0 ; i < transfer_val_count ; ++i)
    {
        ((int *) g_ptr)[i] = myid + i;
    }

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));

    TEST_DART_CALL(dart_gptr_setunit(&gptr_src, next_unit));

    TEST_DART_CALL(dart_get_gptr_blocking(gptr_dest, gptr_src, transfer_val_count * sizeof(int)));

    TEST_DART_CALL(dart_gptr_getaddr(gptr_dest, &g_ptr));
    for(size_t i = 0 ; i < transfer_val_count ; ++i)
    {
        EXPECT_EQ(next_unit + i, ((int *) g_ptr)[i]);
    }

    TEST_DART_CALL(dart_memfree(gptr_dest));
    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
    TEST_DART_CALL(dart_team_memfree(DART_TEAM_ALL, gptr_src));
}

TEST(Get_Blocking, same_segment)
{
    dart_unit_t myid;
    size_t size;
    const int offset = transfer_val_count * sizeof(int);

    dart_gptr_t g;

    TEST_DART_CALL(dart_myid(&myid));
    TEST_DART_CALL(dart_size(&size));

    const dart_unit_t prev_unit = (myid - 1 + size) % size;
    const dart_unit_t next_unit = (myid + 1 + size) % size;

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));

    TEST_DART_CALL(dart_team_memalloc_aligned(DART_TEAM_ALL, 2 * transfer_val_count * sizeof(int), &g));

    void * g_ptr = NULL;

    TEST_DART_CALL(dart_gptr_setunit(&g, myid));
    TEST_DART_CALL(dart_gptr_getaddr(g, &g_ptr));

    for(size_t i = 0 ; i < transfer_val_count ; ++i)
    {
        ((int *) g_ptr)[i] = myid + i;
    }

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));

    dart_gptr_t gptr_dest = g, gptr_src = g;

    TEST_DART_CALL(dart_gptr_incaddr(&gptr_dest, offset));
    TEST_DART_CALL(dart_gptr_setunit(&gptr_src, next_unit));

    TEST_DART_CALL(dart_get_gptr_blocking(gptr_dest, gptr_src, transfer_val_count * sizeof(int)));

    TEST_DART_CALL(dart_gptr_getaddr(gptr_dest, &g_ptr));
    for(size_t i = 0 ; i < transfer_val_count ; ++i)
    {
        EXPECT_EQ(next_unit + i, ((int *) g_ptr)[i]);
    }

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
    TEST_DART_CALL(dart_team_memfree(DART_TEAM_ALL, g));
}

TEST(Get_Blocking, local_access)
{
    dart_unit_t myid;
    size_t size;
    dart_gptr_t gptr_src;
    dart_gptr_t gptr_dest;

    TEST_DART_CALL(dart_myid(&myid));
    TEST_DART_CALL(dart_size(&size));

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));

    TEST_DART_CALL(dart_memalloc(transfer_val_count * sizeof(int), &gptr_dest));
    TEST_DART_CALL(dart_team_memalloc_aligned(DART_TEAM_ALL, transfer_val_count * sizeof(int), &gptr_src));

    void * g_own_ptr  = NULL;

    dart_gptr_t gptr_own = gptr_src;

    TEST_DART_CALL(dart_gptr_setunit(&gptr_own, myid));
    TEST_DART_CALL(dart_gptr_getaddr(gptr_own, &g_own_ptr));

    for(size_t i = 0 ; i < transfer_val_count ; ++i)
    {
        ((int *) g_own_ptr)[i] = transfer_val_begin + i;
    }

    TEST_DART_CALL(dart_get_gptr_blocking(gptr_dest, gptr_own, transfer_val_count * sizeof(int)));

    void * g_dest_ptr = NULL;

    TEST_DART_CALL(dart_gptr_getaddr(gptr_dest, &g_dest_ptr));

    for(size_t i = 0 ; i < transfer_val_count ; ++i)
    {
        EXPECT_EQ((int) transfer_val_begin + i, ((int *) g_dest_ptr)[i]);
    }

    TEST_DART_CALL(dart_memfree(gptr_dest));
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
