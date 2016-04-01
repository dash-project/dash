#ifndef DASH__TEST__ARRAY_LOCAL_TEST_H_
#define DASH__TEST__ARRAY_LOCAL_TEST_H_

#include <gtest/gtest.h>
#include <libdash.h>

#include "TestBase.h"

#define fielddim 4
#define NUM_SAMPLES 110592
#define FEAT_MAX_LEN ((NUM_SAMPLES) * (fielddim) * (2))
#define STD_DEVIATION 512

#define MAX_LEN 15
typedef struct DGNode_s {
  void **inArc, **outArc;
  int maxInDegree,maxOutDegree;
  int inDegree,outDegree;
  int id;
  char name[MAX_LEN];
  int in[4], out[4];
  int depth,height,width;
  int color,attribute,address,verified;
  struct {
    int len;
    double val[FEAT_MAX_LEN];
  } feat;

  DGNode_s(const char *pname)
  {
    strcpy(name, pname);
  }
} DGNode;

#define DASH__TEST__ARRAY_LOCAL_TEST_H_
/**
 * Test fixture for class dash::Array
 */
class ArrayTestLocal : public ::testing::Test {
protected:
  size_t _dash_id;
  size_t _dash_size;
  int _num_elem;

  ArrayTestLocal()
    : _dash_id(0),
      _dash_size(0),
      _num_elem(0) {
    LOG_MESSAGE(">>> Test suite: ArrayTestLocal");
  }

  virtual ~ArrayTestLocal() {
    LOG_MESSAGE("<<< Closing test suite: ArrayTestLocal");
  }

  virtual void SetUp() {
    _dash_id   = dash::myid();
    _dash_size = dash::size();
    _num_elem  = 10;
    LOG_MESSAGE("===> Running test case with %d units ...",
                _dash_size);
  }

  virtual void TearDown() {
    dash::Team::All().barrier();
    LOG_MESSAGE("<=== Finished test case with %d units",
                _dash_size);
  }
};

#endif // DASH__TEST__ARRAY_TEST_H_
