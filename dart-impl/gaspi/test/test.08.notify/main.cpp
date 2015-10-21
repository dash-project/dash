#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include <dash/dart/if/dart.h>
#include <assert.h>
#include <GASPI.h>


#include <test.h>
#include <gtest/gtest.h>

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
    TEST_DART_CALL(dart_notify_wait(gptr_team, &tag));

    ASSERT_TRUE(tag == ((unsigned int) prev_unit + 42));

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
    TEST_DART_CALL(dart_team_memfree(DART_TEAM_ALL, gptr_team));
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
