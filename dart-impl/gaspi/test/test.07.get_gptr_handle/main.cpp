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

TEST(Get_Handle, different_segment)
{
    dart_unit_t myid;
    size_t size;
    dart_gptr_t gptr_team;
    dart_gptr_t gptr_priv;

    TEST_DART_CALL(dart_myid(&myid));
    TEST_DART_CALL(dart_size(&size));

    const dart_unit_t next_unit = (myid + 1 + size) % size;

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));

    TEST_DART_CALL(dart_memalloc(transfer_val_count * sizeof(int), &gptr_priv));
    TEST_DART_CALL(dart_team_memalloc_aligned(DART_TEAM_ALL, transfer_val_count * sizeof(int), &gptr_team));

    void * g_ptr = NULL;

    TEST_DART_CALL(dart_gptr_setunit(&gptr_team, myid));
    TEST_DART_CALL(dart_gptr_getaddr(gptr_team, &g_ptr));

    for(size_t i = 0 ; i < transfer_val_count ; ++i)
    {
        ((int *) g_ptr)[i] = myid + i;
    }

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));

    TEST_DART_CALL(dart_gptr_setunit(&gptr_team, next_unit));

    dart_handle_t handle;
    TEST_DART_CALL(dart_create_handle(&handle));

    TEST_DART_CALL(dart_get_gptr_handle(gptr_priv, gptr_team, transfer_val_count * sizeof(int), handle));
    TEST_DART_CALL(dart_wait_local(handle));

    TEST_DART_CALL(dart_gptr_getaddr(gptr_priv, &g_ptr));
    for(size_t i = 0 ; i < transfer_val_count ; ++i)
    {
        EXPECT_EQ(next_unit + i, ((int *) g_ptr)[i]);
    }

    TEST_DART_CALL(dart_delete_handle(&handle));
    TEST_DART_CALL(dart_memfree(gptr_priv));
    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
    TEST_DART_CALL(dart_team_memfree(DART_TEAM_ALL, gptr_team));
}

TEST(Get_Handle, same_segment)
{
    dart_unit_t myid;
    size_t size;
    const int offset = transfer_val_count * sizeof(int);

    dart_gptr_t g;

    TEST_DART_CALL(dart_myid(&myid));
    TEST_DART_CALL(dart_size(&size));

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

    dart_gptr_t g_dest = g, g_src = g;

    TEST_DART_CALL(dart_gptr_incaddr(&g_dest, offset));
    TEST_DART_CALL(dart_gptr_setunit(&g_src, next_unit));

    dart_handle_t handle;
    TEST_DART_CALL(dart_create_handle(&handle));

    TEST_DART_CALL(dart_get_gptr_handle(g_dest, g_src, transfer_val_count * sizeof(int), handle));
    TEST_DART_CALL(dart_wait_local(handle));

    TEST_DART_CALL(dart_gptr_getaddr(g_dest, &g_ptr));
    for(size_t i = 0 ; i < transfer_val_count ; ++i)
    {
        EXPECT_EQ(next_unit + i, ((int *) g_ptr)[i]);
    }

    TEST_DART_CALL(dart_delete_handle(&handle));
    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
    TEST_DART_CALL(dart_team_memfree(DART_TEAM_ALL, g));
}

TEST(Get_Handle, local_access)
{
    dart_unit_t myid;
    size_t size;
    dart_gptr_t gptr_team;
    dart_gptr_t gptr_priv;

    TEST_DART_CALL(dart_myid(&myid));
    TEST_DART_CALL(dart_size(&size));

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));

    TEST_DART_CALL(dart_memalloc(transfer_val_count * sizeof(int), &gptr_priv));
    TEST_DART_CALL(dart_team_memalloc_aligned(DART_TEAM_ALL, transfer_val_count * sizeof(int), &gptr_team));

    void * g_src_ptr = NULL;
    void * g_dest_ptr = NULL;


    TEST_DART_CALL(dart_gptr_setunit(&gptr_team, myid));
    TEST_DART_CALL(dart_gptr_getaddr(gptr_team, &g_src_ptr));

    for(size_t i = 0 ; i < transfer_val_count ; ++i)
    {
        ((int *) g_src_ptr)[i] = transfer_val_begin + i;
    }

    TEST_DART_CALL(dart_gptr_getaddr(gptr_priv, &g_dest_ptr));

    dart_handle_t handle;

    TEST_DART_CALL(dart_create_handle(&handle));

    TEST_DART_CALL(dart_get_gptr_handle(gptr_priv, gptr_team, transfer_val_count * sizeof(int), handle));
    TEST_DART_CALL(dart_wait_local(handle));

    for(size_t i = 0 ; i < transfer_val_count ; ++i)
    {
        EXPECT_EQ((int) transfer_val_begin + i, ((int *) g_dest_ptr)[i]);
    }

    TEST_DART_CALL(dart_delete_handle(&handle));

    TEST_DART_CALL(dart_memfree(gptr_priv));
    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
    TEST_DART_CALL(dart_team_memfree(DART_TEAM_ALL, gptr_team));
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
