/*
 * ThreadsafetyTest.h
 *
 *  Created on: Jan 26, 2017
 *      Author: joseph
 */

#ifndef DASH_DASH_TEST_THREADSAFETYTEST_H_
#define DASH_DASH_TEST_THREADSAFETYTEST_H_

#include "TestBase.h"

#if defined(DASH_ENABLE_OPENMP)
#include <omp.h>
#endif // DASH_ENABLE_OPENMP


/**
 * Test fixture for onesided operations provided by DART.
 */
class ThreadsafetyTest : public dash::test::TestBase {
protected:
  int _num_threads = 1;

public:
  ThreadsafetyTest() {

#if defined(DASH_ENABLE_OPENMP)
#pragma omp parallel
    {
#pragma omp master
    {
      _num_threads = omp_get_num_threads();
    }
    }
#endif // DASH_ENABLE_OPENMP
  }

  virtual ~ThreadsafetyTest() {
  }
};


#endif /* DASH_DASH_TEST_THREADSAFETYTEST_H_ */
