
#include "ViewTest.h"

#include <dash/Types.h>
#include <dash/Array.h>
#include <dash/View.h>
#include <dash/Meta.h>

#include <dash/internal/StreamConversion.h>

#include <array>
#include <algorithm>
#include <sstream>
#include <string>
#include <iomanip>


namespace dash {
namespace test {

  class seq_pos_t {
  public:
    int unit;
    int lindex;
    int lblock;
    int gindex;
    int marker;

    constexpr bool operator==(const seq_pos_t & rhs) const {
      return &rhs == this ||
             ( unit   == rhs.unit   &&
               lindex == rhs.lindex &&
               lblock == rhs.lblock &&
               gindex == rhs.gindex &&
               marker == rhs.marker );
    }
  };

  std::ostream & operator<<(std::ostream & os, const seq_pos_t & sp) {
    std::ostringstream ss;
    if (sp.marker != 0) {
      ss << "<" << sp.marker << "> ";
    }
    ss << "u" << sp.unit
       << "b" << sp.lblock
       << "l" << sp.lindex;
    return operator<<(os, ss.str());
  }

  template <class ArrayT>
  auto initialize_array(ArrayT & array)
  -> typename std::enable_if<
                std::is_floating_point<typename ArrayT::value_type>::value,
                void >::type
  {
    auto block_size = array.pattern().blocksize(0);
    for (auto li = 0; li != array.local.size(); ++li) {
      auto block_lidx = li / block_size;
      auto block_gidx = (block_lidx * dash::size()) + dash::myid().id;
      auto gi         = (block_gidx * block_size) + (li % block_size);
      array.local[li] = // unit
                        (1.0000 * dash::myid().id) +
                        // local offset
                        (0.0001 * (li+1)) +
                        // global offset
                        (0.0100 * gi);
    }
    array.barrier();
  }

  template <class ArrayT>
  auto initialize_array(ArrayT & array)
  -> typename std::enable_if<
                std::is_same<typename ArrayT::value_type, seq_pos_t>::value,
                void >::type
  {
    auto block_size = array.pattern().blocksize(0);
    for (auto li = 0; li != array.local.size(); ++li) {
      auto block_lidx = li / block_size;
      auto block_gidx = (block_lidx * dash::size()) + dash::myid().id;
      auto gi         = (block_gidx * block_size) + (li % block_size);
      seq_pos_t val {
                      static_cast<int>(dash::myid().id), // unit
                      static_cast<int>(li),              // local offset
                      static_cast<int>(block_lidx),      // local block index
                      static_cast<int>(gi),              // global offset
                      0                                  // marker
                    };
      array.local[li] = val;
    }
    array.barrier();
    DASH_LOG_TRACE("ViewTest.initialize_array", "Array initialized");
  }

}
}

using dash::test::range_str;
using dash::test::seq_pos_t;


TEST_F(ViewTest, ViewTraits)
{
  dash::Array<int> array(dash::size() * 10);
  auto v_sub  = dash::sub(0, 10, array);
  auto i_sub  = dash::index(v_sub);
  auto v_ssub = dash::sub(0, 5, (dash::sub(0, 10, array)));
  auto v_loc  = dash::local(array);
  auto v_lsub = dash::local(dash::sub(0, 10, array));
  auto v_bsub = *dash::begin(dash::blocks(v_sub));

  static_assert(
      dash::view_traits<decltype(array)>::is_local::value == false,
      "view traits is_local for dash::Array not matched");
  static_assert(
      dash::view_traits<decltype(array)>::is_view::value == false,
      "view traits is_view for dash::Array not matched");
  static_assert(
      dash::view_traits<decltype(v_ssub)>::is_view::value == true,
      "view traits is_view for sub(sub(dash::Array)) not matched");
  static_assert(
      dash::view_traits<decltype(v_lsub)>::is_view::value == true,
      "view traits is_view for local(sub(dash::Array)) not matched");

  // Local container proxy types are not considered views as they do
  // not specify an index set:
  static_assert(
      dash::view_traits<decltype(v_loc)>::is_view::value == false,
      "view traits is_view for local(dash::Array) not matched");
  static_assert(
      dash::view_traits<decltype(i_sub)>::is_view::value == false,
      "view traits is_view for index(sub(dash::Array)) not matched");
  static_assert(
      dash::view_traits<decltype(v_bsub)>::is_view::value == true,
      "view traits is_view for begin(blocks(dash::Array)) not matched");

  static_assert(
      dash::view_traits<decltype(array)>::is_origin::value == true,
      "view traits is_origin for dash::Array not matched");
  static_assert(
      dash::view_traits<decltype(v_sub)>::is_origin::value == false,
      "view traits is_origin for sub(dash::Array) not matched");
  static_assert(
      dash::view_traits<decltype(i_sub)>::is_origin::value == true,
      "view traits is_origin for index(sub(dash::Array)) not matched");
  static_assert(
      dash::view_traits<decltype(v_bsub)>::is_origin::value == false,
      "view traits is_origin for begin(blocks(sub(dash::Array))) "
      "not matched");
  static_assert(
      dash::view_traits<decltype(v_ssub)>::is_origin::value == false,
      "view traits is_origin for sub(sub(dash::Array)) not matched");
  static_assert(
      dash::view_traits<decltype(v_loc)>::is_origin::value == true,
      "view traits is_origin for local(dash::Array) not matched");
  static_assert(
      dash::view_traits<decltype(v_lsub)>::is_origin::value == false,
      "view traits is_local for local(sub(dash::Array)) not matched");

  static_assert(
      dash::view_traits<decltype(v_loc)>::is_local::value == true,
      "view traits is_local for local(dash::Array) not matched");
  static_assert(
      dash::view_traits<decltype(v_lsub)>::is_local::value == true,
      "view traits is_local for local(sub(dash::Array)) not matched");

  static_assert(
       dash::view_traits<decltype(array)>::rank::value == 1,
       "rank of array different from 1");
  static_assert(
       dash::view_traits<decltype(v_sub)>::rank::value == 1,
       "rank of sub(array) different from 1");
  static_assert(
       dash::view_traits<decltype(v_ssub)>::rank::value == 1,
       "rank of sub(sub(array)) different from 1");
  static_assert(
       dash::view_traits<decltype(v_loc)>::rank::value == 1,
       "rank of local(array) different from 1");
}

TEST_F(ViewTest, NestedTemporaries)
{
  typedef float value_t;

  int block_size       = 15;
  int array_size       = dash::size() * block_size;

  dash::Array<value_t> a(array_size);
  dash::test::initialize_array(a);

  if (dash::myid() != 0) {
    return;
  }

  DASH_LOG_DEBUG_VAR("ViewTest.NestedTemporaries",
                     range_str(a));

  auto gview_sub      = dash::sub(1, array_size - 2, a);
  DASH_LOG_DEBUG_VAR("ViewTest.NestedTemporaries", range_str(gview_sub));

  auto gview_ssub     = dash::sub(1, array_size - 3,
                         dash::sub(1, array_size - 2, a));
  DASH_LOG_DEBUG_VAR("ViewTest.NestedTemporaries", range_str(gview_ssub));

  auto gview_lref     = dash::sub(1, array_size - 5,
                         gview_ssub);
  DASH_LOG_DEBUG_VAR("ViewTest.NestedTemporaries", range_str(gview_lref));

  auto gview_temp     = dash::sub(1, array_size - 5,
                          dash::sub(1, array_size - 3,
                            dash::sub(1, array_size - 2, a)));
  DASH_LOG_DEBUG_VAR("ViewTest.NestedTemporaries", range_str(gview_temp));

  EXPECT_EQ(a.size() - 3 - 3,
            gview_lref.size());
  EXPECT_EQ(gview_temp.size(), gview_lref.size());

  int v_idx = 0;
  for (const auto & view_elem : gview_temp) {
    EXPECT_EQ(static_cast<value_t>(a[v_idx + 3]),
              static_cast<value_t>(view_elem));
    ++v_idx;
  }
}

