#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include <dash/dart/if/dart.h>
#include <assert.h>
#include <GASPI.h>


#include <test.h>
#include <gtest/gtest.h>

const int    val_count = 128;
const size_t type_size = sizeof(int);

TEST(Memory, Team_Alloc)
{
    dart_unit_t myid;
    dart_gptr_t gptr_team1;
    dart_gptr_t gptr_team2;
    void * g_ptr1;
    void * g_ptr2;

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
    TEST_DART_CALL(dart_myid(&myid));

    TEST_DART_CALL(dart_team_memalloc_aligned(DART_TEAM_ALL, val_count * type_size, &gptr_team1));
    TEST_DART_CALL(dart_team_memalloc_aligned(DART_TEAM_ALL, val_count * type_size, &gptr_team2));
    TEST_DART_CALL(dart_gptr_setunit(&gptr_team1, myid));
    TEST_DART_CALL(dart_gptr_getaddr(gptr_team1, &g_ptr1));
    int * int_ptr1 = (int *) g_ptr1;
    for(int i = 0 ; i < val_count ; ++i)
    {
        int_ptr1[i] = 42 + i;
    }

    TEST_DART_CALL(dart_gptr_setunit(&gptr_team2, myid));
    TEST_DART_CALL(dart_gptr_getaddr(gptr_team2, &g_ptr2));
    int * int_ptr2 = (int *) g_ptr2;
    for(int i = 0 ; i < val_count ; ++i)
    {
        int_ptr2[i] = 1 + i;
    }
    for(int i = 0 ; i < val_count ; ++i)
    {
        EXPECT_EQ(1 + i, int_ptr2[i]);
    }
    for(int i = 0 ; i < val_count ; ++i)
    {
        EXPECT_EQ(42 + i, int_ptr1[i]);
    }
    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
    TEST_DART_CALL(dart_team_memfree(DART_TEAM_ALL, gptr_team1));
    TEST_DART_CALL(dart_team_memfree(DART_TEAM_ALL, gptr_team2));
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
