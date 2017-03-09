#ifndef DASH__TEST__HDF5_ARRAY_TEST_H__INCLUDED
#define DASH__TEST__HDF5_ARRAY_TEST_H__INCLUDED

#ifdef DASH_ENABLE_HDF5

#include "TestBase.h"

// for CustomType test
#include "hdf5.h"


class HDF5ArrayTest : public dash::test::TestBase {
protected:
  const std::string _filename = "test_array.hdf5";
  const std::string _dataset  = "data";

  HDF5ArrayTest() {
    LOG_MESSAGE(">>> Test suite: HDFTest");
  }

  virtual ~HDF5ArrayTest() {
    LOG_MESSAGE("<<< Closing test suite: HDFTest");
  }

  virtual void SetUp() {
    dash::test::TestBase::SetUp();

    if(dash::myid().id == 0) {
      remove(_filename.c_str());
    }
    dash::Team::All().barrier();
  }

  virtual void TearDown() {
    if(dash::myid().id == 0) {
      remove(_filename.c_str());
    }

    dash::test::TestBase::TearDown();
  }
};

#endif // DASH_ENABLE_HDF5

#endif // DASH__TEST__HDF5_ARRAY_TEST_H__INCLUDED