TEST_F(ViewTest, ArrayBlockedPatternGlobalView)
{
  int block_size       = 3;
  int array_size       = dash::size() * block_size;
  int block_begin_gidx = block_size * dash::myid();
  int block_end_gidx   = block_size * (dash::myid() + 1);

  dash::Array<float> a(array_size);
  dash::test::initialize_array(a);

  if (dash::myid() == 0) {
    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternGlobalView",
                       range_str(a));
  }
  a.barrier();

  // View to global index range of local block:
  auto block_gview = dash::sub(block_begin_gidx,
                               block_end_gidx,
                               a);
  EXPECT_EQ(block_size, block_gview.size());

  DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternGlobalView",
                     range_str(block_gview));
  DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternGlobalView",
                     block_gview.begin());

  EXPECT_TRUE_U(std::equal(a.begin() + block_begin_gidx,
                           a.begin() + block_end_gidx,
                           block_gview.begin()));

  // Origin of block view is array:
  auto & block_domain = dash::domain(block_gview);

  EXPECT_EQ(array_size, block_domain.size());
  EXPECT_EQ(a.begin(),  dash::begin(block_domain));
  EXPECT_EQ(a.end(),    dash::end(block_domain));

  auto view_begin_gidx = dash::index(dash::begin(block_gview));
  auto view_end_gidx   = dash::index(dash::end(block_gview));

  EXPECT_EQ(block_begin_gidx, view_begin_gidx);
  EXPECT_EQ(block_end_gidx,   view_end_gidx);
}

TEST_F(ViewTest, ArrayBlockedPatternChainedGlobalView)
{
  int block_size       = 7;
  int array_size       = dash::size() * block_size;
  int block_begin_gidx = block_size * dash::myid();
  int block_end_gidx   = block_size * (dash::myid() + 1);

  dash::Array<float> a(array_size);
  dash::test::initialize_array(a);

  if (dash::myid() == 0) {
    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternChainedGlobalView",
                       range_str(a));
  }
  a.barrier();

  // View to global index range of local block:
  auto l_block_gview = dash::sub(block_begin_gidx,
                                 block_end_gidx,
                                 a);
  DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternChainedGlobalView",
                     range_str(l_block_gview));
  EXPECT_TRUE_U(
    std::equal(l_block_gview.begin(),
               l_block_gview.end(),
               a.local.begin()));

  // View to global index range spanning over local block:
  int block_outer_begin_gidx = (dash::myid() == 0
                                ? block_begin_gidx
                                : block_begin_gidx - 2);
  int block_outer_end_gidx   = (dash::myid() == dash::size() - 1
                                ? block_end_gidx
                                : block_end_gidx + 2);
  auto block_gview_outer     = dash::sub(block_outer_begin_gidx,
                                         block_outer_end_gidx,
                                         a);
  DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternChainedGlobalView",
                     range_str(block_gview_outer));

  EXPECT_EQ_U(block_outer_end_gidx - block_outer_begin_gidx,
              block_gview_outer.size());

  // Sub-range in block from block index 2 to -2:
  auto block_gview_inner = dash::sub(2,
                                     block_size - 2, 
                                     l_block_gview);
  DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternChainedGlobalView",
                     range_str(block_gview_inner));

  EXPECT_EQ(block_size - 4,  block_gview_inner.size());
  EXPECT_EQ(block_begin_gidx + 2,
            dash::index(dash::begin(block_gview_inner)));
  EXPECT_EQ(block_begin_gidx + block_size - 2,
            dash::index(dash::end(block_gview_inner)));

  // Origin of inner view is outer view:
  auto & block_gview_inner_domain = dash::domain(block_gview_inner);
  EXPECT_TRUE_U(
    std::equal(l_block_gview.begin(),
               l_block_gview.end(),
               block_gview_inner_domain.begin()));

  // Origin of outer view is array:
  auto & block_gview_outer_domain = dash::domain(block_gview_outer);
  EXPECT_EQ(a.begin(),  dash::begin(block_gview_outer_domain));
  EXPECT_EQ(a.end(),    dash::end(block_gview_outer_domain));
}

TEST_F(ViewTest, ArrayBlockCyclicPatternGlobalView)
{
  int block_size       = 5;
  int blocks_per_unit  = 3;
  int array_size       = dash::size() * block_size * blocks_per_unit
                         + (block_size * 2)
                         - 2;
  int block_begin_gidx = block_size * dash::myid();
  int block_end_gidx   = block_size * (dash::myid() + 1);

  dash::Array<float> a(array_size, dash::BLOCKCYCLIC(block_size));
  dash::test::initialize_array(a);

  if (dash::myid() == 0) {
    auto blocks_view = dash::blocks(dash::sub(0, a.size(), a));
    int  b_idx      = 0;
    for (auto block : blocks_view) {
      DASH_LOG_DEBUG("ViewTest.ArrayBlockCyclicPatternGlobalView",
                     "a.block[", b_idx, "]:", range_str(block));
      ++b_idx;
    }
  }
  a.barrier();

  // View to global index range of local block:
  auto block_gview = dash::sub(block_begin_gidx,
                               block_end_gidx,
                               a);

  EXPECT_EQ(block_size, block_gview.size());

  // Origin of block view is array:
  auto & block_domain = dash::domain(block_gview);
  EXPECT_EQ(a.begin(),  dash::begin(block_domain));
  EXPECT_EQ(a.end(),    dash::end(block_domain));

  // --- blocks(sub(array)) ----------------------------------------------
  //
  if (dash::myid() == 0) {
    auto sub_begin_gidx  = block_size / 2;
    auto sub_end_gidx    = a.size() - (block_size / 2);
    auto sub_view        = dash::sub(sub_begin_gidx, sub_end_gidx, a);
   
    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockCyclicPatternGlobalView",
                       range_str(sub_view));

    auto blocks_sub_view = dash::blocks(
                             dash::sub(
                               sub_begin_gidx,
                               sub_end_gidx,
                               a));
    int b_idx      = 0;
    int begin_idx  = sub_begin_gidx;
    int num_blocks = a.pattern().blockspec().size();
    for (auto block : blocks_sub_view) {
      DASH_LOG_DEBUG("ViewTest.ArrayBlockCyclicPatternGlobalView",
                     "a.sub.block[", b_idx, "]:", range_str(block));
      int exp_block_size = block_size;
      if (b_idx == 0) {
        exp_block_size -= (block_size / 2);     // 5 - 2   = 3
      } else if (b_idx == num_blocks - 1) {
        exp_block_size -= 2 * (block_size / 2); // 5 - 2*2 = 1
      }

      EXPECT_EQ(exp_block_size, block.size());
      EXPECT_EQ(exp_block_size, block.end() - block.begin());
      EXPECT_TRUE(std::equal(a.begin() + begin_idx,
                             a.begin() + begin_idx + exp_block_size,
                             block.begin()));

      begin_idx += exp_block_size;
      ++b_idx;
    }
    EXPECT_EQ(num_blocks, b_idx);
  }
}

TEST_F(ViewTest, ArrayBlockCyclicPatternLocalSub)
{
  int block_size       = 4;
  // minimum number of blocks per unit:
  int blocks_per_unit  = 2;
  // two extra blocks, last block underfilled:
  int array_size       = dash::size() * block_size * blocks_per_unit
                         + (block_size * 2)
                         - 2;
  int num_blocks       = (dash::size() * blocks_per_unit)
                         + 2;
  int num_local_blocks = dash::size() == 1
                         ? num_blocks
                         : ( dash::myid() < 2
                             ? blocks_per_unit + 1
                             : blocks_per_unit );

  dash::Array<float> a(array_size, dash::BLOCKCYCLIC(block_size));
  dash::test::initialize_array(a);

  DASH_LOG_DEBUG("ViewTest.ArrayBlockCyclicPatternLocalSub",
                 "array:", range_str(a));
  DASH_LOG_DEBUG("ViewTest.ArrayBlockCyclicPatternLocalSub",
                 "local(array):", range_str(dash::local(a)));

  // sub(local(array))
  //
  {
    auto l_begin  = block_size / 2;
    auto l_end    = a.lsize() - (block_size / 2);
    DASH_LOG_DEBUG("ViewTest.ArrayBlockCyclicPatternLocalSub", "==",
                   "sub(", l_begin, ",", l_end, ", local(array))");

    auto s_l_view = dash::sub(
                      block_size / 2,
                      a.lsize() - (block_size / 2),
                      dash::local(
                        a));
    DASH_LOG_DEBUG("ViewTest.ArrayBlockCyclicPatternLocalSub",
                   "lbegin:", l_begin, "lend:", l_end);
    DASH_LOG_DEBUG("ViewTest.ArrayBlockCyclicPatternLocalSub",
                   range_str(s_l_view));

    EXPECT_EQ_U(l_end - l_begin, s_l_view.size());
    EXPECT_TRUE_U(std::equal(a.lbegin() + l_begin,
                             a.lbegin() + l_end,
                             s_l_view.begin()));
  }
  a.barrier();

  // local(sub(array))
  //
  {
    auto l_begin  = 0;
    auto l_end    = a.lsize();
    if (a.pattern().unit_at(0) == dash::myid().id) {
      l_begin += block_size / 2;
    }
    if (a.pattern().unit_at(a.size() - 1) == dash::myid().id) {
      l_end   -= block_size / 2;
    }

    DASH_LOG_DEBUG("ViewTest.ArrayBlockCyclicPatternLocalSub", "==",
                   "local(sub(",
                   (block_size / 2), ",", (a.size() - (block_size / 2)),
                   ", array))");
    auto l_s_view = dash::local(
                      dash::sub(
                        block_size / 2,
                        a.size() - (block_size / 2),
                        a));
    DASH_LOG_DEBUG("ViewTest.ArrayBlockCyclicPatternLocalSub",
                   "lbegin:", l_begin, "lend:", l_end);
    DASH_LOG_DEBUG("ViewTest.ArrayBlockCyclicPatternLocalSub",
                   range_str(l_s_view));

    EXPECT_EQ_U(l_end - l_begin, l_s_view.size());
    EXPECT_TRUE_U(std::equal(a.lbegin() + l_begin,
                             a.lbegin() + l_end,
                             l_s_view.begin()));
  }
  a.barrier();

  // sub(local(sub(array)))
  //
  {
    auto l_begin  = 0;
    auto l_end    = a.lsize();
    if (a.pattern().unit_at(0) == dash::myid().id) {
      l_begin += block_size / 2;
    }
    if (a.pattern().unit_at(a.size() - 1) == dash::myid().id) {
      l_end   -= block_size / 2;
    }
    l_begin      += 1;
    l_end        -= 1;

    auto l_s_view   = dash::local(
                        dash::sub(
                          block_size / 2,
                          a.size() - (block_size / 2),
                          a));
    DASH_LOG_DEBUG("ViewTest.ArrayBlockCyclicPatternLocalSub", "==",
                   "sub(", 1, ",", l_s_view.size() - 1,
                   ", local(sub(",
                   (block_size / 2), ",", (a.size() - (block_size / 2)),
                   ", array)))");

    auto s_l_s_view = dash::sub(
                        1,
                        l_s_view.size() - 1,
                        l_s_view);
    DASH_LOG_DEBUG("ViewTest.ArrayBlockCyclicPatternLocalSub",
                   "lbegin:", l_begin, "lend:", l_end);
    DASH_LOG_DEBUG("ViewTest.ArrayBlockCyclicPatternLocalSub",
                   range_str(s_l_s_view));

    EXPECT_EQ_U(l_end - l_begin, s_l_s_view.size());
    EXPECT_TRUE_U(std::equal(a.lbegin() + l_begin,
                             a.lbegin() + l_end,
                             s_l_s_view.begin()));
  }
  a.barrier();
}

