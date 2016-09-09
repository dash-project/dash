#ifndef DASH__TEST__ISPARTITIONED_TEST_H_
#define DASH__TEST__ISPARTITIONED_TEST_H_

#include <gtest/gtest.h>
#include <libdash.h>
#include <vector>

/**
 * Test fixture for algorithm dash::find.
 */
class IsPartitionedTest : public ::testing::Test
{
protected:

  typedef int                                         Element_t;
  typedef dash::Array<Element_t>                      Array_t;
  typedef typename Array_t::pattern_type::index_type  index_t;

  size_t _num_elem = 251;

  IsPartitionedTest(){
  }

  virtual ~IsPartitionedTest(){
  }

  virtual void SetUp(){
  }

  virtual void TearDown(){
  }
};

#endif // DASH__TEST__ISPARTITIONED_TEST_H_
