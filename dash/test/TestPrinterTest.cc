
#include "TestPrinterTest.h"
#include "TestPrinter.h"

void failOnOneUnit() {
  if(dash::myid() == 2){
    ADD_FAILURE() << "FAILED";
  }
  dash::barrier();
}

TEST_F(TestPrinterTest, FailOnOneUnit) {
// To test the TestPrinter, enable this test and run with at least 2 units
#if 0
  failOnOneUnit();
#else
  SUCCEED();
#endif
}

TEST_F(TestPrinterTest, FailWithExpect) {
// To test the TestPrinter, enable this test and run with at least 2 units
#if 0
  EXPECT_EQ(1,2);
#else
  SUCCEED();
#endif
}