TEST_F(ViewTest, ArrayBlockCyclicPatternLocalBlocks)
{
  int block_size       = 5;
  // minimum number of blocks per unit:
  int blocks_per_unit  = 3;
  // two extra blocks, last block underfilled:
  int array_size       = dash::size() * block_size * blocks_per_unit
                         + (block_size * 2)
                         - 2;
  int num_blocks       = (dash::size() * blocks_per_unit)
                         + 2;
  int num_local_blocks = dash::size() == 1
                         ? num_blocks
                         : ( dash::myid() < 2
                             ? blocks_per_unit + 1
                             : blocks_per_unit );

  dash::Array<float> a(array_size, dash::BLOCKCYCLIC(block_size));
  dash::test::initialize_array(a);

  // local(blocks(array))
  //
  {
    int  l_b_idx;
    int  l_idx;

    auto blocks_view   = dash::blocks(a);
    if (dash::myid() == 0) {
      for (auto block : blocks_view) {
        DASH_LOG_DEBUG("ViewTest.ArrayBlockCyclicPatternLocalBlocks", "----",
                       "blocks_view", range_str(block));
      }
    }
    a.barrier();

    EXPECT_EQ_U(num_blocks, blocks_view.size());

    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockCyclicPatternLocalBlocks",
                       dash::typestr(blocks_view));
    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockCyclicPatternLocalBlocks",
                       dash::typestr(blocks_view.begin()));
    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockCyclicPatternLocalBlocks",
                       blocks_view.size());

    auto l_blocks_view = dash::local(
                           dash::blocks(a) );
    EXPECT_EQ_U(num_local_blocks, l_blocks_view.size());

    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockCyclicPatternLocalBlocks",
                       dash::typestr(l_blocks_view));
    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockCyclicPatternLocalBlocks",
                       l_blocks_view.size());
    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockCyclicPatternLocalBlocks",
                       dash::typestr(l_blocks_view.begin()));

    l_b_idx = 0;
    l_idx   = 0;
    for (const auto & l_block : l_blocks_view) {
      DASH_LOG_DEBUG("ViewTest.ArrayBlockCyclicPatternLocalBlocks",
                     "l_block[", l_b_idx, "]:", range_str(l_block));
      EXPECT_TRUE_U(std::equal(a.lbegin() + l_idx,
                               a.lbegin() + l_idx + l_block.size(),
                               l_block.begin()));
      ++l_b_idx;
      l_idx += l_block.size();
    }
    EXPECT_EQ_U(l_idx, a.lsize());
  }
  a.barrier();

  // blocks(local(array))
  //
  {
    int  l_b_idx = 0;
    int  l_idx   = 0;

    auto blocks_l_view = dash::blocks(
                           dash::local(a));
    for (const auto & block_l : blocks_l_view) {
      DASH_LOG_DEBUG("ViewTest.ArrayBlockCyclicPatternLocalBlocks",
                     "block_l[", l_b_idx, "]:", range_str(block_l));
      EXPECT_TRUE_U(std::equal(a.lbegin() + l_idx,
                               a.lbegin() + l_idx + block_l.size(),
                               block_l.begin()));
      ++l_b_idx;
      l_idx += block_l.size();
    }
    EXPECT_EQ_U(l_idx, a.lsize());
  }
}

