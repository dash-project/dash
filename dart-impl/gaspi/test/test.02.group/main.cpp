#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include <dash/dart/if/dart.h>
#include <assert.h>
#include <GASPI.h>


#include <test.h>
#include <gtest/gtest.h>

TEST(Group, Create)
{
    dart_unit_t myid;
    size_t size;

    TEST_DART_CALL(dart_myid(&myid));
    TEST_DART_CALL(dart_size(&size));

    size_t gsize;
    TEST_DART_CALL(dart_group_sizeof(&gsize));

    dart_group_t *even_group = (dart_group_t *) malloc(gsize);
    assert(even_group);
    dart_group_t *odd_group  = (dart_group_t *) malloc(gsize);
    assert(odd_group);

    TEST_DART_CALL(dart_group_init(even_group));
    TEST_DART_CALL(dart_group_init(odd_group));

    for(size_t i = 0 ; i < size ;++i)
    {
        if(i % 2 == 0)
        {
            TEST_DART_CALL(dart_group_addmember(even_group, i));
        }
        else
        {
            TEST_DART_CALL(dart_group_addmember(odd_group, i));
        }
    }

    size_t odd_size, even_size;

    TEST_DART_CALL(dart_group_size(even_group, &even_size));
    TEST_DART_CALL(dart_group_size(odd_group, &odd_size));

    dart_unit_t * even_ids = (dart_unit_t *) malloc(sizeof(dart_unit_t) * even_size);
    ASSERT_TRUE(even_ids);

    dart_unit_t * odd_ids =  (dart_unit_t *) malloc(sizeof(dart_unit_t) * odd_size);
    ASSERT_TRUE(odd_ids);

    TEST_DART_CALL(dart_group_getmembers(even_group, even_ids));
    TEST_DART_CALL(dart_group_getmembers(odd_group, odd_ids));

    for(size_t i = 0; i < even_size; ++i)
    {
        ASSERT_TRUE(0 == even_ids[i] % 2);
    }

    for(size_t i = 0; i < odd_size; ++i)
    {
        ASSERT_TRUE(0 != odd_ids[i] % 2);
    }

    dart_group_t *all_group = (dart_group_t *) malloc(gsize);
    ASSERT_TRUE(all_group);

    TEST_DART_CALL(dart_group_init(all_group));

    TEST_DART_CALL(dart_group_union(even_group, odd_group, all_group));

    size_t all_size;
    TEST_DART_CALL(dart_group_size(all_group, &all_size));

    ASSERT_TRUE(all_size == size);

    dart_unit_t * all_ids =  (dart_unit_t *) malloc(sizeof(dart_unit_t) * all_size);
    ASSERT_TRUE(all_ids);

    TEST_DART_CALL(dart_group_getmembers(all_group, all_ids));

    for(size_t i = 0; i < size; ++i)
    {
        ASSERT_TRUE(((int) i) == all_ids[i]);
    }

    free(even_ids);
    free(odd_ids);
    free(all_ids);

    TEST_DART_CALL(dart_group_fini(even_group));
    TEST_DART_CALL(dart_group_fini(odd_group));
    TEST_DART_CALL(dart_group_fini(all_group));

    free(even_group);
    free(odd_group);
    free(all_group);
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
