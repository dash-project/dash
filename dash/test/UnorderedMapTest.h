#ifndef DASH__TEST__UNORDERED_MAP_TEST_H_
#define DASH__TEST__UNORDERED_MAP_TEST_H_

#include <gtest/gtest.h>
#include <libdash.h>

#include "TestBase.h"

/**
 * Test fixture for class dash::UnorderedMap
 */
class UnorderedMapTest : public ::testing::Test {
protected:
  size_t _dash_id;
  size_t _dash_size;

  UnorderedMapTest()
  : _dash_id(0),
    _dash_size(0) {
    LOG_MESSAGE(">>> Test suite: UnorderedMapTest");
    LOG_MESSAGE(">>> Hostname: %s PID: %d", _hostname().c_str(), _pid());
  }

  virtual ~UnorderedMapTest() {
    LOG_MESSAGE("<<< Closing test suite: UnorderedMapTest");
  }

  virtual void SetUp() {
    _dash_id   = dash::myid();
    _dash_size = dash::size();
    dash::barrier();
    LOG_MESSAGE("===> Running test case with %d units ...", _dash_size);
  }

  virtual void TearDown() {
    dash::barrier();
    LOG_MESSAGE("<=== Finished test case with %d units", _dash_size);
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

#endif // DASH__TEST__UNORDERED_MAP_TEST_H_
