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
  // Initial number of elements per unit:
  auto lcap_init = 5;
  // Initial global capacity:
  auto gcap_init = nunits * lcap_init;
  // Number of elements to re-allocate at every unit:
  auto nalloc    = 2;
  // Number of elements to be added by every unit.
  // Set greater than initial local capacity to force re-allocation:
  auto nlocal    = lcap_init + nalloc;
  // Total number of elements to be added:
  auto nglobal   = nlocal * nunits;
  // Local capacity after local insert operations:
  auto lcap_new  = nlocal;
  // Global capacity after committing all insert operations:
  auto gcap_new  = nunits * nlocal;

  dash::List<value_t> list(gcap_init);
  DASH_LOG_DEBUG("ListTest.Initialization", "list initialized");

  // no elements added yet, size is 0:
  EXPECT_EQ_U(0, list.size());
  EXPECT_EQ_U(0, list.lsize());
  EXPECT_TRUE_U(list.empty());
  // capacities have initial setting:
  EXPECT_EQ_U(gcap_init, list.capacity());
  EXPECT_EQ_U(lcap_init, list.lcapacity());

  dash::barrier();

  for (auto li = 0; li < nlocal; ++li) {
    DASH_LOG_DEBUG("ListTest.Initialization", "list.local.push_back()");
    list.local.push_back(myid + 1 + li);
  }
  // No commit yet, only changes of local size should be visible:
  EXPECT_EQ_U(nlocal,    list.size());
  EXPECT_EQ_U(nlocal,    list.lsize());
  EXPECT_EQ_U(nlocal,    list.local.size());
  EXPECT_EQ_U(lcap_new,  list.lcapacity());
  EXPECT_EQ_U(gcap_init + nalloc, list.capacity());

  DASH_LOG_DEBUG("ListTest.Initialization", "list.barrier()");
  list.barrier();
  DASH_LOG_DEBUG("ListTest.Initialization", "list.barrier() passed");

  // Commit passed, all changes should be globally visible:
  EXPECT_EQ_U(nglobal,   list.size());
  EXPECT_EQ_U(nlocal,    list.lsize());
  EXPECT_EQ_U(nlocal,    list.local.size());
  EXPECT_EQ_U(gcap_new,  list.capacity());
  EXPECT_EQ_U(lcap_new,  list.lcapacity());

  for (auto li = 0; li < list.local.size(); ++li) {
    DASH_LOG_DEBUG("ListTest.Initialization",
                   "validate list.local[", li, "]");
    value_t expect = myid + 1 + li;
//  value_t actual = *(list.local.begin() + li);
//  EXPECT_EQ_U(expect, actual);
    LOG_MESSAGE("list.local[%d] = %d", li, *(list.local.begin() + li));
  }
}

