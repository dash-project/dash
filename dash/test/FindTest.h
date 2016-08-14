#ifndef DASH__TEST__FIND_TEST_H_
#define DASH__TEST__FIND_TEST_H_

#include <gtest/gtest.h>
#include <libdash.h>
#include <vector>

/**
 * Test fixture for algorithm dash::find.
 */
class FindTest : public ::testing::Test {
protected:
<<<<<<< HEAD
  typedef int                                        Element_t;
  typedef dash::Array<Element_t>                     Array_t;
  typedef typename Array_t::pattern_type::index_type index_t;
=======
  typedef int                                         Element_t;
  typedef dash::Array<Element_t>                      Array_t;
  typedef typename Array_t::pattern_type::index_type  index_t;
>>>>>>> ebf1c0aa9589c55aa4c46194e2ff44d41f5e4650

  size_t _num_elem = 251;

  FindTest(){
  }

  virtual ~FindTest(){
  }

  virtual void SetUp(){
  }

  virtual void TearDown(){
  }
};

#endif // DASH__TEST__FIND_TEST_H_
