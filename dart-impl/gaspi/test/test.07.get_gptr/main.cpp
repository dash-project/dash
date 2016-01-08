#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include <dash/dart/if/dart.h>
#include <assert.h>
#include <GASPI.h>


#include <test.h>
#include <gtest/gtest.h>

const size_t number_of_gets     = 8;
const int    transfer_val_count = 128;
const int    transfer_val_begin = 42;


TEST(Get, More_Targets)
{
    dart_unit_t myid;
    size_t size;
    dart_gptr_t gptr_team;
    dart_gptr_t gptr_priv;
    dart_gptr_t gptr_priv_prev;

    TEST_DART_CALL(dart_myid(&myid));
    TEST_DART_CALL(dart_size(&size));

    const dart_unit_t next_unit = (myid + 1 + size) % size;
    const dart_unit_t prev_unit = (myid - 1 + size) % size;

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));


    TEST_DART_CALL(dart_team_memalloc_aligned(DART_TEAM_ALL, transfer_val_count * sizeof(int), &gptr_team));
    TEST_DART_CALL(dart_team_memalloc_aligned(DART_TEAM_ALL, transfer_val_count * sizeof(int), &gptr_priv));
    TEST_DART_CALL(dart_team_memalloc_aligned(DART_TEAM_ALL, transfer_val_count * sizeof(int), &gptr_priv_prev));

    TEST_DART_CALL(dart_gptr_setunit(&gptr_priv, myid));
    TEST_DART_CALL(dart_gptr_setunit(&gptr_priv_prev, myid));

    void * g_ptr = nullptr;

    TEST_DART_CALL(dart_gptr_setunit(&gptr_team, myid));
    TEST_DART_CALL(dart_gptr_getaddr(gptr_team, &g_ptr));

    for(size_t i = 0 ; i < transfer_val_count ; ++i)
    {
        ((int *) g_ptr)[i] = myid + i;
    }

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));

    dart_gptr_t gptr_team_prev = gptr_team;

    TEST_DART_CALL(dart_gptr_setunit(&gptr_team_prev, prev_unit));
    TEST_DART_CALL(dart_gptr_setunit(&gptr_team, next_unit));

    TEST_DART_CALL(dart_get_gptr(gptr_priv, gptr_team, transfer_val_count * sizeof(int)));
    TEST_DART_CALL(dart_get_gptr(gptr_priv_prev, gptr_team_prev, transfer_val_count * sizeof(int)));

    TEST_DART_CALL(dart_flush_local_all(gptr_team));

    TEST_DART_CALL(dart_gptr_getaddr(gptr_priv, &g_ptr));
    for(size_t i = 0 ; i < transfer_val_count ; ++i)
    {
        EXPECT_EQ(next_unit + i, ((int *) g_ptr)[i]);
    }

    TEST_DART_CALL(dart_gptr_getaddr(gptr_priv_prev, &g_ptr));
    for(size_t i = 0 ; i < transfer_val_count ; ++i)
    {
        EXPECT_EQ(prev_unit + i, ((int *) g_ptr)[i]);
    }

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
    TEST_DART_CALL(dart_team_memfree(DART_TEAM_ALL, gptr_team));
    TEST_DART_CALL(dart_team_memfree(DART_TEAM_ALL, gptr_priv));
    TEST_DART_CALL(dart_team_memfree(DART_TEAM_ALL, gptr_priv_prev));
    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
}

