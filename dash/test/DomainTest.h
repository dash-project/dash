#ifndef DASH__TEST__DOMAIN_TEST_H_
#define DASH__TEST__DOMAIN_TEST_H_

#include <gtest/gtest.h>
#include <libdash.h>

#include "TestBase.h"

/**
 * Test fixture for class dash::Domain
 */
class DomainTest : public ::testing::Test {
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
    dash::init(&TESTENV.argc, &TESTENV.argv);
    _dash_id   = dash::myid();
    _dash_size = dash::size();
    LOG_MESSAGE("===> Running test case with %zu units ...",
                _dash_size);
  }

  virtual void TearDown() {
    dash::Team::All().barrier();
    LOG_MESSAGE("<=== Finished test case with %zu units",
                _dash_size);
    dash::finalize();
  }
};

#endif // DASH__TEST__DOMAIN_TEST_H_
