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

TEST(Memory, Create_delete)
{
    TEST_GASPI_CALL(gaspi_barrier(GASPI_GROUP_ALL, GASPI_BLOCK));
    TEST_GASPI_CALL(gaspi_segment_create(29, sizeof(int) * val_count, GASPI_GROUP_ALL, GASPI_BLOCK, GASPI_MEM_INITIALIZED));
    TEST_GASPI_CALL(gaspi_segment_create(30, sizeof(int) * val_count, GASPI_GROUP_ALL, GASPI_BLOCK, GASPI_MEM_INITIALIZED));
    TEST_GASPI_CALL(gaspi_segment_delete(29));
    TEST_GASPI_CALL(gaspi_segment_delete(30));
    TEST_GASPI_CALL(gaspi_barrier(GASPI_GROUP_ALL, GASPI_BLOCK));
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

    TEST_GASPI_CALL(gaspi_proc_init(GASPI_BLOCK));
    int ret = RUN_ALL_TESTS();
    TEST_GASPI_CALL(gaspi_proc_term(GASPI_BLOCK));

    return ret;
}
