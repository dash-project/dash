#include <gtest/gtest.h>

#include <libdash.h>
#include <dash/List.h>

#include "TestBase.h"
#include "ListTest.h"

TEST_F(ListTest, Initialization)
{
  typedef int                   value_t;
  typedef dash::default_index_t index_t;

  auto nunits    = dash::size();
  auto myid      = dash::myid();
  // initial number of elements per unit:
  auto lcapacity = 5;
  auto capacity  = nunits * lcapacity;

  dash::List<value_t> list(capacity);

  // no elements added yet, size is 0:
  EXPECT_EQ_U(0, list.size());
  EXPECT_EQ_U(0, list.lsize());
  EXPECT_TRUE_U(list.empty());

  // capacity is initial size:
  EXPECT_EQ_U(capacity,  list.capacity());
  EXPECT_EQ_U(lcapacity, list.lcapacity());

  list.local.push_back(myid + 1);
  list.local.push_back(myid + 2);
  list.local.push_back(myid + 3);
  EXPECT_EQ_U(3,            list.lsize());
  EXPECT_EQ_U(list.lsize(), list.local.size());

  list.barrier();

  for (auto li = 0; li < list.local.size(); ++li) {
    LOG_MESSAGE("list.local[%d] = %d", li, *(list.local.begin() + li));
  }

  EXPECT_EQ_U(3 * nunits, list.size());
}

