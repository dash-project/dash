#ifdef DASH_ENABLE_HDF5

#ifndef DASH__TEST_HDF5_H
#define DASH__TEST_HDF5_H

#include <gtest/gtest.h>
#include <libdash.h>
#include <stdio.h>

class HDFTest : public ::testing::Test {
protected:
  dart_unit_t _dash_id;
  size_t      _dash_size;

  HDFTest()
  : _dash_id(0),
    _dash_size(0) {
    LOG_MESSAGE(">>> Test suite: HDFTest");
  }

  virtual ~HDFTest() {
    LOG_MESSAGE("<<< Closing test suite: HDFTest");
  }

  virtual void SetUp() {
    _dash_id   = dash::myid();
    _dash_size = dash::size();
		remove("test.hdf5");
    LOG_MESSAGE("===> Running test case with %d units ...",
                _dash_size);
  }

  virtual void TearDown() {
    dash::Team::All().barrier();
		remove("test.hdf5");
    LOG_MESSAGE("<=== Finished test case with %d units",
                _dash_size);
  }
};
#endif
#endif
