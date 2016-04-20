#include <libdash.h>
#include <gtest/gtest.h>

#include "TestBase.h"
#include "UnorderedMapTest.h"

#include <vector>
#include <algorithm>


TEST_F(UnorderedMapTest, Initialization)
{
  typedef int                                  key_t;
  typedef double                               mapped_t;
  typedef dash::UnorderedMap<key_t, mapped_t>  map_t;
  typedef typename map_t::iterator             map_iterator;
  typedef typename map_t::value_type           map_value;

  auto nunits    = dash::size();
  auto myid      = dash::myid();
  // Size of local commit buffer:
  auto lbuf_size = 1;
  // Initial number of elements per unit:
  auto lcap_init = 1;
  // Initial global capacity:
  auto gcap_init = nunits * lcap_init;
  // Number of elements to insert:
  auto ninsert   = 3;

  map_t map(gcap_init, lbuf_size);

  EXPECT_EQ_U(0, map.size());
  EXPECT_EQ_U(0, map.lsize());
  EXPECT_EQ_U(gcap_init, map.capacity());
  EXPECT_EQ_U(lcap_init, map.lcapacity());

  dash::barrier();
  DASH_LOG_DEBUG("UnorderedMapTest.Initialization", "map initialized");

  if (_dash_id == 0) {
    for (auto li = 0; li < ninsert; ++li) {
      DASH_LOG_DEBUG("UnorderedMapTest.Initialization", "insert element");
      key_t     key    = 100 * (_dash_id + 1) + li;
      mapped_t  mapped = 1.0 * (_dash_id + 1) + (0.01 * li);
      map_value value({ key, mapped });

      auto inserted = map.insert(value);
      EXPECT_TRUE_U(inserted.second);
      auto existing = map.insert(value);
      EXPECT_FALSE_U(existing.second);
    }
  }
  dash::barrier();

  DASH_LOG_DEBUG("UnorderedMapTest.Initialization", "committing elements");
  map.barrier();

  if (_dash_id == 0) {
    // Validate elements after commit:
    DASH_LOG_DEBUG("UnorderedMapTest.Initialization",
                   "validate values after commit");
    int li = 0;
    for (auto git = map.begin(); git != map.end(); ++git) {
      key_t     key    = 100 * (_dash_id + 1) + li;
      mapped_t  mapped = 1.0 * (_dash_id + 1) + (0.01 * li);
      map_value expect({ key, mapped });
      map_value actual = static_cast<map_value>(*git);
      DASH_LOG_DEBUG("UnorderedMapTest.Initialization", "after commit",
                     "git:",   git,
                     "value:", actual.first, "->", actual.second);
      EXPECT_EQ_U(expect, actual);
      li++;
    }
  }
  map.barrier();
}

TEST_F(UnorderedMapTest, BalancedGlobalInsert)
{
  typedef int                                  key_t;
  typedef double                               mapped_t;
  typedef dash::UnorderedMap<key_t, mapped_t>  map_t;
  typedef typename map_t::iterator             map_iterator;
  typedef typename map_t::value_type           map_value;

  map_t map;
  EXPECT_EQ_U(0, map.size());
  EXPECT_EQ_U(0, map.lsize());

  // key-value pair to be inserted:
  int       elem_per_unit = 2;
  key_t     key_a    = 100 * (_dash_id + 1) + 1;
  mapped_t  mapped_a = 1.0 * (_dash_id + 1) + 0.01;
  map_value value_a({ key_a, mapped_a });
  key_t     key_b    = 100 * (_dash_id + 1) + 2;
  mapped_t  mapped_b = 1.0 * (_dash_id + 1) + 0.02;
  map_value value_b({ key_b, mapped_b });

  DASH_LOG_TRACE("UnorderedMapTest.BalancedGlobalInsert", "insert elements");
  auto insertion_a = map.insert(value_a);
  EXPECT_TRUE_U(insertion_a.second);
  auto existing_a  = map.insert(value_a);
  EXPECT_FALSE_U(existing_a.second);
  auto insertion_b = map.insert(value_b);
  EXPECT_TRUE_U(insertion_a.second);
  auto existing_b  = map.insert(value_b);
  EXPECT_FALSE_U(existing_b.second);

  DASH_LOG_DEBUG("UnorderedMapTest.BalancedGlobalInsert",
                 "map size before commit:", map.size(),
                 "local size:", map.lsize());
  EXPECT_EQ_U(elem_per_unit, map.lsize());
  EXPECT_EQ_U(map.lsize(),   map.size());

  DASH_LOG_TRACE("UnorderedMapTest.BalancedGlobalInsert",
                 "validating global elements before commit");
  // Changes of global elements must be immediately visible to the unit that
  // performed the changes (i.e. without committing):
  int gidx = 0;
  for (auto git = map.begin(); git != map.end(); ++git) {
    key_t     key;
    mapped_t  mapped;
    if (gidx % 2 == 0) {
      key    = key_a;
      mapped = mapped_a;
    } else {
      key    = key_b;
      mapped = mapped_b;
    }
    map_value expect({ key, mapped });
    map_value actual = *git;
    DASH_LOG_TRACE("UnorderedMapTest.BalancedGlobalInsert",
                   "before commit:",
                   "gidx:",  gidx,
                   "unit:",  _dash_id,
                   "git:",   git,
                   "value:", actual.first, "->", actual.second);
    EXPECT_EQ_U(expect, actual);
    gidx++;
  }

  DASH_LOG_DEBUG("UnorderedMapTest.BalancedGlobalInsert",
                 "committing elements");
  map.barrier();

  DASH_LOG_DEBUG("UnorderedMapTest.BalancedGlobalInsert",
                 "map size after commit:", map.size(),
                 "local size:", map.lsize());
  EXPECT_EQ_U(elem_per_unit,                map.lsize());
  EXPECT_EQ_U(dash::size() * elem_per_unit, map.size());

  DASH_LOG_TRACE("UnorderedMapTest.BalancedGlobalInsert",
                 "validating global elements after commit");
  gidx = 0;
  for (auto git = map.begin(); git != map.end(); ++git) {
    auto      unit = gidx / elem_per_unit;
    key_t     key;
    mapped_t  mapped;
    if (gidx % 2 == 0) {
      key    = 100 * (unit + 1) + 1;
      mapped = 1.0 * (unit + 1) + 0.01;
    } else {
      key    = 100 * (unit + 1) + 2;
      mapped = 1.0 * (unit + 1) + 0.02;
    }
    map_value expect({ key, mapped });
    map_value actual = *git;
    DASH_LOG_TRACE("UnorderedMapTest.BalancedGlobalInsert",
                   "after commit:",
                   "gidx:",  gidx,
                   "unit:",  unit,
                   "git:",   git,
                   "value:", actual.first, "->", actual.second);
    EXPECT_EQ_U(expect, actual);
    gidx++;
  }
}

