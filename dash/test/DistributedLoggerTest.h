#ifndef DASH__TEST__DISTRIBUTED_LOGGER_TEST_H_
#define DASH__TEST__DISTRIBUTED_LOGGER_TEST_H_

#include "TestBase.h"


/**
 * Test fixture for the distributed logger
 */
class DistributedLoggerTest : public dash::test::TestBase {
protected:

  DistributedLoggerTest() 
  {
    LOG_MESSAGE(">>> Test suite: DistributedLoggerTest");
  }

  virtual ~DistributedLoggerTest() {
    LOG_MESSAGE("<<< Closing test suite: DistributedLoggerTest");
  }
};

#endif // DASH__TEST__DISTRIBUTED_LOGGER_TEST_H_

