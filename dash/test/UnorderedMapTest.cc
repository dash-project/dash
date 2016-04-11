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
                   "got key:", actual.first, "mapped:", actual.second);
    EXPECT_EQ_U(actual, value);
  }

  map.barrier();
}