TEST_F(UnorderedMapTest, UnbalancedGlobalInsert)
{
  typedef int                                  key_t;
  typedef double                               mapped_t;
  typedef dash::UnorderedMap<key_t, mapped_t>  map_t;
  typedef typename map_t::iterator             map_iterator;
  typedef typename map_t::value_type           map_value;
  typedef typename map_t::size_type            size_type;

  // Use small local buffer size to enforce reallocation:
  size_type init_global_size  = 0;
  size_type local_buffer_size = 2;
  map_t map(init_global_size, local_buffer_size);
  DASH_LOG_DEBUG("UnorderedMapTest.UnbalancedGlobalInsert",
                 "map initialized");
  EXPECT_EQ_U(0, map.size());
  EXPECT_EQ_U(0, map.lsize());
  // Minumum local capacity is local buffer size:
  EXPECT_EQ_U(local_buffer_size, map.lcapacity());

  // key-value pair to be inserted:
  int unit_0_elements = 4; // two reallocs
  int unit_1_elements = 3; // one realloc
  int unit_x_elements = 2; // no realloc
  int total_elements  = unit_0_elements + unit_1_elements +
                        ((dash::size() - 2) * unit_x_elements);
  int local_elements  = unit_x_elements;

  if (_dash_id == 0) {
    local_elements = unit_0_elements;
  }
  if (_dash_id == 1) {
    local_elements = unit_1_elements;
  }

  DASH_LOG_DEBUG("UnorderedMapTest.UnbalancedGlobalInsert",
                 "insert elements");
  for (int li = 0; li < local_elements; ++li) {
    key_t     key    = 100 * (_dash_id + 1) + (li + 1);
    mapped_t  mapped = 1.0 * (_dash_id + 1) + (0.01 * (li + 1));
    map_value value({ key, mapped });
    auto insertion = map.insert(value);
    EXPECT_TRUE(insertion.second);
    auto existing  = map.insert(value);
    EXPECT_FALSE(existing.second);
    EXPECT_EQ(insertion.first, existing.first);
    map_value value_res = *insertion.first;
    DASH_LOG_DEBUG("UnorderedMapTest.UnbalancedGlobalInsert",
                   "inserted element:",
                   "iterator:", insertion.first,
                   "value:",    value_res.first, "->", value_res.second);
  }
  DASH_LOG_DEBUG("UnorderedMapTest.UnbalancedGlobalInsert",
                 "map size before commit:", map.size(),
                 "local size:",             map.lsize());
  EXPECT_EQ_U(local_elements, map.size());
  EXPECT_EQ_U(local_elements, map.lsize());

  DASH_LOG_DEBUG("UnorderedMapTest.UnbalancedGlobalInsert", "commit");
  map.barrier();

  DASH_LOG_DEBUG("UnorderedMapTest.UnbalancedGlobalInsert",
                 "map size after commit:", map.size(),
                 "local size:",            map.lsize());
  EXPECT_EQ_U(total_elements, map.size());
  EXPECT_EQ_U(local_elements, map.lsize());

  DASH_LOG_TRACE("UnorderedMapTest.UnbalancedGlobalInsert",
                 "validating global elements after commit");
  int gidx = 0;
  int unit = 0;
  for (auto git = map.begin(); git != map.end(); ++git) {
    int      lidx;
    int      nlocal;
    if (unit == 0) {
      lidx   = gidx;
      nlocal = unit_0_elements;
    }
    else if (unit == 1) {
      lidx   = gidx - unit_0_elements;
      nlocal = unit_1_elements;
    }
    else if (unit > 1) {
      lidx   = gidx - unit_0_elements - unit_1_elements -
                      ((unit - 2) * unit_x_elements);
      nlocal = unit_x_elements;
    }
    if (lidx == nlocal) {
      unit++;
      lidx = 0;
    }
    key_t    key    = 100 * (unit + 1) + (lidx + 1);
    mapped_t mapped = 1.0 * (unit + 1) + (0.01 * (lidx + 1));
    map_value expect({ key, mapped });
    map_value actual = *git;
    DASH_LOG_TRACE("UnorderedMapTest.UnbalancedGlobalInsert",
                   "after commit:",
                   "gidx:",  gidx,
                   "unit:",  unit,
                   "lidx:",  lidx,
                   "git:",   git,
                   "value:", actual.first, "->", actual.second);
    EXPECT_EQ_U(expect, actual);
    ++gidx;
  }
}
