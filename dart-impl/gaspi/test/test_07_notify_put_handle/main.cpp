#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include <dash/dart/if/dart.h>
#include <assert.h>
#include <GASPI.h>

#include "../test.h"

TEST(Notify_Handle, Put_Notify)
{
    dart_unit_t myid;
    size_t size;
    dart_gptr_t gptr_a, gptr_b;
    const size_t transfer_count = 2;
    dart_handle_t handle;

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
    TEST_DART_CALL(dart_create_handle(&handle));

    TEST_DART_CALL(dart_put_gptr_handle(gptr_dest, my_gptr_a, sizeof(int) * transfer_count, handle));

    TEST_DART_CALL(dart_notify_handle(handle, 42 + myid));

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

    TEST_DART_CALL(dart_delete_handle(&handle));

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