TEST_F(ViewTest, ArrayBlockCyclicPatternSubLocalBlocks)
{
  int block_size       = 5;
  // minimum number of blocks per unit:
  int blocks_per_unit  = 2;
  // two extra blocks, last block underfilled:
  int array_size       = dash::size() * block_size * blocks_per_unit
                         + (block_size * 2)
                         - 2;
  int num_blocks       = (dash::size() * blocks_per_unit)
                         + 2;
  int num_local_blocks = dash::size() == 1
                         ? num_blocks
                         : ( dash::myid() < 2
                             ? blocks_per_unit + 1
                             : blocks_per_unit );

  dash::Array<float> a(array_size, dash::BLOCKCYCLIC(block_size));
  dash::test::initialize_array(a);

  // local(blocks(array))
  //
  {
    auto l_blocks_view = dash::local(
                           dash::blocks(
                             a));

    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockCyclicPatternSubLocalBlocks",
                       dash::typestr(l_blocks_view));
    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockCyclicPatternSubLocalBlocks",
                       l_blocks_view.size());

    EXPECT_EQ_U(num_local_blocks, l_blocks_view.size());

    int l_b_idx = 0;
    int l_idx   = 0;
    for (const auto & l_block : l_blocks_view) {
      DASH_LOG_DEBUG("ViewTest.ArrayBlockCyclicPatternSubLocalBlocks",
                     "l_block[", l_b_idx, "]:", range_str(l_block));

      EXPECT_TRUE_U(std::equal(a.local.begin() + l_idx,
                               a.local.begin() + l_idx + l_block.size(),
                               l_block.begin()));

      ++l_b_idx;
      l_idx += l_block.size();
    }
    EXPECT_EQ_U(a.lsize(), l_idx);
  }
  a.barrier();

  if (dash::myid() == 0) {
    auto sub_view = dash::sub(
                      block_size / 2,
                      a.size() - (block_size / 2),
                      a);
    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockCyclicPatternSubLocalBlocks",
                       range_str(sub_view));
  }
  a.barrier();

  // local(sub(array))
  //
  {
    auto l_sub_view = dash::local(
                        dash::sub(
                          block_size / 2,
                          a.size() - (block_size / 2),
                          a));

    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockCyclicPatternSubLocalBlocks",
                       dash::typestr(l_sub_view));
    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockCyclicPatternSubLocalBlocks",
                       l_sub_view.size());
    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockCyclicPatternSubLocalBlocks",
                       range_str(l_sub_view));
    int g_idx;
    int l_idx = 0;
    for (g_idx  = block_size / 2;
         g_idx != a.size() - (block_size / 2);
         ++g_idx) {
      float * lp = (a.begin() + g_idx).local();
      if (l_idx < l_sub_view.size() && lp != nullptr) {
        EXPECT_EQ_U(*lp, static_cast<float>(l_sub_view[l_idx]));
        ++l_idx;
      }
    }
    int exp_l_idx = a.lsize();
    if (dash::myid().id == a.pattern().unit_at(0)) {
      // Owner of first global block:
      exp_l_idx -= (block_size / 2);
    }
    if (dash::myid().id == a.pattern().unit_at(a.size() - 1)) {
      // Owner of last global block:
      exp_l_idx -= (block_size / 2);
    }
    EXPECT_EQ_U(exp_l_idx, l_idx);
  }
  a.barrier();

  // local(blocks(sub(array)))
  //
  {
    int  l_b_idx;
    int  l_idx;
    auto sub_view          = dash::sub(
                               block_size / 2,
                               a.size() - (block_size / 2),
                               a);
    auto blocks_sub_view   = dash::blocks(
                               dash::sub(
                                 block_size / 2,
                                 a.size() - (block_size / 2),
                                 a));
    if (dash::myid() == 0) {
      DASH_LOG_DEBUG("ViewTest.ArrayBlockCyclicPatternSubLocalBlocks",
                     "type(blocks_sub_view):",
                     dash::typestr(blocks_sub_view));
      DASH_LOG_DEBUG("ViewTest.ArrayBlockCyclicPatternSubLocalBlocks",
                     "type(blocks_sub_view::domain_type):",
                     dash::typestr<
                       typename decltype(blocks_sub_view)::domain_type
                     >());
      DASH_LOG_DEBUG("ViewTest.ArrayBlockCyclicPatternSubLocalBlocks",
                     "type(blocks_sub_view::local_type):",
                     dash::typestr<
                       typename decltype(blocks_sub_view)::local_type
                     >());
      DASH_LOG_DEBUG("ViewTest.ArrayBlockCyclicPatternSubLocalBlocks",
                     "type(blocks_sub_view::origin_type):",
                     dash::typestr<
                       typename decltype(blocks_sub_view)::origin_type
                     >());
      DASH_LOG_DEBUG("ViewTest.ArrayBlockCyclicPatternSubLocalBlocks",
                     "type(blocks_sub_view[0]):",
                     dash::typestr(blocks_sub_view[0]));

      DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockCyclicPatternSubLocalBlocks",
                         blocks_sub_view.size());
      DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockCyclicPatternSubLocalBlocks",
                         dash::index(blocks_sub_view).is_strided());
      int b_idx = 0;
      int idx   = 0;
      for (auto block : blocks_sub_view) {
        DASH_LOG_DEBUG("ViewTest.ArrayBlockCyclicPatternSubLocalBlocks",
                       "blocks_sub[", b_idx, "]", range_str(block));
        ++b_idx;
      }
    }
    a.barrier();

    EXPECT_EQ_U(num_blocks, blocks_sub_view.size());

    auto l_blocks_sub_view = dash::local(
                               blocks_sub_view);

    if (dash::myid() == 0) {
      DASH_LOG_DEBUG("ViewTest.ArrayBlockCyclicPatternSubLocalBlocks",
                     "type(l_blocks_sub_view):",
                     dash::typestr(l_blocks_sub_view));
      DASH_LOG_DEBUG("ViewTest.ArrayBlockCyclicPatternSubLocalBlocks",
                     "type(l_blocks_sub_view::domain_type):",
                     dash::typestr<
                       typename decltype(l_blocks_sub_view)::domain_type
                     >());
      DASH_LOG_DEBUG("ViewTest.ArrayBlockCyclicPatternSubLocalBlocks",
                     "type(l_blocks_sub_view::local_type):",
                     dash::typestr<
                       typename decltype(l_blocks_sub_view)::local_type
                     >());
      DASH_LOG_DEBUG("ViewTest.ArrayBlockCyclicPatternSubLocalBlocks",
                     "type(l_blocks_sub_view::origin_type):",
                     dash::typestr<
                       typename decltype(l_blocks_sub_view)::origin_type
                     >());
      DASH_LOG_DEBUG("ViewTest.ArrayBlockCyclicPatternSubLocalBlocks",
                     "type(l_blocks_sub_view[0]):",
                     dash::typestr(l_blocks_sub_view[0]));
      DASH_LOG_DEBUG("ViewTest.ArrayBlockCyclicPatternSubLocalBlocks",
                     "type(l_blocks_sub_view[0].origin):",
                     dash::typestr(dash::origin(
                       l_blocks_sub_view[0])));
    }

    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockCyclicPatternSubLocalBlocks",
                       l_blocks_sub_view.size());
    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockCyclicPatternSubLocalBlocks",
                       dash::index(l_blocks_sub_view).is_strided());

    EXPECT_EQ_U(num_local_blocks, l_blocks_sub_view.size());

    std::vector<float> l_blocks_sub_values;

    l_b_idx = 0;
    l_idx   = 0;
    for (const auto & l_block : l_blocks_sub_view) {
      auto l_block_index = dash::index(l_block);
      DASH_LOG_DEBUG("ViewTest.ArrayBlockCyclicPatternSubLocalBlocks",
                     "l_block_sub[", l_b_idx, "]",
                     range_str(l_block));
      EXPECT_EQ_U(dash::distance(l_block.begin(), l_block.end()),
                  l_block.size());
      EXPECT_EQ_U(l_block_index.size(), l_block.size());

      std::move(l_block.begin(), l_block.end(), std::back_inserter(l_blocks_sub_values));
      ++l_b_idx;
      l_idx += l_block.size();
    }
    DASH_LOG_DEBUG("ViewTest.ArrayBlockCyclicPatternSubLocalBlocks",
                   "l_idx:", l_idx, "l_b_idx:", l_b_idx);
    DASH_LOG_DEBUG("ViewTest.ArrayBlockCyclicPatternSubLocalBlocks",
                   "l_blocks_sub:", l_blocks_sub_values);

    EXPECT_EQ_U(dash::local(sub_view).size(),
                l_blocks_sub_values.size());
    EXPECT_TRUE_U(std::equal(l_blocks_sub_values.begin(),
                             l_blocks_sub_values.end(),
                             dash::local(sub_view).begin()));
    a.barrier();

    int exp_l_idx = a.lsize();
    if (dash::myid().id == a.pattern().unit_at(0)) {
      // Owner of first global block:
      exp_l_idx -= (block_size / 2);
    }
    if (dash::myid().id == a.pattern().unit_at(a.size() - 1)) {
      // Owner of last global block:
      exp_l_idx -= (block_size / 2);
    }
    EXPECT_EQ_U(exp_l_idx,        l_idx);
    EXPECT_EQ_U(num_local_blocks, l_b_idx);
  }
  a.barrier();
}

