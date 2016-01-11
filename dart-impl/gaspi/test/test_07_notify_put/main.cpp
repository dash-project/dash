#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include <dash/dart/if/dart.h>
#include <assert.h>
#include <GASPI.h>


#include "../test.h"

TEST(Notify, Notify_Next)
{
    dart_unit_t myid;
    size_t size;
    dart_gptr_t gptr_team;

    TEST_DART_CALL(dart_myid(&myid));
    TEST_DART_CALL(dart_size(&size));

    const dart_unit_t next_unit = (myid + 1 + size) % size;
    const dart_unit_t prev_unit = (myid - 1 + size) % size;

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));

    TEST_DART_CALL(dart_team_memalloc_aligned(DART_TEAM_ALL, 1024, &gptr_team));

    dart_gptr_t gptr_next = gptr_team;
    TEST_DART_CALL(dart_gptr_setunit(&gptr_next, next_unit));

    unsigned int send_tag = 42 + myid;
    TEST_DART_CALL(dart_notify(gptr_next, send_tag));

    unsigned int tag = 1337;
    TEST_DART_CALL(dart_notify_waitsome(gptr_team, &tag));

    ASSERT_TRUE(tag == ((unsigned int) prev_unit + 42));

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
    TEST_DART_CALL(dart_team_memfree(DART_TEAM_ALL, gptr_team));
    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
}

TEST(Notify, Put)
{
    dart_unit_t myid;
    size_t size;
    dart_gptr_t gptr_a, gptr_b;
    const size_t transfer_count = 2;

    TEST_DART_CALL(dart_myid(&myid));
    TEST_DART_CALL(dart_size(&size));

    const dart_unit_t next_unit = (myid + 1 + size) % size;
    const dart_unit_t prev_unit = (myid - 1 + size) % size;

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));

    TEST_DART_CALL(dart_team_memalloc_aligned(DART_TEAM_ALL, transfer_count * sizeof(int), &gptr_a));
    TEST_DART_CALL(dart_team_memalloc_aligned(DART_TEAM_ALL, transfer_count * sizeof(int), &gptr_b));

    dart_gptr_t my_gptr_a = gptr_a;
    TEST_DART_CALL(dart_gptr_setunit(&my_gptr_a, myid));

    void * my_ptr = NULL;
    TEST_DART_CALL(dart_gptr_getaddr(my_gptr_a, &my_ptr));

    for(size_t i = 0; i < transfer_count; ++i)
    {
        ((int *) my_ptr)[i] = myid + i;
    }
    dart_gptr_t gptr_dest = gptr_b;
    TEST_DART_CALL(dart_gptr_setunit(&gptr_dest, next_unit));

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
    TEST_DART_CALL(dart_put_gptr(gptr_dest, my_gptr_a, sizeof(int) * transfer_count));
    TEST_DART_CALL(dart_notify(gptr_dest, 42 + myid));
    unsigned int tag = 0;
    TEST_DART_CALL(dart_notify_waitsome(gptr_dest, &tag));
    EXPECT_EQ(42 + prev_unit, tag);

    dart_gptr_t gptr_my_dest = gptr_b;
    TEST_DART_CALL(dart_gptr_setunit(&gptr_my_dest, myid));
    TEST_DART_CALL(dart_gptr_getaddr(gptr_my_dest, &my_ptr));

    for(size_t i = 0; i < transfer_count; ++i)
    {
        EXPECT_EQ(i + prev_unit, ((int *) my_ptr)[i]);
    }

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
    TEST_DART_CALL(dart_team_memfree(DART_TEAM_ALL, gptr_a));
    TEST_DART_CALL(dart_team_memfree(DART_TEAM_ALL, gptr_b));
    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
}

