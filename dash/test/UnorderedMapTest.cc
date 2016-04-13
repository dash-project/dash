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

  map_t map;
  EXPECT_EQ_U(0, map.size());
  EXPECT_EQ_U(0, map.lsize());

  // key-value pair to be inserted:
  key_t     key    = 100 + _dash_id;
  mapped_t  mapped = 1.0 + 0.1 * _dash_id;
  map_value value({ key, mapped });

  map.barrier();
  DASH_LOG_DEBUG("UnorderedMapTest.Initialization", "map initialized");

  if (_dash_id == 0) {
    DASH_LOG_DEBUG("UnorderedMapTest.Initialization", "insert element");
    auto insertion     = map.insert(value);
    bool elem_inserted = insertion.second;
    EXPECT_TRUE_U(elem_inserted);
  }

  DASH_LOG_DEBUG("UnorderedMapTest.Initialization", "committing elements");
  map.barrier();

  if (_dash_id == 0) {
    DASH_LOG_DEBUG("UnorderedMapTest.Initialization",
                   "validate inserted element");
    auto      elem_git = map.begin();
    map_value actual   = static_cast<map_value>(*elem_git);
    DASH_LOG_DEBUG("UnorderedMapTest.Initialization",
                   "key:",    actual.first,
                   "mapped:", actual.second);
    EXPECT_EQ_U(actual, value);
  }
  map.barrier();
}

TEST_F(UnorderedMapTest, BalancedMultiInsert)
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

  DASH_LOG_DEBUG("UnorderedMapTest.MultiInsert", "map initialized");

  DASH_LOG_DEBUG("UnorderedMapTest.MultiInsert", "insert element");
  auto insertion_a = map.insert(value_a);
  EXPECT_TRUE_U(insertion_a.second);
  auto existing_a  = map.insert(value_a);
  EXPECT_FALSE_U(existing_a.second);
  auto insertion_b = map.insert(value_b);
  EXPECT_TRUE_U(insertion_a.second);
  auto existing_b  = map.insert(value_b);
  EXPECT_FALSE_U(existing_b.second);

  DASH_LOG_DEBUG("UnorderedMapTest.MultiInsert", "committing elements");
  map.barrier();

  DASH_LOG_DEBUG("UnorderedMapTest.MultiInsert", "map size:", map.size());
  EXPECT_EQ_U(elem_per_unit, map.lsize());
  EXPECT_EQ_U(dash::size() * elem_per_unit, map.size());

  DASH_LOG_TRACE("UnorderedMapTest.MultiInsert", "map globmem buckets:");
  auto gmem_buckets = map.globmem().local_buckets();
  for (auto bit = gmem_buckets.begin(); bit != gmem_buckets.end(); ++bit) {
    auto local_bucket = *bit;
    DASH_LOG_TRACE_VAR("UnorderedMapTest.MultiInsert", local_bucket.size);
    DASH_LOG_TRACE_VAR("UnorderedMapTest.MultiInsert", local_bucket.attached);
    DASH_LOG_TRACE_VAR("UnorderedMapTest.MultiInsert", local_bucket.gptr);
    DASH_LOG_TRACE_VAR("UnorderedMapTest.MultiInsert", local_bucket.lptr);
  }
  dash::barrier();

  if (dash::myid() == 0) {
    DASH_LOG_DEBUG("UnorderedMapTest.MultiInsert", "validate elements");
    EXPECT_EQ_U(map.size(), map.end() - map.begin());
    int gidx = 0;
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
      DASH_LOG_DEBUG("UnorderedMapTest.MultiInsert",
                     "gidx:",  gidx,
                     "unit:",  unit,
                     "git:",   git,
                     "value:", actual.first,
                     "->",     actual.second);
      EXPECT_EQ_U(expect, actual);
      gidx++;
    }
  }
}

TEST_F(UnorderedMapTest, UnbalancedMultiInsert)
{
}
