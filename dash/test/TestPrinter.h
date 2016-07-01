#ifndef DASH__UTIL__TEST_PRINTER_H_
#define DASH__UTIL__TEST_PRINTER_H_

#include "gtest/gtest.h"

using ::testing::EmptyTestEventListener;
using ::testing::InitGoogleTest;
using ::testing::Test;
using ::testing::TestCase;
using ::testing::TestEventListeners;
using ::testing::TestInfo;
using ::testing::TestPartResult;
using ::testing::UnitTest;

class TestPrinter : public EmptyTestEventListener {
	private:
  // Called before any test activity starts.
  virtual void OnTestProgramStart(const UnitTest& unit_test) {
			std::cout << "[----------] "
							<< unit_test.total_test_case_count()
							<< " tests will be run."
							<< std::endl;
	}

	virtual void OnTestCaseStart(const TestCase& test_case){
			std::cout << "[----------]"
							<< " run "
							<< test_case.test_to_run_count()
							<< " out of "
							<< test_case.total_test_count()
							<< " tests from "
  						<< test_case.name()
							<< std::endl;
	}

  // Called after all test activities have ended.
  virtual void OnTestProgramEnd(const UnitTest& unit_test) {
			std::cout << "[==========] "
								<< unit_test.test_to_run_count()
								<< " tests from "
								<< unit_test.test_case_to_run_count()
								<< " test cases ran. ("
								<< unit_test.elapsed_time()
								<< " ms total)"
								<< std::endl;

			std::cout << (unit_test.Passed() ? "[  PASSED  ]" : "[  FAILED  ]") << " "
								<< unit_test.failed_test_count()
								<< " tests, listed below"
								<< std::endl;
			// TODO: Print failed tests
  }

  // Called before a test starts.
  virtual void OnTestStart(const TestInfo& test_info) {
		std::cout << "[ RUN      ] "
              << test_info.test_case_name() << "."
							<< test_info.name() << std::endl;
  }

  // Called after a test ends.
  virtual void OnTestEnd(const TestInfo& test_info) {
		std::cout << (test_info.result()->Passed() ? "[       OK ] " : "[  FAILED  ] ")
              << test_info.test_case_name() << "."
							<< test_info.name() << std::endl;
  }
};

#endif // DASH__UTIL__TEST_PRINTER_H_
 
