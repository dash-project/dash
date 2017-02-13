/*
 * ThreadsafetyTest.h
 *
 *  Created on: Jan 26, 2017
 *      Author: joseph
 */

#ifndef DASH_DASH_TEST_THREADSAFETYTEST_H_
#define DASH_DASH_TEST_THREADSAFETYTEST_H_

#include "TestBase.h"


/**
 * Test fixture for onesided operations provided by DART.
 */
class ThreadsafetyTest : public dash::test::TestBase {
protected:
  int _num_threads;

public:
  ThreadsafetyTest() {

  #pragma omp parallel
    {
  #pragma omp master
    {
      _num_threads = omp_get_num_threads();
    }
    }
  }

  virtual ~ThreadsafetyTest() {
  }
};


#endif /* DASH_DASH_TEST_THREADSAFETYTEST_H_ */