TEST(Notify, PutMoreTargets)
{
    dart_unit_t myid;
    size_t size;
    dart_gptr_t gptr_a, gptr_b, gptr_c;
    const size_t transfer_count = 2;

    TEST_DART_CALL(dart_myid(&myid));
    TEST_DART_CALL(dart_size(&size));

    const dart_unit_t next_unit = (myid + 1 + size) % size;
    const dart_unit_t prev_unit = (myid - 1 + size) % size;

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));

    TEST_DART_CALL(dart_team_memalloc_aligned(DART_TEAM_ALL, transfer_count * sizeof(int), &gptr_a));
    TEST_DART_CALL(dart_team_memalloc_aligned(DART_TEAM_ALL, transfer_count * sizeof(int), &gptr_b));
    TEST_DART_CALL(dart_team_memalloc_aligned(DART_TEAM_ALL, transfer_count * sizeof(int), &gptr_c));

    dart_gptr_t my_gptr_a = gptr_a;
    TEST_DART_CALL(dart_gptr_setunit(&my_gptr_a, myid));

    void * my_ptr = NULL;
    TEST_DART_CALL(dart_gptr_getaddr(my_gptr_a, &my_ptr));

    for(size_t i = 0; i < transfer_count; ++i)
    {
        ((int *) my_ptr)[i] = myid + i;
    }
    dart_gptr_t gptr_dest_next = gptr_b;
    TEST_DART_CALL(dart_gptr_setunit(&gptr_dest_next, next_unit));

    dart_gptr_t gptr_dest_prev = gptr_c;
    TEST_DART_CALL(dart_gptr_setunit(&gptr_dest_prev, prev_unit));

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));

    TEST_DART_CALL(dart_put_gptr(gptr_dest_next, my_gptr_a, sizeof(int) * transfer_count));
    TEST_DART_CALL(dart_notify(gptr_dest_next, 42 + myid));

    TEST_DART_CALL(dart_put_gptr(gptr_dest_prev, my_gptr_a, sizeof(int) * transfer_count));
    TEST_DART_CALL(dart_notify(gptr_dest_prev, 42 + myid));

    unsigned int tag = 0;
    TEST_DART_CALL(dart_notify_waitsome(gptr_dest_next, &tag));
    EXPECT_EQ(42 + prev_unit, tag);

    TEST_DART_CALL(dart_notify_waitsome(gptr_dest_prev, &tag));
    EXPECT_EQ(42 + next_unit, tag);

    dart_gptr_t gptr_my_dest_next = gptr_b;
    TEST_DART_CALL(dart_gptr_setunit(&gptr_my_dest_next, myid));
    TEST_DART_CALL(dart_gptr_getaddr(gptr_my_dest_next, &my_ptr));

    for(size_t i = 0; i < transfer_count; ++i)
    {
        EXPECT_EQ(i + prev_unit, ((int *) my_ptr)[i]);
    }

    dart_gptr_t gptr_my_dest_prev = gptr_c;
    TEST_DART_CALL(dart_gptr_setunit(&gptr_my_dest_prev, myid));
    TEST_DART_CALL(dart_gptr_getaddr(gptr_my_dest_prev, &my_ptr));

    for(size_t i = 0; i < transfer_count; ++i)
    {
        EXPECT_EQ(i + next_unit, ((int *) my_ptr)[i]);
    }

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
    TEST_DART_CALL(dart_team_memfree(DART_TEAM_ALL, gptr_a));
    TEST_DART_CALL(dart_team_memfree(DART_TEAM_ALL, gptr_b));
    TEST_DART_CALL(dart_team_memfree(DART_TEAM_ALL, gptr_c));
    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
}

TEST(Notify, MorePuts)
{
    dart_unit_t myid;
    size_t size;
    dart_gptr_t gptr_a, gptr_b;
    const size_t packet_count = 2;
    const size_t transfer_count = 16;

    TEST_DART_CALL(dart_myid(&myid));
    TEST_DART_CALL(dart_size(&size));

    const dart_unit_t next_unit = (myid + 1 + size) % size;
    const dart_unit_t prev_unit = (myid - 1 + size) % size;

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));

    TEST_DART_CALL(dart_team_memalloc_aligned(DART_TEAM_ALL, transfer_count * sizeof(int), &gptr_a));
    TEST_DART_CALL(dart_team_memalloc_aligned(DART_TEAM_ALL, transfer_count * sizeof(int), &gptr_b));

    dart_gptr_t my_gptr_a = gptr_a;
    TEST_DART_CALL(dart_gptr_setunit(&my_gptr_a, myid));

    void * my_ptr = NULL;
    TEST_DART_CALL(dart_gptr_getaddr(my_gptr_a, &my_ptr));

    for(size_t i = 0; i < transfer_count; ++i)
    {
        ((int *) my_ptr)[i] = myid + i;
    }
    dart_gptr_t gptr_dest = gptr_b;
    TEST_DART_CALL(dart_gptr_setunit(&gptr_dest, next_unit));

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));

    size_t number_of_puts = transfer_count / packet_count;
    dart_gptr_t src_iter = my_gptr_a;
    dart_gptr_t dest_iter = gptr_dest;

    for(size_t i = 0; i < number_of_puts; ++i)
    {
        TEST_DART_CALL(dart_put_gptr(dest_iter, src_iter, sizeof(int) * packet_count));
        TEST_DART_CALL(dart_gptr_incaddr(&src_iter , sizeof(int) * packet_count));
        TEST_DART_CALL(dart_gptr_incaddr(&dest_iter, sizeof(int) * packet_count));

    }
    TEST_DART_CALL(dart_notify(gptr_dest, 42 + myid));
    unsigned int tag = 0;
    TEST_DART_CALL(dart_notify_waitsome(gptr_dest, &tag));
    EXPECT_EQ(42 + prev_unit, tag);

    dart_gptr_t gptr_my_dest = gptr_b;
    TEST_DART_CALL(dart_gptr_setunit(&gptr_my_dest, myid));
    TEST_DART_CALL(dart_gptr_getaddr(gptr_my_dest, &my_ptr));

    for(size_t i = 0; i < transfer_count; ++i)
    {
        EXPECT_EQ(i + prev_unit, ((int *) my_ptr)[i]);
    }

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
    TEST_DART_CALL(dart_team_memfree(DART_TEAM_ALL, gptr_a));
    TEST_DART_CALL(dart_team_memfree(DART_TEAM_ALL, gptr_b));
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
