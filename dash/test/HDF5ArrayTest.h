#ifndef DASH__TEST__HDF5_ARRAY_TEST_H__INCLUDED
#define DASH__TEST__HDF5_ARRAY_TEST_H__INCLUDED

#ifdef DASH_ENABLE_HDF5

#include <gtest/gtest.h>
#include <libdash.h>

#include "TestBase.h"

// for CustomType test
#include "hdf5.h"


class HDF5ArrayTest : public ::testing::Test {
protected:
  dash::global_unit_t _dash_id;
  size_t      _dash_size;
  std::string _filename = "test_array.hdf5";
  std::string _dataset  = "data";

  HDF5ArrayTest()
  : _dash_id(0),
    _dash_size(0) {
    LOG_MESSAGE(">>> Test suite: HDFTest");
  }

  virtual ~HDF5ArrayTest() {
    LOG_MESSAGE("<<< Closing test suite: HDFTest");
  }

  virtual void SetUp() {
    dash::init(&TESTENV.argc, &TESTENV.argv);
    _dash_id   = dash::myid();
    _dash_size = dash::size();
    if(_dash_id == 0) {
      remove(_filename.c_str());
    }
    dash::Team::All().barrier();
    LOG_MESSAGE("===> Running test case with %d units ...",
                _dash_size);
  }

  virtual void TearDown() {
    dash::Team::All().barrier();
    if(_dash_id == 0) {
      remove(_filename.c_str());
    }
    LOG_MESSAGE("<=== Finished test case with %d units",
                _dash_size);
    dash::finalize();
  }
};

#endif // DASH_ENABLE_HDF5

#endif // DASH__TEST__HDF5_ARRAY_TEST_H__INCLUDED