TEST_F(ViewTest, IndexSet)
{
  typedef float                 value_t;
  typedef dash::default_index_t index_t;

  int block_size           = 3;
  int blocks_per_unit      = 3;
  int array_size           = dash::size()
                             * (blocks_per_unit * block_size);

  dash::Array<value_t, index_t, dash::TilePattern<1>>
    array(array_size, dash::TILE(block_size));
  dash::test::initialize_array(array);

  auto sub_begin_gidx = block_size / 2;
  auto sub_end_gidx   = array_size - (block_size / 2);

  // ---- sub(array) ----------------------------------------------------
  //
  if (dash::myid() == 0) {
    std::vector<value_t> values(array.begin(), array.end());
    DASH_LOG_DEBUG_VAR("ViewTest.IndexSet", values);

    auto sub_gview = dash::sub(
                       sub_begin_gidx,
                       sub_end_gidx,
                       array);

    auto sub_index = dash::index(sub_gview);

    EXPECT_EQ_U(
        dash::distance(
          array.begin() + sub_begin_gidx,
          array.begin() + sub_end_gidx),
        dash::distance(
          sub_gview.begin(),
          sub_gview.end()) );
    EXPECT_TRUE_U(
        std::equal(
          array.begin() + sub_begin_gidx,
          array.begin() + sub_end_gidx,
          sub_gview.begin()) );

    DASH_LOG_DEBUG("ViewTest.IndexSet", "---- sub(",
                   sub_begin_gidx, ",", sub_end_gidx, ")");

    DASH_LOG_DEBUG_VAR("ViewTest.IndexSet", sub_index);
    DASH_LOG_DEBUG_VAR("ViewTest.IndexSet", sub_index.pre().first());
    DASH_LOG_DEBUG_VAR("ViewTest.IndexSet", sub_index.pre().last());

    DASH_LOG_DEBUG_VAR("ViewTest.IndexSet", range_str(sub_gview));

    EXPECT_EQ_U(array_size - (2 * (block_size / 2)), sub_gview.size());
    EXPECT_EQ_U(array_size - (2 * (block_size / 2)), sub_index.size());

    EXPECT_TRUE_U(std::equal(array.begin() + (block_size / 2),
                             array.begin() + array_size - (block_size / 2),
                             sub_gview.begin()));
  }
  array.barrier();

  // ---- local(all(array)) ---------------------------------------------
  //
  auto all_gview    = dash::sub(
                        0, array_size,
                        array);
  auto l_all_gview = dash::local(all_gview);
  auto l_all_index = dash::index(l_all_gview);

  DASH_LOG_DEBUG("ViewTest.IndexSet", "---- local(sub(",
                 0, ",", array_size, "))");

  DASH_LOG_DEBUG_VAR("ViewTest.IndexSet", l_all_index);
  DASH_LOG_DEBUG_VAR("ViewTest.IndexSet", l_all_gview);

  array.barrier();

  // ---- local(sub(array)) ---------------------------------------------
  //
  auto locsub_gview = dash::local(
                        dash::sub(
                          sub_begin_gidx,
                          sub_end_gidx,
                          array));
  auto locsub_index = dash::index(locsub_gview);

  DASH_LOG_DEBUG("ViewTest.IndexSet", "---- local(sub(",
                 sub_begin_gidx, ",", sub_end_gidx, "))");

  DASH_LOG_DEBUG_VAR("ViewTest.IndexSet", locsub_index);
  DASH_LOG_DEBUG_VAR("ViewTest.IndexSet", locsub_index.pre().first());
  DASH_LOG_DEBUG_VAR("ViewTest.IndexSet", locsub_index.pre().last());

  DASH_LOG_DEBUG_VAR("ViewTest.IndexSet", locsub_gview);

  array.barrier();

  // ---- sub(sub(array)) -----------------------------------------------
  //
  if (dash::myid() == 0) {
    auto subsub_begin_idx = 3;
    auto subsub_end_idx   = subsub_begin_idx + block_size;

    auto subsub_gview = dash::sub(
                          subsub_begin_idx,
                          subsub_end_idx,
                          dash::sub(
                            sub_begin_gidx,
                            sub_end_gidx,
                            array));
    auto subsub_index = dash::index(subsub_gview);

    DASH_LOG_DEBUG("ViewTest.IndexSet", "---- sub(sub(",
                   sub_begin_gidx,   ",", sub_end_gidx, ") ",
                   subsub_begin_idx, ",", subsub_end_idx, ")");

    auto subsub_begin_gidx = sub_begin_gidx + subsub_begin_idx;
    auto subsub_end_gidx   = sub_begin_gidx + subsub_end_idx;

    DASH_LOG_DEBUG_VAR("ViewTest.IndexSet", subsub_index);
    DASH_LOG_DEBUG_VAR("ViewTest.IndexSet", subsub_index.pre().first());
    DASH_LOG_DEBUG_VAR("ViewTest.IndexSet", subsub_index.pre().last());

    std::vector<value_t> subsub_values(subsub_gview.begin(),
                                       subsub_gview.end());
    DASH_LOG_DEBUG_VAR("ViewTest.IndexSet", subsub_values);

    EXPECT_EQ_U(
        dash::distance(
          array.begin() + subsub_begin_gidx,
          array.begin() + subsub_end_gidx),
        dash::distance(
          subsub_gview.begin(),
          subsub_gview.end()) );
    EXPECT_TRUE_U(
        std::equal(
          array.begin() + subsub_begin_gidx,
          array.begin() + subsub_end_gidx,
          subsub_gview.begin()) );
  }
}

TEST_F(ViewTest, LocalBlocksView1Dim)
{
  typedef float                 value_t;
  typedef dash::default_index_t index_t;

  int block_size           = 4;
  int blocks_per_unit      = 2;
  int array_size           = dash::size()
                             * (blocks_per_unit * block_size)
                             + (block_size * 3 / 2);

  dash::Array<value_t> array(array_size, dash::BLOCKCYCLIC(block_size));
  dash::test::initialize_array(array);

  if (dash::myid() == 0) {
    std::vector<value_t> values(array.begin(), array.end());
    DASH_LOG_DEBUG_VAR("ViewTest.LocalBlocksView1Dim", values);
  }
  array.barrier();

  auto lblocks_view  = dash::local(
                         dash::blocks(
                           array));
  // DASH_LOG_DEBUG_VAR("ViewTest.LocalBlocksView1Dim", lblocks_view);

  auto lblocks_index = dash::index(lblocks_view);
  DASH_LOG_DEBUG_VAR("ViewTest.LocalBlocksView1Dim", lblocks_index);

  auto blocksl_view  = dash::blocks(
                         dash::local(
                           array));
  // DASH_LOG_DEBUG_VAR("ViewTest.LocalBlocksView1Dim", blocksl_view);

  auto blocksl_index = dash::index(blocksl_view);
  DASH_LOG_DEBUG_VAR("ViewTest.LocalBlocksView1Dim", blocksl_index);

  auto lsize      = array.pattern().local_extent(0);
  auto l_beg      = array.pattern().global_index(array.team().myid(),
                                                 {{ 0 }} );
  auto l_end      = array.pattern().global_index(array.team().myid(),
                                                 {{ lsize }} );
  auto n_lblocks  = dash::math::div_ceil(array.lsize(), block_size);

  DASH_LOG_DEBUG("ViewTest.LocalBlocksView1Dim",
                 "n_lblocks:", n_lblocks, "l_beg:", l_beg, "l_end:", l_end);

  EXPECT_EQ_U(n_lblocks, blocksl_view.size());
  EXPECT_EQ_U(n_lblocks, blocksl_index.size());

  int b_idx = 0;
  for (const auto & block : blocksl_view) {
    auto block_index = dash::index(block);

    DASH_LOG_DEBUG("ViewTest.LocalBlocksView1Dim",
                   "---- local block", b_idx);

    std::vector<index_t> block_indices(block_index.begin(),
                                       block_index.end());
    DASH_LOG_DEBUG_VAR("ViewTest.LocalBlocksView1Dim", block_indices);
  //DASH_LOG_DEBUG_VAR("ViewTest.LocalBlocksView1Dim", block);

    std::vector<value_t> block_values(block.begin(), block.end());
    DASH_LOG_DEBUG_VAR("ViewTest.LocalBlocksView1Dim", block_values);

    auto lblock_size   = array.pattern().local_block(b_idx).extents()[0];
    auto lblock_gbegin = array.pattern().local_block(b_idx).offsets()[0];

    EXPECT_EQ_U(lblock_size, block.size());
    for (auto bi = 0; bi < lblock_size; ++bi) {
      EXPECT_EQ_U(static_cast<value_t>(array[bi + lblock_gbegin]),
                  static_cast<value_t>(block[bi]));
    }
    b_idx++;
  }

  dash::Array<value_t> array_bal(
                         dash::size() * block_size,
                         dash::BLOCKCYCLIC(block_size));
  dash::test::initialize_array(array_bal);

  auto sub_view        = dash::sub(
                           block_size / 2,
                           array.size() - (block_size / 2),
                           array_bal);
  auto blockssub_view  = dash::blocks(sub_view);
  auto lblockssub_view = dash::local(blockssub_view);

  auto lblockssub_index = dash::index(lblockssub_view);

  DASH_LOG_DEBUG_VAR("ViewTest.LocalBlocksView1Dim", lblockssub_index);
}