TEST(Get, Many_Gets)
{
    dart_gptr_t src_seg;
    dart_gptr_t dest_seg;

    dart_unit_t myid;
    size_t      team_size;

    TEST_DART_CALL(dart_myid(&myid));
    TEST_DART_CALL(dart_size(&team_size));

    dart_unit_t     next_unit = (myid + 1 + team_size) % team_size;

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));

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
        TEST_DART_CALL(dart_get_gptr(iter_gptr_dest, iter_gptr_src, size_per_get));

        TEST_DART_CALL(dart_gptr_incaddr(&iter_gptr_dest, size_per_get));
        TEST_DART_CALL(dart_gptr_incaddr(&iter_gptr_src , size_per_get));
    }

    TEST_DART_CALL(dart_flush_local(iter_gptr_src));
    void * recv_buffer = nullptr;
    TEST_DART_CALL(dart_gptr_getaddr(dest_seg, &recv_buffer));
    for(int i = 0; i < transfer_val_count; ++i)
    {
        EXPECT_EQ(next_unit + i, ((int *) recv_buffer)[i]);
    }

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));

    TEST_DART_CALL(dart_team_memfree(DART_TEAM_ALL, src_seg));
    TEST_DART_CALL(dart_team_memfree(DART_TEAM_ALL, dest_seg));
    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
}

TEST(Get, More_Segments)
{
    dart_unit_t myid;
    size_t size;
    dart_gptr_t gptr_team;
    dart_gptr_t gptr_team_2;
    dart_gptr_t gptr_priv;
    dart_gptr_t gptr_priv_prev;

    TEST_DART_CALL(dart_myid(&myid));
    TEST_DART_CALL(dart_size(&size));

    const dart_unit_t next_unit = (myid + 1 + size) % size;
    const dart_unit_t prev_unit = (myid - 1 + size) % size;

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));

    TEST_DART_CALL(dart_team_memalloc_aligned(DART_TEAM_ALL, transfer_val_count * sizeof(int), &gptr_priv));
    TEST_DART_CALL(dart_team_memalloc_aligned(DART_TEAM_ALL, transfer_val_count * sizeof(int), &gptr_priv_prev));
    TEST_DART_CALL(dart_team_memalloc_aligned(DART_TEAM_ALL, transfer_val_count * sizeof(int), &gptr_team));
    TEST_DART_CALL(dart_team_memalloc_aligned(DART_TEAM_ALL, transfer_val_count * sizeof(int), &gptr_team_2));

    TEST_DART_CALL(dart_gptr_setunit(&gptr_priv, myid));
    TEST_DART_CALL(dart_gptr_setunit(&gptr_priv_prev, myid));

    void * g_ptr   = nullptr;
    void * g_ptr_2 = nullptr;

    TEST_DART_CALL(dart_gptr_setunit(&gptr_team, myid));
    TEST_DART_CALL(dart_gptr_getaddr(gptr_team, &g_ptr));

    TEST_DART_CALL(dart_gptr_setunit(&gptr_team_2, myid));
    TEST_DART_CALL(dart_gptr_getaddr(gptr_team_2, &g_ptr_2));

    for(size_t i = 0 ; i < transfer_val_count ; ++i)
    {
        ((int *) g_ptr)[i]   = myid + i;
        ((int *) g_ptr_2)[i] = myid + i + 42;
    }

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));

    TEST_DART_CALL(dart_gptr_setunit(&gptr_team_2, prev_unit));
    TEST_DART_CALL(dart_gptr_setunit(&gptr_team, next_unit));

    TEST_DART_CALL(dart_get_gptr(gptr_priv, gptr_team, transfer_val_count * sizeof(int)));
    TEST_DART_CALL(dart_get_gptr(gptr_priv_prev, gptr_team_2, transfer_val_count * sizeof(int)));


    TEST_DART_CALL(dart_flush_local(gptr_team));

    TEST_DART_CALL(dart_gptr_getaddr(gptr_priv, &g_ptr));
    for(size_t i = 0 ; i < transfer_val_count ; ++i)
    {
        EXPECT_EQ(next_unit + i, ((int *) g_ptr)[i]);
    }

    TEST_DART_CALL(dart_flush_local(gptr_team_2));
    TEST_DART_CALL(dart_gptr_getaddr(gptr_priv_prev, &g_ptr));
    for(size_t i = 0 ; i < transfer_val_count ; ++i)
    {
        EXPECT_EQ(prev_unit + i + 42, ((int *) g_ptr)[i]);
    }

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
    TEST_DART_CALL(dart_team_memfree(DART_TEAM_ALL, gptr_team));
    TEST_DART_CALL(dart_team_memfree(DART_TEAM_ALL, gptr_team_2));
    TEST_DART_CALL(dart_team_memfree(DART_TEAM_ALL, gptr_priv));
    TEST_DART_CALL(dart_team_memfree(DART_TEAM_ALL, gptr_priv_prev));
    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
}

