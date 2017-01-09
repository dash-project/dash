
#include "ListTest.h"

#include <dash/List.h>


TEST_F(ListTest, Initialization)
{
  typedef int                   value_t;
  typedef dash::default_index_t index_t;

  auto nunits    = dash::size();
  auto myid      = dash::myid();
  // Size of local commit buffer:
  auto lbuf_size = 2;
  // Initial number of elements per unit:
  auto lcap_init = 3;
  // Initial global capacity:
  auto gcap_init = nunits * lcap_init;
  // Number of elements to insert beyond initial capacity at every unit:
  auto nalloc    = lbuf_size + 3;
  // Number of elements to be added by every unit.
  // Set greater than initial local capacity to force re-allocation:
  auto nlocal    = lcap_init + nalloc;
  // Total number of elements to be added:
  auto nglobal   = nlocal * nunits;
  // Local capacity after local insert operations:
  auto lcap_new  = lcap_init +
                   lbuf_size * dash::math::div_ceil(nalloc, lbuf_size);
  // Global capacity after committing all insert operations:
  auto gcap_new  = gcap_init + nunits * (lcap_new - lcap_init);
  // Global capacity visible to local unit after local insert operations:
  auto gcap_loc  = gcap_init + (lcap_new - lcap_init);

  dash::List<value_t> list(gcap_init, lbuf_size);
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
    value_t v = 1000 * (myid + 1) + li;
    DASH_LOG_DEBUG("ListTest.Initialization",
                   "list.local.push_back(", v, ")");
    list.local.push_back(v);
  }
  // No commit yet, only changes of local size should be visible:
  EXPECT_EQ_U(nlocal,    list.size());
  EXPECT_EQ_U(nlocal,    list.lsize());
  EXPECT_EQ_U(nlocal,    list.local.size());
  EXPECT_EQ_U(lcap_new,  list.lcapacity());
  EXPECT_EQ_U(gcap_loc,  list.capacity());

  // Validate local values before commit:
  for (auto li = 0; li < list.local.size(); ++li) {
    DASH_LOG_DEBUG("ListTest.Initialization",
                   "validate list.local[", li, "]");
    auto    l_node_unattached = *(list.local.begin() + li);
    DASH_LOG_DEBUG_VAR("ListTest.Initialization", l_node_unattached.value);
    DASH_LOG_DEBUG_VAR("ListTest.Initialization", l_node_unattached.lprev);
    DASH_LOG_DEBUG_VAR("ListTest.Initialization", l_node_unattached.lnext);
    value_t expect = 1000 * (myid + 1) + li;
    value_t actual = l_node_unattached.value;
    EXPECT_EQ_U(expect, actual);
  }

  DASH_LOG_DEBUG("ListTest.Initialization", "list.barrier()");
  list.barrier();
  DASH_LOG_DEBUG("ListTest.Initialization", "list.barrier() passed");

  // Commit passed, all changes should be globally visible:
  EXPECT_EQ_U(nglobal,   list.size());
  EXPECT_EQ_U(nlocal,    list.lsize());
  EXPECT_EQ_U(nlocal,    list.local.size());
  EXPECT_EQ_U(gcap_new,  list.capacity());
  EXPECT_EQ_U(lcap_new,  list.lcapacity());

  // Validate local values after commit:
  for (auto li = 0; li < list.local.size(); ++li) {
    DASH_LOG_DEBUG("ListTest.Initialization",
                   "validate list.local[", li, "]");
    auto    l_node_attached = *(list.local.begin() + li);
    DASH_LOG_DEBUG_VAR("ListTest.Initialization", l_node_attached.value);
    DASH_LOG_DEBUG_VAR("ListTest.Initialization", l_node_attached.lprev);
    DASH_LOG_DEBUG_VAR("ListTest.Initialization", l_node_attached.lnext);
    value_t expect = 1000 * (myid + 1) + li;
    value_t actual = l_node_attached.value;
    EXPECT_EQ_U(expect, actual);
  }
}

