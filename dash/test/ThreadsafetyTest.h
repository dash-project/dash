#ifndef DASH_DASH_TEST_THREADSAFETYTEST_H_
#define DASH_DASH_TEST_THREADSAFETYTEST_H_

#include "TestBase.h"

#ifdef DASH_ENABLE_OPENMP
#include <omp.h>
#endif // DASH_ENABLE_OPENMP


/**
 * Test fixture for onesided operations provided by DART.
 */
class ThreadsafetyTest : public dash::test::TestBase {
protected:
  int _num_threads = 1;

  ThreadsafetyTest() {

#ifdef DASH_ENABLE_OPENMP
#pragma omp parallel
    {
#pragma omp master
    {
      _num_threads = omp_get_num_threads();
    }
    }
    LOG_MESSAGE("Running ThreadsafetyTests with %i threads", _num_threads);

#endif // DASH_ENABLE_OPENMP

  }

  virtual ~ThreadsafetyTest() {
  }
};


#endif /* DASH_DASH_TEST_THREADSAFETYTEST_H_ */