TEST(Get, same_segment)
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

    TEST_DART_CALL(dart_get_gptr(g_dest, g_src, transfer_val_count * sizeof(int)));
    TEST_DART_CALL(dart_flush_local(g_src));

    TEST_DART_CALL(dart_gptr_getaddr(g_dest, &g_ptr));
    for(size_t i = 0 ; i < transfer_val_count ; ++i)
    {
        EXPECT_EQ(next_unit + i, ((int *) g_ptr)[i]);
    }

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
    TEST_DART_CALL(dart_team_memfree(DART_TEAM_ALL, g));
    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
}

TEST(Get, different_segment)
{
    dart_unit_t myid;
    size_t size;
    dart_gptr_t gptr_team;
    dart_gptr_t gptr_priv;

    TEST_DART_CALL(dart_myid(&myid));
    TEST_DART_CALL(dart_size(&size));

    const dart_unit_t next_unit = (myid + 1 + size) % size;

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));

    TEST_DART_CALL(dart_team_memalloc_aligned(DART_TEAM_ALL, transfer_val_count * sizeof(int), &gptr_priv));
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
    TEST_DART_CALL(dart_get_gptr(gptr_priv, gptr_team, transfer_val_count * sizeof(int)));
    TEST_DART_CALL(dart_flush_local(gptr_team));

    TEST_DART_CALL(dart_gptr_setunit(&gptr_priv, myid));
    TEST_DART_CALL(dart_gptr_getaddr(gptr_priv, &g_ptr));
    for(size_t i = 0 ; i < transfer_val_count ; ++i)
    {
        EXPECT_EQ(next_unit + i, ((int *) g_ptr)[i]);
    }
    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
    TEST_DART_CALL(dart_team_memfree(DART_TEAM_ALL, gptr_team));
    TEST_DART_CALL(dart_team_memfree(DART_TEAM_ALL, gptr_priv));
    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
}

TEST(Get, local_access)
{
    dart_unit_t myid;
    size_t size;
    dart_gptr_t gptr_team;
    dart_gptr_t gptr_priv;

    TEST_DART_CALL(dart_myid(&myid));
    TEST_DART_CALL(dart_size(&size));

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
    TEST_DART_CALL(dart_team_memalloc_aligned(DART_TEAM_ALL, transfer_val_count * sizeof(int), &gptr_priv));
    TEST_DART_CALL(dart_team_memalloc_aligned(DART_TEAM_ALL, transfer_val_count * sizeof(int), &gptr_team));

    void * g_src_ptr = nullptr;
    void * g_dest_ptr = nullptr;


    TEST_DART_CALL(dart_gptr_setunit(&gptr_team, myid));
    TEST_DART_CALL(dart_gptr_setunit(&gptr_priv, myid));

    TEST_DART_CALL(dart_gptr_getaddr(gptr_team, &g_src_ptr));

    for(size_t i = 0 ; i < transfer_val_count ; ++i)
    {
        ((int *) g_src_ptr)[i] = transfer_val_begin + i;
    }

    TEST_DART_CALL(dart_gptr_getaddr(gptr_priv, &g_dest_ptr));

    TEST_DART_CALL(dart_get_gptr(gptr_priv, gptr_team, transfer_val_count * sizeof(int)));

    TEST_DART_CALL(dart_flush_local(gptr_team));

    for(size_t i = 0 ; i < transfer_val_count ; ++i)
    {
        EXPECT_EQ((int) transfer_val_begin + i, ((int *) g_dest_ptr)[i]);
    }

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
    TEST_DART_CALL(dart_team_memfree(DART_TEAM_ALL, gptr_team));
    TEST_DART_CALL(dart_team_memfree(DART_TEAM_ALL, gptr_priv));
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
