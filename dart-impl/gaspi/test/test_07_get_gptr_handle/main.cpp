#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include <dash/dart/if/dart.h>
#include <assert.h>
#include <GASPI.h>

#include "../test.h"

const size_t number_of_gets     = 8;
const int    transfer_val_count = 128;
const int    transfer_val_begin = 42;

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

    void * g_ptr = nullptr;

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
    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
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

    void * g_ptr = nullptr;

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
    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
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

    void * g_src_ptr = nullptr;
    void * g_dest_ptr = nullptr;


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
    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
}

TEST(Get_Handle, Many_Gets)
{
    dart_gptr_t src_seg;
    dart_gptr_t dest_seg;

    dart_unit_t myid;
    size_t      team_size;

    TEST_DART_CALL(dart_myid(&myid));
    TEST_DART_CALL(dart_size(&team_size));

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));

    dart_unit_t     next_unit = (myid + 1 + team_size) % team_size;
    dart_handle_t * handles   = new dart_handle_t[number_of_gets];
    ASSERT_TRUE(handles !=  nullptr);

    for(size_t i = 0; i < number_of_gets; ++i)
    {
        TEST_DART_CALL(dart_create_handle(&(handles[i])));
    }

    TEST_DART_CALL(dart_team_memalloc_aligned(DART_TEAM_ALL, transfer_val_count * sizeof(int), &src_seg));
    TEST_DART_CALL(dart_team_memalloc_aligned(DART_TEAM_ALL, transfer_val_count * sizeof(int), &dest_seg));

    void * own_seg_ptr  = nullptr;
    dart_gptr_t own_seg = src_seg;

    TEST_DART_CALL(dart_gptr_setunit(&own_seg, myid));
    TEST_DART_CALL(dart_gptr_getaddr(own_seg, &own_seg_ptr));

    for(int i = 0; i < transfer_val_count; ++i)
    {
        ((int *) own_seg_ptr)[i] = myid + i;
    }

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));

    TEST_DART_CALL(dart_gptr_setunit(&src_seg,  next_unit));
    TEST_DART_CALL(dart_gptr_setunit(&dest_seg, myid));

    dart_gptr_t iter_gptr_src  = src_seg;
    dart_gptr_t iter_gptr_dest = dest_seg;

    EXPECT_EQ(0, transfer_val_count % number_of_gets);

    size_t size_per_get = (transfer_val_count / number_of_gets) * sizeof(int);

    for(size_t i = 0; i < number_of_gets; ++i)
    {
        TEST_DART_CALL(dart_get_gptr_handle(iter_gptr_dest, iter_gptr_src, size_per_get, handles[i]));

        TEST_DART_CALL(dart_gptr_incaddr(&iter_gptr_dest, size_per_get));
        TEST_DART_CALL(dart_gptr_incaddr(&iter_gptr_src , size_per_get));
    }

    TEST_DART_CALL(dart_waitall_local(handles, number_of_gets));

    void * recv_buffer = nullptr;
    TEST_DART_CALL(dart_gptr_getaddr(dest_seg, &recv_buffer));
    for(int i = 0; i < transfer_val_count; ++i)
    {
        EXPECT_EQ(next_unit + i, ((int *) recv_buffer)[i]);
    }

    for(size_t i = 0; i < number_of_gets; ++i)
    {
        TEST_DART_CALL(dart_delete_handle(&(handles[i])));
    }

    delete handles;

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));

    TEST_DART_CALL(dart_team_memfree(DART_TEAM_ALL, src_seg));
    TEST_DART_CALL(dart_team_memfree(DART_TEAM_ALL, dest_seg));

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
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
