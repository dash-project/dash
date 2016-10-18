#ifndef DASH__TEST__GLOB_DYNAMIC_MEM_TEST_H_
#define DASH__TEST__GLOB_DYNAMIC_MEM_TEST_H_

#include <gtest/gtest.h>
#include <libdash.h>

#include "TestBase.h"

/**
 * Test fixture for class dash::GlobDynamicMem
 */
class GlobDynamicMemTest : public ::testing::Test {
protected:
  size_t _dash_id;
  size_t _dash_size;

  GlobDynamicMemTest()
  : _dash_id(0),
    _dash_size(0)
  {
    LOG_MESSAGE(">>> Test suite: GlobDynamicMemTest");
    LOG_MESSAGE(">>> Hostname: %s PID: %d", _hostname().c_str(), _pid());
  }

  virtual ~GlobDynamicMemTest() {
    LOG_MESSAGE("<<< Closing test suite: GlobDynamicMemTest");
  }

  virtual void SetUp() {
    _dash_id   = dash::myid();
    _dash_size = dash::size();
    dash::Team::All().barrier();
    LOG_MESSAGE("===> Running test case with %ld units ...",
                _dash_size);
  }

  virtual void TearDown() {
    dash::Team::All().barrier();
    LOG_MESSAGE("<=== Finished test case with %ld units",
                _dash_size);
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

#endif // DASH__TEST__GLOB_DYNAMIC_MEM_TEST_H_