TEST_F(ViewTest, BlocksView1Dim)
{
  typedef float                 value_t;
  typedef dash::default_index_t index_t;

  int block_size           = 3;
  int blocks_per_unit      = 3;
  int array_size           = dash::size()
                             * (blocks_per_unit * block_size)
                             // unbalanced size, last block underfilled:
                             - (block_size / 2);

  int sub_left_begin_gidx  = 0;
  int sub_left_end_gidx    = array_size - (block_size / 2) - 1;
  int sub_right_begin_gidx = (block_size * 3) / 2;
  int sub_right_end_gidx   = array_size;

  dash::Array<value_t> array(array_size, dash::BLOCKCYCLIC(block_size));
  dash::test::initialize_array(array);

  if (dash::myid() == 0) {
    std::vector<value_t> values(array.begin(), array.end());
    DASH_LOG_DEBUG_VAR("ViewTest.BlocksView1Dim", values);
  }
  array.barrier();

  auto array_blocks = dash::blocks(
                          dash::sub<0>(0, array.size(),
                            array));

  DASH_LOG_DEBUG("ViewTest.BlocksView1Dim",
                 "array.blocks.size:", array_blocks.size(),
                 "=", array_blocks.end() - array_blocks.begin(),
                 "=", dash::index(array_blocks).size());

  EXPECT_EQ_U(array_blocks.size(),
              array_blocks.end() - array_blocks.begin());

  array.barrier();

  if (dash::myid() == 0) {
    DASH_LOG_DEBUG("ViewTest.BlocksView1Dim", "blocks(array):",
                   "index(blocks).begin, index(blocks).end:",
                   "(", *(dash::index(array_blocks).begin()),
                   ",", *(dash::index(array_blocks).end()),
                   ")", "size:",    array_blocks.size(),
                   "=", array_blocks.end() - array_blocks.begin(),
                   "=", "indices:", dash::index(array_blocks).size());

    int b_idx = 0;
    for (auto b_it = array_blocks.begin();
         b_it != array_blocks.end(); ++b_it, ++b_idx) {
      auto && block = *b_it;
      EXPECT_EQ_U(b_idx, b_it.pos());

      DASH_LOG_DEBUG("ViewTest.BlocksView1Dim", "--",
                     "block[", b_idx, "]:",
                     dash::typestr(block));
      DASH_LOG_DEBUG("ViewTest.BlocksView1Dim", "----",
                     "p.offsets:", array.pattern().block(b_idx).offsets()[0],
                     "p.extents:", array.pattern().block(b_idx).extents()[0],
                     "->", dash::index(array_blocks)[b_idx],
                     "index(block).begin, index(block).end:",
                     "(", *(dash::begin(dash::index(block))),
                     ",", *(dash::end(dash::index(block))),
                     ")", "size:",    block.size(),
                     "=", "indices:", dash::index(block).size());

      DASH_LOG_DEBUG("ViewTest.BlocksView1Dim", "----",
                     range_str(block));

      EXPECT_EQ_U(( b_idx < array_blocks.size() - 1
                    ? block_size
                    : block_size - (block_size / 2) ),
                  block.size());
      EXPECT_TRUE_U(
        std::equal(array.begin() + (b_idx * block_size),
                   array.begin() + (b_idx * block_size) + block.size(),
                   block.begin()));
    }
  }
  array.barrier();

  // View to first two thirds of global array:
  auto gview_left   = dash::sub(sub_left_begin_gidx,
                                sub_left_end_gidx,
                                array);
  // View to last two thirds of global array:
  auto gview_right  = dash::sub(sub_right_begin_gidx,
                                sub_right_end_gidx,
                                array);

  auto gview_isect  = dash::intersect(gview_left, gview_right);

  EXPECT_EQ_U(sub_left_end_gidx - sub_right_begin_gidx,
              gview_isect.size());

  if (dash::myid() == 0) {
    DASH_LOG_DEBUG("ViewTest.BlocksView1Dim", "index(gview_isect(array)):",
                   "(begin, first, last, end):",
                   "(", *(dash::index(gview_isect).begin()),
                   ",", dash::index(gview_isect).first(),
                   ",", dash::index(gview_isect).last(),
                   ",", *(dash::index(gview_isect).end()),
                   ")", "size:", dash::index(gview_isect).size());

    DASH_LOG_DEBUG_VAR("ViewTest.BlocksView1Dim", range_str(gview_isect));
  }
  array.barrier();

  EXPECT_TRUE_U(
    std::equal(array.begin() + sub_right_begin_gidx,
               array.begin() + sub_left_end_gidx,
               gview_isect.begin()));

  auto gview_blocks = dash::blocks(gview_isect);

  static_assert(
      dash::view_traits<
        std::remove_reference< decltype(gview_blocks) >::type
      >::is_view::value == true,
      "view traits is_view for blocks(dash::Array) not matched");

  array.barrier();

  if (dash::myid() == 0) {
    DASH_LOG_DEBUG("ViewTest.BlocksView1Dim",
                   "index(blocks(gview_isect(array))):",
                   "(begin, first, last, end):",
                   "(", *(dash::index(gview_blocks).begin()),
                   ",", dash::index(gview_blocks).first(),
                   ",", dash::index(gview_blocks).last(),
                   ",", *(dash::index(gview_blocks).end()),
                   ")", "size:", dash::index(gview_blocks).size());

    std::vector<value_t> gview_blocks_values;

    int b_idx = 0;
    for (auto block : gview_blocks) {
      DASH_LOG_DEBUG("ViewTest.BlocksView1Dim", "--",
                     "block[", b_idx, "]:",
                     dash::typestr(block));
      DASH_LOG_DEBUG("ViewTest.BlocksView1Dim", "----",
                     "p.offsets:", array.pattern().block(b_idx).offsets()[0],
                     "p.extents:", array.pattern().block(b_idx).extents()[0],
                     "->", (dash::index(gview_blocks)[b_idx]),
                     "index(block.begin, block.end):", 
                     "(", *(dash::index(block).begin()),
                     ",", *(dash::index(block).end()), ")",
                     "size:", dash::index(block).size());

      DASH_LOG_DEBUG("ViewTest.BlocksView1Dim", "----",
                     range_str(block));

      std::move(block.begin(), block.end(), std::back_inserter(gview_blocks_values));
      b_idx++;
    }
    EXPECT_EQ_U(gview_isect.size(),
                gview_blocks_values.size());
    EXPECT_TRUE(std::equal(gview_isect.begin(),
                           gview_isect.end(),
                           gview_blocks_values.begin()));
  }
}

TEST_F(ViewTest, Intersect1DimSingle)
{
  int block_size           = 13;
  int array_size           = dash::size() * block_size
                             // unbalanced size:
                             + 2;

  int sub_left_begin_gidx  = 0;
  int sub_left_end_gidx    = (array_size * 2) / 3;
  int sub_right_begin_gidx = (array_size * 1) / 3;
  int sub_right_end_gidx   = array_size;

  dash::Array<int> array(array_size);

  for (auto li = 0; li != array.local.size(); ++li) {
    array.local[li] = (1000 * (dash::myid() + 1)) +
                      (100    * li) +
                      (dash::myid() * block_size) + li;
  }
  array.barrier();

  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimSingle", array);

  // View to first two thirds of global array:
  auto gview_left   = dash::sub(sub_left_begin_gidx,
                                sub_left_end_gidx,
                                array);
  // View to last two thirds of global array:
  auto gview_right  = dash::sub(sub_right_begin_gidx,
                                sub_right_end_gidx,
                                array);

  auto gview_isect  = dash::intersect(gview_left, gview_right);

  auto gindex_isect = dash::index(gview_isect);

  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimSingle", gview_isect);
  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimSingle", gindex_isect);

  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimSingle", array.size());
  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimSingle", gview_left.size());
  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimSingle", gview_right.size());
  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimSingle", gview_isect.size());
  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimSingle", *gindex_isect.begin());
  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimSingle", *gindex_isect.end());

  EXPECT_EQ_U(sub_left_end_gidx  - sub_left_begin_gidx,
              gview_left.size());
  EXPECT_EQ_U(sub_right_end_gidx - sub_right_begin_gidx,
              gview_right.size());
  EXPECT_EQ_U(sub_left_end_gidx  - sub_right_begin_gidx,
              gview_isect.size());

  for (int isect_idx = 0; isect_idx < gview_isect.size(); isect_idx++) {
    EXPECT_EQ_U(static_cast<int>(array[sub_right_begin_gidx + isect_idx]),
                static_cast<int>(gview_isect[isect_idx]));
  }

  auto lview_isect  = dash::local(gview_isect);
  auto lindex_isect = dash::index(lview_isect);

  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimSingle", *lindex_isect.begin());
  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimSingle", *lindex_isect.end());
}

