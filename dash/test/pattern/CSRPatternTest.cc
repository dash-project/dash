
#include "CSRPatternTest.h"

#include <dash/Array.h>
#include <dash/pattern/CSRPattern.h>
#include <dash/algorithm/Copy.h>

TEST_F(CSRPatternTest, InitArray) {
  using pattern_t = dash::CSRPattern<1>;
  using extent_t  = pattern_t::size_type;
  using index_t   = pattern_t::index_type;

  typedef int value_t;

  auto   myid   = dash::myid();
  auto & team   = dash::Team::All();
  auto   nunits = team.size();

  std::vector<extent_t> local_sizes;

  extent_t tmp;
  extent_t sum = 0;

  for (size_t unit_idx = 0; unit_idx < nunits; ++unit_idx) {
    tmp = (unit_idx + 2) * 4;
    local_sizes.push_back(tmp);
    sum += tmp;
  }
  auto max_local_size = local_sizes.back();

  DASH_LOG_DEBUG_VAR("CSRPatternTest.InitArray", local_sizes);

  pattern_t pattern(local_sizes);
  dash::Array<value_t, index_t, pattern_t> array(pattern);

  EXPECT_EQ_U(local_sizes[myid], array.lsize());
  EXPECT_EQ_U(local_sizes[myid], array.lend() - array.lbegin());
  EXPECT_EQ_U(sum,               array.size());
  EXPECT_EQ_U(pattern.size(),    array.size());
  EXPECT_EQ_U(max_local_size,    array.lcapacity());

  DASH_LOG_DEBUG_VAR("CSRPatternTest.InitArray", array.lcapacity());
  DASH_LOG_DEBUG_VAR("CSRPatternTest.InitArray", array.lbegin());
  DASH_LOG_DEBUG_VAR("CSRPatternTest.InitArray",
                       reinterpret_cast<size_t>(array.lbegin()) % 64);

  DASH_LOG_DEBUG("CSRPatternTest.InitArray", "init local values (lidx)");
  for (index_t li = 0; li < array.lsize(); li++) {
    array.local[li] = 100 + myid.id;
  }

  DASH_LOG_DEBUG("CSRPatternTest.InitArray", "verify local values");
  for (value_t *i = array.lbegin(); i != array.lend(); ++i) {
    EXPECT_EQ_U(*i, 100 + myid.id);
  }

  DASH_LOG_DEBUG("CSRPatternTest.InitArray", "init local values (*lp)");
  for (value_t *i = array.lbegin(); i != array.lend(); ++i) {
    *i = myid.id;
  }

  DASH_LOG_DEBUG("CSRPatternTest.InitArray", "verify local values");
  for (value_t *i = array.lbegin(); i != array.lend(); ++i) {
    EXPECT_EQ_U(*i, myid.id);
  }
}

TEST_F(CSRPatternTest, CopyGlobalToLocal) {

  using pattern_t = dash::CSRPattern<1>;
  using extent_t  = pattern_t::size_type;
  using index_t   = pattern_t::index_type;

  typedef int value_t;

  auto   myid   = dash::myid();
  auto & team   = dash::Team::All();
  auto   nunits = team.size();

  std::vector<extent_t> local_sizes;

  extent_t tmp;
  extent_t sum = 0;

  for (size_t unit_idx = 0; unit_idx < nunits; ++unit_idx) {
    tmp = (unit_idx + 2) * 4;
    local_sizes.push_back(tmp);
    sum += tmp;
  }
  auto max_local_size = local_sizes.back();

  DASH_LOG_DEBUG_VAR("CSRPatternTest.InitArray", local_sizes);

  pattern_t pattern(local_sizes);
  dash::Array<value_t, index_t, pattern_t> array(pattern);

  value_t cnt = 0;
  for (auto it = array.lbegin(); it != array.lend(); ++it) {
    *it = dash::myid()*1000 + cnt++;
  }

  dash::barrier();

  if (dash::myid() == 0) {
    value_t *buf = new value_t[sum];
    dash::copy(array.begin(), array.end(), buf);

    size_t idx = 0;
    for (auto uid = 0; uid < dash::size(); ++uid) {
      for (auto pos = 0; pos < local_sizes[uid]; ++pos) {
        ASSERT_EQ_U(uid*1000+pos, buf[idx]);
        ++idx;
      }
    }

    delete[] buf;
  }
  dash::barrier();
}
