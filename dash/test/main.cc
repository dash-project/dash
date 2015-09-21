#include "TestBase.h"

#include <gtest/gtest.h>
#include <libdash.h>
#include <iostream>

#include "BlockPatternTest.h"
#include "TilePatternTest.h"
#include "CartesianTest.h"
#include "TeamTest.h"
#include "TeamSpecTest.h"

#include "ArrayTest.h"
#include "MatrixTest.h"

#include "ForEachTest.h"
#include "MinElementTest.h"
#include "MaxElementTest.h"
#include "STLAlgorithmTest.h"

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
