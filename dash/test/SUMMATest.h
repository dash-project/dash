#ifndef DASH__TEST__SUMMA_TEST_H_
#define DASH__TEST__SUMMA_TEST_H_

#include <gtest/gtest.h>
#include <libdash.h>
#include "TestBase.h"

#include <sstream>
#include <iomanip>

/**
 * Test fixture for algorithm \c dash::summa.
 */
class SUMMATest : public ::testing::Test {
protected:
  size_t _dash_id;
  size_t _dash_size;
  int _num_elem;

  SUMMATest()
  : _dash_id(0),
    _dash_size(0) {
    LOG_MESSAGE(">>> Test suite: SUMMATest");
    LOG_MESSAGE(">>> Hostname: %s PID: %d", _hostname().c_str(), _pid());
  }

  virtual ~SUMMATest() {
    LOG_MESSAGE("<<< Closing test suite: SUMMATest");
  }

  virtual void SetUp() {
    _dash_id   = dash::myid();
    _dash_size = dash::size();
    LOG_MESSAGE("===> Running test case with %lu units ...",
                _dash_size);
  }

  virtual void TearDown() {
    dash::Team::All().barrier();
    LOG_MESSAGE("<=== Finished test case with %lu units",
                _dash_size);
  }

  template<typename MatrixT>
  void print_matrix(const std::string & name, MatrixT & matrix) const {
    typedef typename MatrixT::value_type value_t;
    typedef typename MatrixT::index_type index_t;
    // Print local copy of matrix to avoid interleaving of matrix values
    // and log messages:
    std::vector< std::vector<value_t> > values;
    for (auto row = 0; row < matrix.extent(1); ++row) {
      std::vector<value_t> row_values;
      for (auto col = 0; col < matrix.extent(0); ++col) {
        value_t value = matrix[col][row];
        row_values.push_back(value);
      }
      values.push_back(row_values);
    }
    DASH_LOG_DEBUG("SUMMATest.print_matrix", "summa.matrix", name);
    for (auto row : values) {
      std::ostringstream ss;
      for (auto val : row) {
        ss << std::setprecision(1) << std::fixed << std::setw(4)
           << val << " ";
      }
      DASH_LOG_DEBUG("SUMMATest.print_matrix", "summa.matrix", ss.str());
    }
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

#endif // DASH__TEST__SUMMA_TEST_H_
