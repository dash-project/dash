#include "TestBase.h"

#include <gtest/gtest.h>
#include <libdash.h>
#include <iostream>

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
  // Run Tests
  int ret = RUN_ALL_TESTS();
  // Finalize DASH
  dash::finalize();
  return ret;
}
