#include <gtest/gtest.h>
#include <libdash.h>

#include "ArrayTest.h"

int main(int argc, char * argv[]) {
  // Init GoogleTest (strips gtest arguments from argv)
  ::testing::InitGoogleTest(&argc, argv);
  // Init DASH
  dash::init(&argc, &argv);
  // Run Tests
  int ret = RUN_ALL_TESTS();
  // Finalize DASH
  dash::finalize();
  return ret;
}
