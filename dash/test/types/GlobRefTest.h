#ifndef DASH__TEST__GLOBREF_TEST_H_
#define DASH__TEST__GLOBREF_TEST_H_

#include <gtest/gtest.h>

#include "../TestBase.h"


/**
 * Test fixture for \c dash::GlobRef.
 */
class GlobRefTest : public dash::test::TestBase {
protected:
  size_t _dash_id    = 0;
  size_t _dash_size  = 0;

  virtual void SetUp() {
    dash::test::TestBase::SetUp();
    _dash_id   = dash::myid();
    _dash_size = dash::size();
  }
};

#endif // DASH__TEST__GLOBREF_TEST_H_
