#include "TestBase.h"
#include "TestPrinter.h"

#include <gtest/gtest.h>
#include <libdash.h>
#include <iostream>

using ::testing::UnitTest;
using ::testing::TestEventListeners;

int main(int argc, char * argv[])
{
  char hostname[100];
  gethostname(hostname, 100);
  std::string host(hostname);

  // Init GoogleTest (strips gtest arguments from argv)
  ::testing::InitGoogleTest(&argc, argv);
  // Init DASH
  dash::init(&argc, &argv);
  dash::barrier();
  std::cout << "#### "
            << "Starting test on unit " << dash::myid() << " "
            << "(" << host << " PID: " << getpid() << ")"
            << std::endl;
  dash::barrier();

	// Change Test Printer
	UnitTest& unit_test = *UnitTest::GetInstance();
	TestEventListeners& listeners = unit_test.listeners();

	delete listeners.Release(listeners.default_result_printer());

	listeners.Append(new TestPrinter);

  // Run Tests
  int ret = RUN_ALL_TESTS();
  // Finalize DASH
  dash::finalize();
  return ret;
}
