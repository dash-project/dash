
#include "CSRPatternTest.h"

#include <dash/Array.h>
#include <dash/pattern/CSRPattern.h>

TEST_F(CSRPatternTest, InitArray) {
  using pattern_t = dash::CSRPattern<1>;
  using extent_t  = pattern_t::size_type;
  using index_t   = pattern_t::index_type;

  typedef size_t value_t;

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

  DASH_LOG_DEBUG_VAR("CSRPatternTest.InitArray", local_sizes);

  pattern_t pattern(local_sizes);
  dash::Array<value_t, index_t, pattern_t> array(pattern);

  EXPECT_EQ_U(local_sizes[myid], array.lsize());
  EXPECT_EQ_U(local_sizes[myid], array.lend() - array.lbegin());
  EXPECT_EQ_U(sum, array.size());
  EXPECT_EQ_U(pattern.size(), array.size());

  for (value_t *i = array.lbegin(); i != array.lend(); ++i) {
    *i = myid;
  }

  for (value_t *i = array.lbegin(); i != array.lend(); ++i) {
    EXPECT_EQ_U(*i, myid.id);
  }
}
