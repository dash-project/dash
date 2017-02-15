#ifndef DASH__TEST__DOMAIN_TEST_H_
#define DASH__TEST__DOMAIN_TEST_H_

#include "TestBase.h"

/**
 * Test fixture for class dash::Domain
 */
class DomainTest : public dash::test::TestBase {
protected:
  size_t _dash_id;
  size_t _dash_size;

  DomainTest()
  : _dash_id(0),
    _dash_size(0) {
    LOG_MESSAGE(">>> Test suite: DomainTest");
  }

  virtual ~DomainTest() {
    LOG_MESSAGE("<<< Closing test suite: DomainTest");
  }

  virtual void SetUp() {
    dash::test::TestBase::SetUp();
    _dash_id   = dash::myid();
    _dash_size = dash::size();
  }
};

#endif // DASH__TEST__DOMAIN_TEST_H_
