#ifndef DASH__TEST__ARRAY_LARGE_STRUCT_TEST_H_
#define DASH__TEST__ARRAY_LARGE_STRUCT_TEST_H_

#include <gtest/gtest.h>
#include <libdash.h>

#include "TestBase.h"

#if defined (DASH_ENABLE_REGRESSION_TEST)
#  define FEAT_MAX_LEN 10000000
#else
#  define FEAT_MAX_LEN 100
#endif

#define MAX_LEN 15

typedef struct DGNode_s {
  int    len;
  double val[FEAT_MAX_LEN];
} DGNode;

#define DASH__TEST__ARRAY_LOCAL_TEST_H_
/**
 * Test fixture for class dash::Array
 */
class ArrayLargeStruct : public dash::test::TestBase {
protected:
  size_t _dash_id;
  size_t _dash_size;

  ArrayLargeStruct()
    : _dash_id(0),
      _dash_size(0) {
    LOG_MESSAGE(">>> Test suite: ArrayLargeStruct");
  }

  virtual ~ArrayLargeStruct() {
    LOG_MESSAGE("<<< Closing test suite: ArrayLargeStruct");
  }

  virtual void SetUp() {
    dash::test::TestBase::SetUp();
    _dash_id   = dash::myid();
    _dash_size = dash::size();
  }

  virtual void TearDown() {
    dash::test::TestBase::TearDown();
  }
};

#endif // DASH__TEST__ARRAY_TEST_H_
