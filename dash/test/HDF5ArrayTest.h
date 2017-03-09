#ifndef DASH__TEST__HDF5_ARRAY_TEST_H__INCLUDED
#define DASH__TEST__HDF5_ARRAY_TEST_H__INCLUDED

#ifdef DASH_ENABLE_HDF5

#include "TestBase.h"

// for CustomType test
#include "hdf5.h"

class HDF5ArrayTest : public dash::test::TestBase {
 protected:
  std::string _filename = "test_array.hdf5";
  std::string _dataset = "data";
  bool _preserve;

  HDF5ArrayTest() { LOG_MESSAGE(">>> Test suite: HDFTest"); }

  virtual ~HDF5ArrayTest() { LOG_MESSAGE("<<< Closing test suite: HDFTest"); }

  virtual void SetUp() {
    dash::test::TestBase::SetUp();
    _preserve = dash::util::Config::get<bool>("DASH_HDF5_PRESERVE_FILE");
    if (dash::myid() == 0) {
      remove(_filename.c_str());
    }
    dash::Team::All().barrier();
  }

  virtual void TearDown() {
    if ((dash::myid() == 0) && !_preserve) {
      remove(_filename.c_str());
    }
    dash::test::TestBase::TearDown();
  }
};

#endif  // DASH_ENABLE_HDF5

#endif  // DASH__TEST__HDF5_ARRAY_TEST_H__INCLUDED
