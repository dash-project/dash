#ifndef DASH_DASH_TEST_DARTLOCKTEST_H_
#define DASH_DASH_TEST_DARTLOCKTEST_H_

#include "../TestBase.h"

#ifdef DASH_ENABLE_OPENMP
#include <omp.h>
#endif // DASH_ENABLE_OPENMP


/**
 * Test fixture for dart_lock implemeation
 */
class DARTLockTest : public dash::test::TestBase {
protected:

  DARTLockTest() {}

  virtual ~DARTLockTest() {}
};


#endif /* DASH_DASH_TEST_DARTLOCKTEST_H_ */
