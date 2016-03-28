#include <gtest/gtest.h>

#include <libdash.h>
#include <dash/List.h>

#include "TestBase.h"
#include "ListTest.h"

TEST_F(ListTest, Initialization)
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
  dash::List<value_t, index_t, pattern_t> list(pattern);

  // no elements added yet, size is 0:
  ASSERT_EQ_U(0, list.size());
  ASSERT_EQ_U(0, list.lsize());
  ASSERT_TRUE_U(list.empty());

  // capacity is initial size:
  ASSERT_EQ_U(capacity,  list.capacity());
  ASSERT_EQ_U(lcapacity, list.lcapacity());

  list.local.push_back(myid + 1);
  list.local.push_back(myid + 2);
  list.local.push_back(myid + 3);

  list.barrier();
  ASSERT_EQ_U(3,          list.lsize());
  ASSERT_EQ_U(3 * nunits, list.size());
}

