#ifdef DASH_ENABLE_HDF5

#ifndef DASH__TEST__HDF5_MATRIX_TEST_H__INCLUDED
#define DASH__TEST__HDF5_MATRIX_TEST_H__INCLUDED

#include "TestBase.h"

#include <stdio.h>


class HDF5MatrixTest : public dash::test::TestBase {
protected:
  const std::string _filename = "test_matrix.hdf5";
  const std::string _dataset  = "data";

  HDF5MatrixTest() {
    LOG_MESSAGE(">>> Test suite: HDFTest");
  }

  virtual ~HDF5MatrixTest() {
    LOG_MESSAGE("<<< Closing test suite: HDFTest");
  }

  virtual void SetUp() {
    dash::test::TestBase::SetUp();

    if (dash::myid().id == 0) {
      remove(_filename.c_str());
    }
  }

  virtual void TearDown() {
    if (dash::myid().id == 0) {
      remove(_filename.c_str());
    }
    dash::test::TestBase::TearDown();
  }
};

#endif // DASH__TEST__HDF5_MATRIX_TEST_H__INCLUDED
#endif // DASH_ENABLE_HDF5
