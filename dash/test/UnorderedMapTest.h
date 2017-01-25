#ifndef DASH__TEST__UNORDERED_MAP_TEST_H_
#define DASH__TEST__UNORDERED_MAP_TEST_H_

#include "TestBase.h"

/**
 * Test fixture for class dash::UnorderedMap
 */
class UnorderedMapTest : public dash::test::TestBase {
protected:

  UnorderedMapTest() {
    LOG_MESSAGE(">>> Test suite: UnorderedMapTest");
  }

  virtual ~UnorderedMapTest() {
    LOG_MESSAGE("<<< Closing test suite: UnorderedMapTest");
  }
};

#endif // DASH__TEST__UNORDERED_MAP_TEST_H_
