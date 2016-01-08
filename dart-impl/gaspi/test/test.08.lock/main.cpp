#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include <dash/dart/if/dart.h>
#include <assert.h>
#include <GASPI.h>


#include <test.h>
#include <gtest/gtest.h>

TEST(Lock, init_free)
{
    dart_unit_t myid;
    size_t size;
    TEST_DART_CALL(dart_myid(&myid));
    TEST_DART_CALL(dart_size(&size));
    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));

    dart_lock_t lock;
    TEST_DART_CALL(dart_team_lock_init(DART_TEAM_ALL, &lock));

    TEST_DART_CALL(dart_team_lock_free(DART_TEAM_ALL, &lock));

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
}

TEST(Lock, try_acquire)
{
    dart_unit_t myid;
    TEST_DART_CALL(dart_myid(&myid));

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));

    dart_lock_t lock;
    TEST_DART_CALL(dart_team_lock_init(DART_TEAM_ALL, &lock));

    int32_t result = 0;

    do{
        TEST_DART_CALL(dart_lock_try_acquire(lock, &result));
    }while(result == 0);
    gaspi_printf("Enter critical section\n");

    sleep(1);

    gaspi_printf("Leave critical section\n");
    TEST_DART_CALL(dart_lock_release(lock));

    TEST_DART_CALL(dart_team_lock_free(DART_TEAM_ALL, &lock));

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
}

TEST(Lock, acquire)
{
    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));

    dart_lock_t lock;
    TEST_DART_CALL(dart_team_lock_init(DART_TEAM_ALL, &lock));

    TEST_DART_CALL(dart_lock_acquire(lock));

    gaspi_printf("Enter critical section\n");

    sleep(1);

    gaspi_printf("Leave critical section\n");

    TEST_DART_CALL(dart_lock_release(lock));

    TEST_DART_CALL(dart_team_lock_free(DART_TEAM_ALL, &lock));

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
}

TEST(Lock, more_locks)
{
    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));

    dart_lock_t lock_outer;
    dart_lock_t lock_inner;

    TEST_DART_CALL(dart_team_lock_init(DART_TEAM_ALL, &lock_outer));
    TEST_DART_CALL(dart_team_lock_init(DART_TEAM_ALL, &lock_inner));

    TEST_DART_CALL(dart_lock_acquire(lock_outer));

    gaspi_printf("Enter outer critical section\n");

    TEST_DART_CALL(dart_lock_acquire(lock_inner));

    gaspi_printf("Enter inner critical section\n");
    sleep(1);
    gaspi_printf("Leave inner critical section\n");

    TEST_DART_CALL(dart_lock_release(lock_inner));

    gaspi_printf("Leave outer critical section\n");

    TEST_DART_CALL(dart_lock_release(lock_outer));

    TEST_DART_CALL(dart_team_lock_free(DART_TEAM_ALL, &lock_outer));
    TEST_DART_CALL(dart_team_lock_free(DART_TEAM_ALL, &lock_inner));

    TEST_DART_CALL(dart_barrier(DART_TEAM_ALL));
}

TEST(Lock, teams)
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

    dart_lock_t lock;
    TEST_DART_CALL(dart_team_lock_init(new_team, &lock));

    TEST_DART_CALL(dart_lock_acquire(lock));

    gaspi_printf("Enter critical section\n");

    sleep(1);

    gaspi_printf("Leave critical section\n");

    TEST_DART_CALL(dart_lock_release(lock));

    TEST_DART_CALL(dart_team_lock_free(new_team, &lock));


    TEST_DART_CALL(dart_barrier(new_team));
    TEST_DART_CALL(dart_team_destroy(new_team));

    TEST_DART_CALL(dart_group_fini(g));
    free(g);

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