TEST_F(ViewTest, Intersect1DimChain)
{
  int block_size           = 4;
  int blocks_per_unit      = 3;
  int array_size           = dash::size()
                             * (blocks_per_unit * block_size)
                             // unbalanced size, last block underfilled:
                             - (block_size / 2);

  int sub_left_begin_gidx  = 0;
  int sub_left_end_gidx    = array_size - (block_size / 2);
  int sub_right_begin_gidx = (block_size / 2);
  int sub_right_end_gidx   = array_size;

  dash::Array<seq_pos_t> array(array_size,
                               dash::BLOCKCYCLIC(block_size));
  dash::test::initialize_array(array);

  DASH_LOG_DEBUG("ViewTest.Intersect1DimChain",
                 "array initialized");
  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimChain", array.size());

  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimChain",
                     dash::index(dash::local(array)));
  //DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimChain",
  //                   dash::global(dash::index(dash::local(array))));

  array.barrier();

  // View to first two thirds of global array:
  auto gview_left   = dash::sub(sub_left_begin_gidx,
                                sub_left_end_gidx,
                                array);
  // View to last two thirds of global array:
  auto gview_right  = dash::sub(sub_right_begin_gidx,
                                sub_right_end_gidx,
                                array);

  auto gview_isect  = dash::intersect(gview_left, gview_right);

  auto gindex_isect = dash::index(gview_isect);

  if (dash::myid() == 0) {
    DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimChain",
                       dash::typestr(gview_isect));
    DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimChain", gview_left.size());
    DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimChain", gview_right.size());
    DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimChain", gview_isect.size());
    DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimChain", *gindex_isect.begin());
    DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimChain", *gindex_isect.end());
    DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimChain", range_str(gview_isect));

    EXPECT_TRUE_U(std::equal(array.begin() + sub_right_begin_gidx,
                             array.begin() + sub_left_end_gidx,
                             gview_isect.begin()));
  }
  array.barrier();

  auto exp_isect_n  = sub_left_end_gidx - sub_right_begin_gidx;

  EXPECT_EQ_U(exp_isect_n, gview_isect.size());
  EXPECT_EQ_U(exp_isect_n, gindex_isect.size());
  EXPECT_EQ_U(exp_isect_n,
              dash::distance(gindex_isect.begin(), gindex_isect.end()));
  EXPECT_EQ_U(exp_isect_n,
              dash::distance(gview_isect.begin(), gview_isect.end()));
  EXPECT_EQ_U(sub_right_begin_gidx, *gindex_isect.begin());
  EXPECT_EQ_U(sub_left_end_gidx,    *gindex_isect.end());

  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimChain",
                     array.pattern().local_size());
  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimChain",
                     array.pattern().global(0));
  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimChain",
                     array.pattern().global(array.pattern().local_size()));

  auto lview_isect  = dash::local(gview_isect);
  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimChain",
                     dash::typestr(lview_isect.begin()));

  auto lindex_isect = dash::index(lview_isect);
  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimChain",
                     dash::typestr(lindex_isect));

  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimChain",
                     lindex_isect.domain_block_gidx_last());
  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimChain",
                     lindex_isect.domain_block_lidx_last());
  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimChain",
                     lindex_isect.local_block_gidx_last());
  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimChain",
                     lindex_isect.local_block_gidx_at_block_lidx(
                       lindex_isect.domain_block_lidx_last()));
  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimChain",
                     lindex_isect.local_block_gidx_at_block_lidx(
                       lindex_isect.domain_block_lidx_last() - 1));
  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimChain",
                     lindex_isect.pattern().local_block(1)
                                           .range(0).begin);
  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimChain",
                     lindex_isect.pattern().local_block(1)
                                           .range(0).end);

  array.barrier();

  //auto lrange_isect = dash::local_index_range(
  //                      array.begin() + sub_right_begin_gidx,
  //                      array.begin() + sub_left_end_gidx);
  //DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimChain", lrange_isect.begin);
  //DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimChain", lrange_isect.end);

  static_assert(
    dash::detail::has_type_domain_type<decltype(lindex_isect)>::value,
    "Type trait has_type_domain_type not matched "
    "for index(local(intersect(...))) ");

  auto lindex_isect_dom = dash::domain(lindex_isect);
  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimChain",
                     dash::typestr(lindex_isect_dom));

  static_assert(
    dash::is_range<decltype(lindex_isect_dom)>::value,
    "View trait is_range not matched for index(local(intersect(...)))");

  auto lindex_isect_dom_pre = dash::domain(lindex_isect).pre();

  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimChain", lindex_isect);
  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimChain", lindex_isect_dom);
  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimChain", lindex_isect_dom_pre);
  EXPECT_TRUE_U(
    dash::test::expect_range_values_equal<int>(
      dash::domain(lindex_isect),
      lindex_isect.domain()));

  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimChain", lview_isect.size());
  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimChain", lindex_isect.size());
  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimChain", range_str(lview_isect));

  auto && lindex_pattern         = lindex_isect.pattern();
  auto    lindex_last_lblock_idx = lindex_pattern.local_block_at(
                                     lindex_pattern.coords(
                                       lindex_pattern.lend() - 1)).index;
  auto    lindex_last_lblock     = lindex_pattern.local_block(
                                     lindex_last_lblock_idx);
  auto    lindex_last_dblock_idx = lindex_pattern.local_block_at(
                                     lindex_pattern.coords(
                                       lindex_isect.domain().last())).index;
  auto    lindex_last_dblock     = lindex_pattern.local_block(
                                     lindex_last_dblock_idx);

  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimChain", lindex_last_lblock_idx);
  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimChain", lindex_last_lblock);
  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimChain", lindex_last_dblock_idx);
  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimChain", lindex_last_dblock);

  int lidx = 0;
  for (auto gidx = sub_right_begin_gidx;
            gidx < sub_left_end_gidx;
            ++gidx) {
    auto lptr = (array.begin() + gidx).local();
    if (nullptr != lptr) {
      EXPECT_EQ_U(*lptr, lview_isect[lidx]);
      ++lidx;
    }
  }
  EXPECT_EQ_U(lidx, lview_isect.size());
}

