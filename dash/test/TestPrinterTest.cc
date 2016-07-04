#include <libdash.h>
#include <gtest/gtest.h>

#include "TestBase.h"
#include "TestPrinterTest.h"

void failOnOneUnit(){
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
