#ifndef TEST_H
#define TEST_H
#include <dash/dart/if/dart.h>
#include <GASPI.h>
#include <gtest/gtest.h>

#define TEST_DART_CALL(func) EXPECT_EQ(DART_OK, func)
#define TEST_GASPI_CALL(func) EXPECT_EQ(GASPI_SUCCESS, func)

class GaspiPrinter : public ::testing::EmptyTestEventListener
{
        // Called after a failed assertion or a SUCCEED() invocation.
        virtual void OnTestPartResult(const ::testing::TestPartResult& test_part_result)
        {
            gaspi_rank_t rank;
            gaspi_proc_rank(&rank);
            printf("[%d]= %s in %s:%d\n%s\n",rank,test_part_result.failed() ? "\e[31mFailure\e[0m" : "\e[32mSuccess\e[0m"
                                            ,test_part_result.file_name()
                                            ,test_part_result.line_number()
                                            ,test_part_result.summary());
        }

        // Called after a test ends.
        virtual void OnTestEnd(const ::testing::TestInfo& test_info)
        {
            gaspi_rank_t rank;
            gaspi_proc_rank(&rank);
            const ::testing::TestResult * result = test_info.result();
            printf("[%d]=Test %s.%s -> %s.\n",rank,test_info.test_case_name()
                                              ,test_info.name()
                                              ,(result->Passed()) ? "\e[32mSuccess\e[0m" : "\e[31mFailure\e[0m");
        }
};

#endif /* TEST_H */
