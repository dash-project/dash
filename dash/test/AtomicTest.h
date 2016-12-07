#ifndef DASH__TEST__ATOMIC_TEST_H_
#define DASH__TEST__ATOMIC_TEST_H_

#include <gtest/gtest.h>
#include <libdash.h>

#include "TestBase.h"

/**
 * Test fixture for class dash::Atomic
 */
class AtomicTest : public dash::test::TestBase {
protected:
  size_t _dash_id;
  size_t _dash_size;

  AtomicTest()
  : _dash_id(0),
    _dash_size(0) {
  }

  virtual ~AtomicTest() {
    LOG_MESSAGE("<<< Closing test suite: AtomicTest");
  }

  virtual void SetUp() {
    dash::test::TestBase::SetUp();
    _dash_id   = dash::myid();
    _dash_size = dash::size();
  }

  virtual void TearDown() {
    dash::test::TestBase::TearDown();
  }

protected:
  std::string _hostname() {
    char hostname[100];
    gethostname(hostname, 100);
    return std::string(hostname);
  }

  int _pid() {
    return static_cast<int>(getpid());
  }
};

#endif // DASH__TEST__ATOMIC_TEST_H_
