#ifdef DASH_ENABLE_HDF5

#ifndef DASH__TEST_HDF5Array_H
#define DASH__TEST_HDF5Array_H

#include <gtest/gtest.h>
#include <libdash.h>
#include <stdio.h>

class HDFArrayTest : public ::testing::Test {
  protected:
    dart_unit_t _dash_id;
    size_t      _dash_size;
    std::string _filename = "test_array.hdf5";
    std::string _table    = "data";

    HDFArrayTest()
        : _dash_id(0),
          _dash_size(0) {
        LOG_MESSAGE(">>> Test suite: HDFTest");
    }

    virtual ~HDFArrayTest() {
        LOG_MESSAGE("<<< Closing test suite: HDFTest");
    }

    virtual void SetUp() {
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
    }
};
#endif
#endif
