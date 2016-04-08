#ifndef DASH__TEST__MATRIX_TEST_H_
#define DASH__TEST__MATRIX_TEST_H_

#include <gtest/gtest.h>
#include <libdash.h>

#include <string>

/**
 * Test fixture for class dash::Matrix
 */
class MatrixTest : public ::testing::Test {
protected:
  size_t _dash_id;
  size_t _dash_size;

  MatrixTest()
  : _dash_id(0),
    _dash_size(0) {
    LOG_MESSAGE(">>> Test suite: MatrixTest");
    LOG_MESSAGE(">>> Hostname: %s PID: %d", _hostname().c_str(), _pid());
  }

  virtual ~MatrixTest() {
    LOG_MESSAGE("<<< Closing test suite: MatrixTest");
  }

  virtual void SetUp() {
    dash::Team::All().barrier();
    _dash_id   = dash::myid();
    _dash_size = dash::size();
    LOG_MESSAGE("===> Running test case with %d units ...",
                _dash_size);
  }

  virtual void TearDown() {
    dash::Team::All().barrier();
    LOG_MESSAGE("<=== Finished test case with %d units",
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

#endif // DASH__TEST__MATRIX_TEST_H_
