#ifndef DASH__TEST__TASKING_TEST_H_
#define DASH__TEST__TASKING_TEST_H_

#include "../TestBase.h"


/**
 * Test fixture for class dash::Pattern
 */
class TaskingTest : public dash::test::TestBase {
protected:
  int _dash_size;

  TaskingTest()
  : _dash_size(0)
  { }

  virtual void SetUp() {
    dash::test::TestBase::SetUp();
    _dash_size = dash::size();
  }
};

#endif // DASH__TEST__TASKING_TEST_H_