TEST_F(ViewTest, ArrayBlockedPatternLocalView)
{
  int block_size        = 7;
  int array_size        = dash::size() * block_size;
  int lblock_begin_gidx = block_size * dash::myid();
  int lblock_end_gidx   = lblock_begin_gidx + block_size;

  dash::Array<seq_pos_t> array(array_size);
  initialize_array(array);

  DASH_LOG_DEBUG("ViewTest.ArrayBlockedPatternLocalView",
                 "array initialized");

  if (dash::myid() == 0) {
    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                       array.pattern().size());
    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                       array.pattern().blockspec().size());
    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                       array.pattern().local_size());
    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                       range_str(array));
  }
  array.barrier();

  // View index sets:
  auto l_begin_gidx = array.pattern().global(0);

  DASH_LOG_DEBUG("ViewTest.ArrayBlockedPatternLocalView",
                 "index(sub(",
                 l_begin_gidx, ",", l_begin_gidx + block_size, ", a ))");

  auto g_sub_view   = dash::sub(
                        l_begin_gidx,
                        l_begin_gidx + block_size,
                        array );

  auto g_sub_index  = dash::index(
                        dash::sub(
                          l_begin_gidx,
                          l_begin_gidx + block_size,
                          array) );

  DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                     *g_sub_index.begin());
  DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                     *g_sub_index.end());
  DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                     range_str(g_sub_view));

  EXPECT_EQ_U(block_size,   g_sub_view.size());
  EXPECT_EQ_U(block_size,   g_sub_view.end() - g_sub_view.begin());

  EXPECT_EQ_U(block_size,   g_sub_index.size());
  EXPECT_EQ_U(block_size,   g_sub_index.end() - g_sub_index.begin());
  EXPECT_EQ_U(l_begin_gidx, *g_sub_index.begin());
  EXPECT_EQ_U(l_begin_gidx + block_size, *g_sub_index.end());

  DASH_LOG_DEBUG("ViewTest.ArrayBlockedPatternLocalView",
                 "index(local(sub(",
                 l_begin_gidx, ",", l_begin_gidx + block_size, ", a )))");

  auto l_sub_view   = dash::local(
                        dash::sub(
                          l_begin_gidx,
                          l_begin_gidx + block_size,
                          array) );

  DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                     range_str(l_sub_view));

  auto l_sub_index  = dash::index(
                        dash::local(
                          dash::sub(
                            l_begin_gidx,
                            l_begin_gidx + block_size,
                            array) ) );
  EXPECT_EQ_U(block_size, l_sub_view.size());
  EXPECT_EQ_U(block_size, l_sub_index.size());
  EXPECT_EQ_U(block_size, array.lsize());

  EXPECT_TRUE_U(
    std::equal(array.local.begin(),
               array.local.end(),
               l_sub_view.begin()));

  auto l_idx_set_begin = *dash::begin(l_sub_index);
  auto l_idx_set_end   = *dash::end(l_sub_index);

  EXPECT_EQ(0,              l_idx_set_begin);
  EXPECT_EQ(0 + block_size, l_idx_set_end);

  // Use case:
  //
  // array   [ ... | 0 1 2 3 4 5 6 7 8 9 | ... ]
  //               :     |         |     :
  // sub           :     '---------'     :
  //               |     :         :     |
  // local         '---------------------'
  //                     |         |
  //                     '----.----'
  //                          |
  //                  local(sub(array))
  //
  {
    DASH_LOG_DEBUG("ViewTest.ArrayBlockedPatternLocalView",
                   "--------- inner ---------");
    int sub_begin_gidx    = lblock_begin_gidx + 2;
    int sub_end_gidx      = lblock_end_gidx   - 2;

    // View to global index range of local block:
    auto sub_lblock = dash::sub(sub_begin_gidx,
                                sub_end_gidx,
                                array);

    static_assert(
      !dash::view_traits<decltype(sub_lblock)>::is_local::value,
      "sub(range) expected have type trait local = false");

    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                       range_str(sub_lblock));

    EXPECT_EQ(block_size - 4, sub_lblock.size());
    EXPECT_EQ(sub_lblock.size(),
              dash::end(sub_lblock) - dash::begin(sub_lblock));

    auto l_sub_lblock = dash::local(sub_lblock);

    static_assert(
      dash::view_traits<decltype(l_sub_lblock)>::is_local::value,
      "local(sub(range)) expected have type trait local = true");

    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                       range_str(l_sub_lblock));

    EXPECT_EQ(sub_lblock.size(), l_sub_lblock.size());
    EXPECT_EQ(l_sub_lblock.size(),
              dash::end(l_sub_lblock) - dash::begin(l_sub_lblock));

    EXPECT_EQ(array.pattern().at(
                dash::index(sub_lblock)[0]),
              dash::index(l_sub_lblock)[0]);
    EXPECT_EQ(dash::index(sub_lblock).size(),
              dash::index(l_sub_lblock).size());

    for (int lsi = 0; lsi != sub_lblock.size(); lsi++) {
      auto sub_elem   = sub_lblock[lsi];
      auto l_sub_elem = l_sub_lblock[lsi];
      EXPECT_EQ(sub_elem, l_sub_elem);
    }

    auto sub_l_sub_lblock = dash::sub(
                              1, l_sub_lblock.size() - 2,
                              l_sub_lblock);

    static_assert(
      dash::view_traits<decltype(sub_l_sub_lblock)>::is_local::value,
      "sub(local(sub(range))) expected have type trait local = true");

    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                       range_str(sub_l_sub_lblock));
    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                       l_sub_lblock.size());
    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                       sub_l_sub_lblock.size());

    EXPECT_EQ(sub_l_sub_lblock.size(), l_sub_lblock.size() - 1 - 2);
    EXPECT_EQ(
      sub_l_sub_lblock.size(),
      dash::end(sub_l_sub_lblock) - dash::begin(sub_l_sub_lblock));

    for (int slsi = 0; slsi < sub_l_sub_lblock.size(); slsi++) {
      auto sub_l_sub_elem = sub_l_sub_lblock[slsi];
      auto l_sub_elem     = l_sub_lblock[slsi+1];
      EXPECT_EQ(l_sub_elem, sub_l_sub_elem);
    }
  }
  {
    DASH_LOG_DEBUG("ViewTest.ArrayBlockedPatternLocalView",
                   "------- local inner -----");

    auto sub_local_view = dash::sub(
                            2, array.lsize() - 1,
                            dash::local(
                              array));

    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                       range_str(sub_local_view));

    EXPECT_EQ_U(array.lsize() - 2 - 1,
                sub_local_view.size());

    EXPECT_TRUE_U(
        std::equal(array.lbegin() + 2,
                   array.lbegin() + array.lsize() - 1,
                   sub_local_view.begin()));
  }
  // Use case:
  //
  // array   [ .. | 0 1 2 3 4 5 6 7 8 9 | ... ]
  //              |     :         :     |
  // sub          '---------------------'
  //                    :         :
  // local              '---------'
  //                    |         |
  //                    '----.----'
  //                         |
  //                 local(sub(array))
  //
  {
    DASH_LOG_DEBUG("ViewTest.ArrayBlockedPatternLocalView",
                   "--------- outer ---------");
    int sub_begin_gidx    = lblock_begin_gidx;
    int sub_end_gidx      = lblock_end_gidx;

    if (dash::myid() > 0) {
      sub_begin_gidx -= 2;
    }
    if (dash::myid() < dash::size() - 1) {
      sub_end_gidx   += 3;
    }

    // View to global index range of local block:
    auto sub_block = dash::sub(sub_begin_gidx,
                               sub_end_gidx,
                               array);
    static_assert(
      !dash::view_traits<decltype(sub_block)>::is_local::value,
      "sub(range) expected have type trait local = false");

    EXPECT_EQ_U(sub_end_gidx - sub_begin_gidx, sub_block.size());
    EXPECT_EQ_U(sub_block.size(),
                dash::end(sub_block) - dash::begin(sub_block));

    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                       range_str(sub_block));

    EXPECT_TRUE_U(
      std::equal(array.begin() + sub_begin_gidx,
                 array.begin() + sub_end_gidx,
                 sub_block.begin()));

    auto l_sub_block       = dash::local(sub_block);
    auto l_sub_block_index = dash::index(dash::local(sub_block));

    static_assert(
      dash::view_traits<decltype(l_sub_block)>::is_local::value,
      "local(sub(range)) expected have type trait local = true");

    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                       range_str(l_sub_block));

    int exp_l_sub_block_size = array.lsize();

    EXPECT_EQ_U(l_sub_block_index.size(), l_sub_block.size());
    EXPECT_EQ_U(exp_l_sub_block_size, l_sub_block.size());
    EXPECT_EQ_U(l_sub_block.size(),
                dash::distance(l_sub_block.begin(), l_sub_block.end()));

    EXPECT_TRUE_U(
      std::equal(array.local.begin(), array.local.end(),
                 l_sub_block.begin()));

    // Applying dash::local twice without interleaving dash::global
    // expected to have no effect:
    auto sub_l_sub_block = dash::sub(1,4, dash::local(l_sub_block));

    static_assert(
      dash::view_traits<decltype(sub_l_sub_block)>::is_local::value,
      "sub(local(sub(range))) expected have type trait local = true");

    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                       range_str(sub_l_sub_block));

    EXPECT_EQ_U(3, sub_l_sub_block.size());
    EXPECT_EQ_U(sub_l_sub_block.size(),
                dash::end(sub_l_sub_block) - dash::begin(sub_l_sub_block));

    EXPECT_TRUE_U(
      std::equal(array.local.begin() + 1,
                 array.local.begin() + 4,
                 sub_l_sub_block.begin()));
  }
}

TEST_F(ViewTest, ArrayBlockCyclicPatternLocalView)
{
  int block_size        = 3;
  int nblocks_per_unit  = 2;
  int array_size        = dash::size() * block_size * nblocks_per_unit;

  dash::Array<double> array(array_size, dash::BLOCKCYCLIC(block_size));

  for (auto li = 0; li != array.local.size(); ++li) {
    array.local[li] = (100 * (dash::myid() + 1)) +
                      (li) +
                      ((dash::myid() * nblocks_per_unit * block_size)
                       + li) *
                      0.01;
  }

  array.barrier();

  int sub_begin_gidx    = 2;
  int sub_end_gidx      = array.size() - 2;

  auto sub_range = dash::sub(sub_begin_gidx,
                             sub_end_gidx,
                             array);

  if (dash::myid() == 0) {
    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockCyclicPatternLocalView",
                       range_str(sub_range));

    EXPECT_EQ_U(sub_end_gidx - sub_begin_gidx,
                sub_range.size());
    EXPECT_EQ_U(sub_range.size(),
                dash::distance(sub_range.begin(), sub_range.end()));
    EXPECT_TRUE(std::equal(array.begin() + sub_begin_gidx,
                           array.begin() + sub_end_gidx,
                           sub_range.begin()));
  }
  array.barrier();

  for (int si = 0; si != sub_range.size(); si++) {
    double sub_elem = sub_range[si];
    double arr_elem = array[si + sub_begin_gidx];
    EXPECT_EQ(arr_elem, sub_elem);
  }

  auto lsub_range = dash::local(sub_range);

  DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockCyclicPatternLocalView",
                     lsub_range.size());
  DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockCyclicPatternLocalView",
                     dash::index(lsub_range).size());
  DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockCyclicPatternLocalView",
                     *dash::begin(dash::index(lsub_range)));
  DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockCyclicPatternLocalView",
                     *dash::end(dash::index(lsub_range)));

  DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockCyclicPatternLocalView",
                     range_str(lsub_range));

  int lsi = 0;
  for (int si = 0; si != sub_range.size(); si++) {
    auto     git = (sub_range.begin() + si);
    double * lp  = git.local();
    if (lp != nullptr) {
      double lsub_elem = *lp;
      double arr_elem  = array[si + sub_begin_gidx];
      EXPECT_EQ(arr_elem, lsub_elem);
      lsi++;
    }
  }
}

/*
TEST_F(ViewTest, ArrayBlockedPatternViewUnion)
{
  int block_size         = 37;
  int array_size         = dash::size() * block_size;

  int block_a_begin_gidx = (block_size / 2) * (dash::myid() + 0);
  int block_a_end_gidx   = (block_size / 2) * (dash::myid() + 1);
  int block_b_begin_gidx = (block_size / 2) * (dash::myid() + 1);
  int block_b_end_gidx   = (block_size / 2) * (dash::myid() + 2);

  dash::Array<int> a(array_size);

  auto block_a_gview     = dash::sub(block_a_begin_gidx,
                                     block_a_end_gidx,
                                     a);
  auto block_b_gview     = dash::sub(block_b_begin_gidx,
                                     block_b_end_gidx,
                                     a);
  auto block_views_union = dash::set_union({
                             block_a_gview,
                             block_b_gview
                           });
}
*/
