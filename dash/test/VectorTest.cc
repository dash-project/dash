#include <gtest/gtest.h>

#include <libdash.h>
#include <dash/Vector.h>

#include "TestBase.h"
#include "VectorTest.h"

TEST_F(VectorTest, Initialization)
{
  typedef int                            value_t;
  typedef dash::CSRPattern<1>            pattern_t;
  typedef typename pattern_t::index_type index_t;

  auto nunits    = dash::size();
  auto myid      = dash::myid();
  // initial number of elements per unit:
  auto lcapacity = 5;
  auto capacity  = nunits * lcapacity;

  pattern_t pattern(capacity);
  dash::Vector<value_t, index_t, pattern_t> vector(pattern);

  // no elements added yet, size is 0:
  ASSERT_EQ_U(0, vector.size());
  ASSERT_EQ_U(0, vector.lsize());
  ASSERT_TRUE_U(vector.empty());

  // capacity is initial size:
  ASSERT_EQ_U(capacity,  vector.capacity());
  ASSERT_EQ_U(lcapacity, vector.lcapacity());
}

TEST_F(VectorTest, Reserve)
{
  typedef int                            value_t;
  typedef dash::CSRPattern<1>            pattern_t;
  typedef typename pattern_t::index_type index_t;

  auto nunits    = dash::size();
  auto myid      = dash::myid();
  // initial number of elements per unit to allocate:
  auto lcapacity = 5;
  auto capacity  = nunits * lcapacity;

  // initialize vector with initial capacity 0:
  pattern_t pattern(0);
  dash::Vector<value_t, index_t, pattern_t> vector(pattern);

  // no elements added yet, size is 0:
  ASSERT_EQ_U(0, vector.size());
  ASSERT_EQ_U(0, vector.lsize());
  ASSERT_TRUE_U(vector.empty());

  // capacity is 0 as 0 has been explicitly set as initial capacity:
  ASSERT_EQ_U(0, vector.capacity());
  ASSERT_EQ_U(0, vector.lcapacity());

  // delayed allocation of initial capacity:
  vector.reserve(capacity);
  ASSERT_EQ_U(capacity, vector.capacity());
  // no elements added to vector, size still 0:
  ASSERT_EQ_U(0, vector.size());
  ASSERT_EQ_U(0, vector.lsize());
}
