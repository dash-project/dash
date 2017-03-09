#ifdef MPI_IMPL_ID

#ifndef DASH__UTIL__TEST_PRINTER_H_
#define DASH__UTIL__TEST_PRINTER_H_

#include <gtest/gtest.h>

#include <list>
#include <iostream>

#include <mpi.h>

#define TEST_NEUTRAL "\033[0;32m[----------] \033[m"
#define TEST_SUM     "\033[0;32m[==========] \033[m"
#define TEST_SUCCESS "\033[0;32m[  PASSED  ] \033[m"
#define TEST_SKIPPED "\033[0;33m[  SKIPPED ] \033[m"
#define TEST_FAILURE "\033[0;31m[  FAILED  ] \033[m"
#define TEST_ERROR   "\033[0;31m[  ERROR   ] \033[m"
#define TEST_OK      "\033[0;32m[      OK  ] \033[m"
#define TEST_RUN     "\033[0;32m[  RUN     ] \033[m"

using ::testing::EmptyTestEventListener;
using ::testing::InitGoogleTest;
using ::testing::Test;
using ::testing::TestCase;
using ::testing::TestEventListeners;
using ::testing::TestInfo;
using ::testing::TestPartResult;
using ::testing::UnitTest;
using ::testing::TestResult;

class TestPrinter : public EmptyTestEventListener {
  private:
  int  _myid;
  int  _size;
  bool _testcase_passed = true;
  std::list<std::string> _failed_tests;

  public:
  TestPrinter() {
    MPI_Comm_rank(MPI_COMM_WORLD, &_myid);
    MPI_Comm_size(MPI_COMM_WORLD, &_size);
  }

  private:
  // Called before any test activity starts.
  virtual void OnTestProgramStart(const UnitTest& unit_test) {
    if(_myid == 0){
      std::cout << TEST_NEUTRAL
              << unit_test.total_test_case_count()
              << " tests will be run."
              << std::endl;
    }
  }

  virtual void OnTestCaseStart(const TestCase& test_case){
    if(_myid == 0){
      std::cout << TEST_NEUTRAL
              << "run "
              << test_case.test_to_run_count()
              << " out of "
              << test_case.total_test_count()
              << " tests from "
              << test_case.name()
              << std::endl;
    }
  }

  // Called after a failed assertion or a SUCCEED() invocation.
  virtual void OnTestPartResult(const TestPartResult& test_part_result) {
    if(test_part_result.failed()){
      std::cout << TEST_ERROR 
                << "[UNIT " << _myid << "]" << " in "
                << test_part_result.file_name() << ":"
                << test_part_result.line_number() << std::endl
                << test_part_result.summary()
                << std::endl;
    }
  }

  // Called after all test activities have ended.
  virtual void OnTestProgramEnd(const UnitTest& unit_test) {
    MPI_Barrier(MPI_COMM_WORLD);
    if(_myid == 0){
      bool passed = unit_test.Passed() && _testcase_passed;

      std::cout << TEST_SUM
                << unit_test.test_to_run_count()
                << " tests from "
                << unit_test.test_case_to_run_count()
                << " test cases ran. ("
                << unit_test.elapsed_time()
                << " ms total)"
                << std::endl;

      if (passed) {
        std::cout << TEST_SUCCESS
                  << unit_test.successful_test_count()
                  << " tests passed"
                  << std::endl;
      } else {
        std::cout << TEST_FAILURE
                << unit_test.failed_test_count()
                << " tests, listed below"
                << std::endl;
        for(auto el : _failed_tests){
          std::cout << el << std::endl;
        }
      }
    }
  }

  // Called before a test starts.
  virtual void OnTestStart(const TestInfo& test_info) {
    if(_myid == 0){
      std::cout << TEST_RUN
                << test_info.test_case_name() << "."
                << test_info.name() << std::endl;
    }
  }

  // Called after a test ends.
  virtual void OnTestEnd(const TestInfo& test_info) {
    int success_units = 0;
    bool passed       = test_info.result()->Passed();
    int unit_passed   = passed ? 1 : 0; 

    MPI_Reduce(&unit_passed, &success_units, 1,
               MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if(_myid == 0){
      passed            = (success_units == _size);
      _testcase_passed &= passed;

      std::string res;
      res += (passed ? TEST_OK : TEST_FAILURE );
      res += test_info.test_case_name();
      res += ".";
      res += test_info.name();
      std::cout << res << std::endl;

      if(!passed){
        _failed_tests.push_front(res);
        // Mark test as failed on each units
        ADD_FAILURE() << "Testcase failed at least on one unit";
      }
    }
    // prevent overlapping of tests
    MPI_Barrier(MPI_COMM_WORLD);
  }
};

#endif // DASH__UTIL__TEST_PRINTER_H_

#endif // MPI_IMPL_ID
