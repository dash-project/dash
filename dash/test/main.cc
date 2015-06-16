#include "TestBase.h"

#include <gtest/gtest.h>
#include <libdash.h>
#include <iostream>

#include "ArrayTest.h"
#include "PatternTest.h"
#include "CartesianTest.h"
#include "TeamSpecTest.h"

int main(int argc, char * argv[]) {
  // Init GoogleTest (strips gtest arguments from argv)
  ::testing::InitGoogleTest(&argc, argv);
  // Init DASH
  dash::init(&argc, &argv);
  std::cout << "-- Starting test on unit "
            << dash::myid()
            << std::endl;
  // Run Tests
  int ret = RUN_ALL_TESTS();
  // Finalize DASH
  dash::finalize();
  return ret;
}
