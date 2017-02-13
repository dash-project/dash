#ifndef DASH__TEST__CSR_Pattern_TEST_H_
#define DASH__TEST__CSR_Pattern_TEST_H_

#include "TestBase.h"

/**
 * Test fixture for onesided operations provided by DART.
 */
class CSRPatternTest : public dash::test::TestBase {
protected:

  CSRPatternTest(){
    LOG_MESSAGE(">>> Test suite: CSRPatternTest");
  }

  virtual ~CSRPatternTest() {
    LOG_MESSAGE("<<< Closing test suite: CSRPatternTest");
  }

};


#endif // DASH__TEST__CSR_Pattern_TEST_H_