
#include "ViewTest.h"

#include <dash/Array.h>
#include <dash/View.h>

#include <array>
#include <algorithm>
#include <sstream>


namespace dash {
namespace test {

  template <class ArrayT>
  void initialize_array(ArrayT & array) {
    auto block_size = array.pattern().blocksize(0);
    for (auto li = 0; li != array.local.size(); ++li) {
      auto block_lidx = li / block_size;
      auto block_gidx = (block_lidx * dash::size()) + dash::myid();
      auto gi         = (block_gidx * block_size) + (li % block_size);
      array.local[li] = // unit
                        (1.0000 * dash::myid()) +
                        // local offset
                        (0.0001 * (li+1)) +
                        // global offset
                        (0.0100 * gi);
    }
    array.barrier();
  }

  template <class ValueRange>
  std::string range_str(
    const ValueRange & vrange) {
    typedef typename ValueRange::value_type value_t;
    std::ostringstream ss;
    auto idx = dash::index(vrange);
    int  i   = 0;
    for (const auto & v : vrange) {
      ss << "[" << *(dash::begin(idx) + i) << "] "
         << static_cast<value_t>(v) << " ";
      ++i;
    }
    return ss.str();
  }

}
}

using dash::test::range_str;


TEST_F(ViewTest, ViewTraits)
{
  dash::Array<int> array(dash::size() * 10);
  auto v_sub  = dash::sub(0, 10, array);
  auto i_sub  = dash::index(v_sub);
  auto v_ssub = dash::sub(0, 5, (dash::sub(0, 10, array)));
  auto v_loc  = dash::local(array);

  static_assert(
      dash::view_traits<decltype(array)>::is_local::value == false,
      "view traits is_local for dash::Array not matched");
  static_assert(
      dash::view_traits<decltype(array)>::is_view::value == false,
      "view traits is_view for dash::Array not matched");
  static_assert(
      dash::view_traits<decltype(v_ssub)>::is_view::value == true,
      "view traits is_view for sub(dash::Array) not matched");

  // TODO: Clarify if local container types should be considered views.
  //
  // static_assert(
  //     dash::view_traits<decltype(v_loc)>::is_view::value == true,
  //     "view traits is_view for local(dash::Array) not matched");
  static_assert(
      dash::view_traits<decltype(i_sub)>::is_view::value == false,
      "view traits is_origin for local(dash::Array) not matched");

  static_assert(
      dash::view_traits<decltype(array)>::is_origin::value == true,
      "view traits is_origin for dash::Array not matched");
  static_assert(
      dash::view_traits<decltype(v_sub)>::is_origin::value == false,
      "view traits is_origin for sub(dash::Array) not matched");
  static_assert(
      dash::view_traits<decltype(v_ssub)>::is_origin::value == false,
      "view traits is_origin for sub(sub(dash::Array)) not matched");
  static_assert(
      dash::view_traits<decltype(v_loc)>::is_origin::value == true,
      "view traits is_origin for local(dash::Array) not matched");
  static_assert(
      dash::view_traits<decltype(v_loc)>::is_local::value == true,
      "view traits is_local for local(dash::Array) not matched");

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

TEST_F(ViewTest, ArrayBlockedPatternGlobalView)
{
  int block_size       = 3;
  int array_size       = dash::size() * block_size;
  int block_begin_gidx = block_size * dash::myid();
  int block_end_gidx   = block_size * (dash::myid() + 1);

  dash::Array<float> a(array_size);
  dash::test::initialize_array(a);

  // View to global index range of local block:
  auto block_gview = dash::sub(block_begin_gidx,
                               block_end_gidx,
                               a);
  EXPECT_EQ(block_size, block_gview.size());

  DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternGlobalView",
                     range_str(block_gview));

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

  // View to global index range of local block:
  auto block_gview_outer = dash::sub(block_begin_gidx,
                                     block_end_gidx,
                                     a);
  DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternGlobalView",
                     block_gview_outer);

  // Sub-range in block from block index 10 to -10:
  auto block_gview_inner = dash::sub(2,
                                     block_size - 2, 
                                     block_gview_outer);
  DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternGlobalView",
                     block_gview_inner);

  EXPECT_EQ(block_size - 4,  block_gview_inner.size());
  EXPECT_EQ(block_begin_gidx + 2,
            dash::index(dash::begin(block_gview_inner)));
  EXPECT_EQ(block_begin_gidx + block_size - 2,
            dash::index(dash::end(block_gview_inner)));

  // Origin of inner view is outer view:
  auto & block_gview_inner_domain = dash::domain(block_gview_inner);
  EXPECT_EQ(block_gview_outer, block_gview_inner_domain);

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

  // View to global index range of local block:
  auto block_gview = dash::sub(block_begin_gidx,
                               block_end_gidx,
                               a);

  DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockCyclicPatternGlobalView",
                     block_gview);

  EXPECT_EQ(block_size, block_gview.size());

  // Origin of block view is array:
  auto & block_domain = dash::domain(block_gview);
  EXPECT_EQ(a.begin(),  dash::begin(block_domain));
  EXPECT_EQ(a.end(),    dash::end(block_domain));

  if (dash::myid() == 0) {
    auto blocks_view = dash::blocks(
                         dash::sub(
                           block_size / 2,
                           a.size() - (block_size / 2),
                           a));
    int b_idx = 0;
    for (auto block : blocks_view) {
      DASH_LOG_DEBUG("ViewTest.ArrayBlockCyclicPatternGlobalView",
                     "block[", b_idx, "]:", range_str(block));
      // TODO: Assert
      ++b_idx;
    }
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

TEST_F(ViewTest, IndexSet)
{
  typedef float                 value_t;
  typedef dash::default_index_t index_t;

  int block_size           = 4;
  int blocks_per_unit      = 2;
  int array_size           = dash::size()
                             * (blocks_per_unit * block_size);

  dash::Array<value_t, index_t, dash::TilePattern<1>>
    array(array_size, dash::TILE(block_size));
  dash::test::initialize_array(array);

  if (dash::myid() == 0) {
    std::vector<value_t> values(array.begin(), array.end());
    DASH_LOG_DEBUG_VAR("ViewTest.IndexSet", values);

    auto sub_gview = dash::sub(
                       block_size / 2,
                       array_size - (block_size / 2),
                       array);

    auto sub_index = dash::index(sub_gview);

    DASH_LOG_DEBUG_VAR("ViewTest.IndexSet", sub_index);

    std::vector<value_t> sub_values(sub_gview.begin(),
                                    sub_gview.end());
    DASH_LOG_DEBUG_VAR("ViewTest.IndexSet", sub_values);

    EXPECT_EQ_U(array_size - block_size, sub_gview.size());
    EXPECT_EQ_U(array_size - block_size, sub_index.size());

    EXPECT_TRUE_U(std::equal(array.begin() + (block_size / 2),
                             array.begin() + array_size - (block_size / 2),
                             sub_gview.begin()));
  }
  array.barrier();

  auto sub_gview    = dash::sub(
                        block_size / 2,
                        array_size - (block_size / 2),
                        array);
  auto locsub_gview = dash::local(sub_gview);

  auto locsub_index = dash::index(locsub_gview);

  DASH_LOG_DEBUG_VAR("ViewTest.IndexSet", locsub_index);
  DASH_LOG_DEBUG_VAR("ViewTest.IndexSet", locsub_gview);

  array.barrier();

  if (dash::myid() == 0) {
    auto sub_gview    = dash::sub(
                          block_size / 2,
                          array_size - (block_size / 2),
                          array);
    EXPECT_EQ_U(
        dash::distance(
          array.begin() + (block_size / 2),
          array.begin() + (array_size - (block_size / 2))),
        dash::distance(
          sub_gview.begin(),
          sub_gview.end()) );
    EXPECT_TRUE_U(
        std::equal(
          array.begin() + (block_size / 2),
          array.begin() + (array_size - (block_size / 2)),
          sub_gview.begin()) );

    auto subsub_gview = dash::sub(3, 6, sub_gview);
    auto subsub_index = dash::index(subsub_gview);

    DASH_LOG_DEBUG_VAR("ViewTest.IndexSet", subsub_index);
    std::vector<value_t> subsub_values(subsub_gview.begin(),
                                       subsub_gview.end());
    DASH_LOG_DEBUG_VAR("ViewTest.IndexSet", subsub_values);

    EXPECT_EQ_U(
        dash::distance(
          array.begin() + (block_size / 2) + 3,
          array.begin() + (block_size / 2) + 6),
        dash::distance(
          subsub_gview.begin(),
          subsub_gview.end()) );
    EXPECT_TRUE_U(
        std::equal(
          array.begin() + (block_size / 2) + 3,
          array.begin() + (block_size / 2) + 6,
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

  auto lblocks_index = dash::index(lblocks_view);

  std::vector<index_t> lblocks_indices(lblocks_index.begin(),
                                       lblocks_index.end());
  DASH_LOG_DEBUG_VAR("ViewTest.LocalBlocksView1Dim", lblocks_indices);

  std::vector<value_t> lblocks_values(lblocks_view.begin(),
                                      lblocks_view.end());
  DASH_LOG_DEBUG_VAR("ViewTest.LocalBlocksView1Dim", lblocks_values);

  auto blocksl_view  = dash::blocks(
                         dash::local(
                           array));

  auto blocksl_index = dash::index(blocksl_view);

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
  for (auto block : blocksl_view) {
    auto block_index = dash::index(block);

    DASH_LOG_DEBUG("ViewTest.LocalBlocksView1Dim",
                   "---- local block", b_idx);

    std::vector<index_t> block_indices(block_index.begin(),
                                       block_index.end());
    DASH_LOG_DEBUG_VAR("ViewTest.LocalBlocksView1Dim", block_indices);

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
  DASH_LOG_DEBUG_VAR("ViewTest.LocalBlocksView1Dim", lblockssub_view);
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
                     dash::internal::typestr(block));
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

    int b_idx = 0;
    for (auto block : gview_blocks) {
      DASH_LOG_DEBUG("ViewTest.BlocksView1Dim", "--",
                     "block[", b_idx, "]:",
                     dash::internal::typestr(block));
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
      // TODO: Assert
      b_idx++;
    }
  }
}

TEST_F(ViewTest, Intersect1DimMultiple)
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

  dash::Array<int> array(array_size, dash::BLOCKCYCLIC(block_size));

  for (auto li = 0; li != array.local.size(); ++li) {
    array.local[li] = (1000 * (dash::myid() + 1)) +
                      (100    * li) +
                      (dash::myid() * block_size) + li;
  }

  array.barrier();

  DASH_LOG_DEBUG("ViewTest.Intersect1DimMultiple",
                 "array initialized");

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

  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimMultiple", array.size());
  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimMultiple", gview_left.size());
  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimMultiple", gview_right.size());
  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimMultiple", gview_isect.size());
  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimMultiple", *gindex_isect.begin());
  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimMultiple", *gindex_isect.end());

  // TODO: Assert

  auto lview_isect  = dash::local(gview_isect);
  auto lindex_isect = dash::index(lview_isect);

  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimMultiple",
                     *dash::begin(lindex_isect));
  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimMultiple",
                     *dash::end(lindex_isect));

  if (dash::myid() == 0) {
    std::vector<int> values(array.begin(), array.end());
    DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimMultiple", values);
    // TODO: Assert
  }
}

TEST_F(ViewTest, ArrayBlockedPatternLocalView)
{
  int block_size        = 7;
  int array_size        = dash::size() * block_size;
  int lblock_begin_gidx = block_size * dash::myid();
  int lblock_end_gidx   = lblock_begin_gidx + block_size;

  dash::Array<int> array(array_size);

  for (auto li = 0; li != array.local.size(); ++li) {
    array.local[li] = (1000000 * (dash::myid() + 1)) +
                      (1000    * li) +
                      (dash::myid() * block_size) + li;
  }

  array.barrier();
  DASH_LOG_DEBUG("ViewTest.ArrayBlockedPatternLocalView",
                 "array initialized");

  DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                     array.pattern().size());
  DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                     array.pattern().blockspec().size());
  DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                     array.pattern().local_size());

  // View index sets:
  auto l_begin_gidx = array.pattern().global(0);

  DASH_LOG_DEBUG("ViewTest.ArrayBlockedPatternLocalView",
                 "index(sub(",
                 l_begin_gidx, ",", l_begin_gidx + block_size, ", a ))");

  auto g_idx_set    = dash::index(
                        dash::sub(
                          l_begin_gidx,
                          l_begin_gidx + block_size,
                          array) );

  DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                     *g_idx_set.begin());
  DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                     *g_idx_set.end());
  DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                     g_idx_set.end() - g_idx_set.begin());
  DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                     g_idx_set.size());

  EXPECT_EQ_U(block_size,   g_idx_set.size());
  EXPECT_EQ_U(block_size,   g_idx_set.end() - g_idx_set.begin());
  EXPECT_EQ_U(l_begin_gidx, *g_idx_set.begin());
  EXPECT_EQ_U(l_begin_gidx + block_size, *g_idx_set.end());

  DASH_LOG_DEBUG("ViewTest.ArrayBlockedPatternLocalView",
                 "index(local(sub(",
                 l_begin_gidx, ",", l_begin_gidx + block_size, ", a )))");

  auto l_idx_set    = dash::index(
                        dash::local(
                          dash::sub(
                            l_begin_gidx,
                            l_begin_gidx + block_size,
                            array) ) );

  DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                     *l_idx_set.begin());
  DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                     l_idx_set.end() - l_idx_set.begin());
  DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                     l_idx_set.size());
  // TODO: Assert

  auto l_idx_set_begin = *dash::begin(l_idx_set);
  auto l_idx_set_end   = *dash::end(l_idx_set);

  DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                     l_idx_set_begin);
  DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                     l_idx_set_end);
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
                       *dash::begin(dash::index(sub_lblock)));
    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                       *dash::end(dash::index(sub_lblock)));
    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                       sub_lblock.size());
    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                       dash::end(sub_lblock) - dash::begin(sub_lblock));

    EXPECT_EQ(block_size - 4, sub_lblock.size());
    EXPECT_EQ(sub_lblock.size(),
              dash::end(sub_lblock) - dash::begin(sub_lblock));

    auto l_sub_lblock = dash::local(sub_lblock);

    static_assert(
      dash::view_traits<decltype(l_sub_lblock)>::is_local::value,
      "local(sub(range)) expected have type trait local = true");

    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                       *dash::begin(dash::index(l_sub_lblock)));
    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                       *dash::end(dash::index(l_sub_lblock)));
    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                       l_sub_lblock.size());
    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                       dash::end(l_sub_lblock) - dash::begin(l_sub_lblock));

    EXPECT_EQ(sub_lblock.size(), l_sub_lblock.size());
    EXPECT_EQ(l_sub_lblock.size(),
              dash::end(l_sub_lblock) - dash::begin(l_sub_lblock));

    EXPECT_EQ(array.pattern().at(
                dash::index(sub_lblock)[0]),
              dash::index(l_sub_lblock)[0]);
    EXPECT_EQ(dash::index(sub_lblock).size(),
              dash::index(l_sub_lblock).size());

    for (int lsi = 0; lsi != sub_lblock.size(); lsi++) {
      int sub_elem   = sub_lblock[lsi];
      int l_sub_elem = l_sub_lblock[lsi];
      DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView", sub_elem);
      DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView", l_sub_elem);
      EXPECT_EQ(sub_elem, l_sub_elem);
    }

    auto sub_l_sub_lblock = dash::sub(1,4, dash::local(l_sub_lblock));

    static_assert(
      dash::view_traits<decltype(sub_l_sub_lblock)>::is_local::value,
      "sub(local(sub(range))) expected have type trait local = true");

    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                       *dash::begin(dash::index(sub_l_sub_lblock)));
    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                       *dash::end(dash::index(sub_l_sub_lblock)));
    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                       sub_l_sub_lblock.size());
    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                       dash::end(sub_l_sub_lblock) -
                       dash::begin(sub_l_sub_lblock));
    // TODO: Assert

    for (int slsi = 0; slsi != sub_l_sub_lblock.size(); slsi++) {
      int sub_l_sub_elem = sub_l_sub_lblock[slsi];
      int l_sub_elem     = l_sub_lblock[slsi+1];
      DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                         l_sub_elem);
      DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                         sub_l_sub_elem);
      EXPECT_EQ(l_sub_elem, sub_l_sub_elem);
    }
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
      sub_begin_gidx -= 3;
    }
    if (dash::myid() < dash::size() - 1) {
      sub_end_gidx   += 3;
    }

    // View to global index range of local block:
    auto sub_lblock = dash::sub(sub_begin_gidx,
                                sub_end_gidx,
                                array);
    static_assert(
      !dash::view_traits<decltype(sub_lblock)>::is_local::value,
      "sub(range) expected have type trait local = false");

    EXPECT_EQ(sub_end_gidx - sub_begin_gidx, sub_lblock.size());
    EXPECT_EQ(sub_lblock.size(),
              dash::end(sub_lblock) - dash::begin(sub_lblock));

    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                       *dash::begin(dash::index(sub_lblock)));
    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                       *dash::end(dash::index(sub_lblock)));
    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                       sub_lblock.size());
    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                       dash::end(sub_lblock) - dash::begin(sub_lblock));

    auto l_sub_lblock = dash::local(sub_lblock);

    static_assert(
      dash::view_traits<decltype(l_sub_lblock)>::is_local::value,
      "local(sub(range)) expected have type trait local = true");

    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                       *dash::begin(dash::index(l_sub_lblock)));
    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                       *dash::end(dash::index(l_sub_lblock)));
    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                       l_sub_lblock.size());
    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                       dash::end(l_sub_lblock) - dash::begin(l_sub_lblock));

    EXPECT_EQ(block_size, l_sub_lblock.size());
    EXPECT_EQ(l_sub_lblock.size(),
              dash::end(l_sub_lblock) - dash::begin(l_sub_lblock));

    for (int lsi = 0; lsi != sub_lblock.size(); lsi++) {
      int sub_elem   = sub_lblock[lsi];
      DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView", sub_elem);
    }
    for (int lsi = 0; lsi != l_sub_lblock.size(); lsi++) {
      int l_sub_elem = l_sub_lblock[lsi];
      DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView", l_sub_elem);
      // TODO: Assert
    }

    auto sub_l_sub_lblock = dash::sub(1,4, dash::local(l_sub_lblock));

    static_assert(
      dash::view_traits<decltype(sub_l_sub_lblock)>::is_local::value,
      "sub(local(sub(range))) expected have type trait local = true");

    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                       *dash::begin(dash::index(sub_l_sub_lblock)));
    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                       *dash::end(dash::index(sub_l_sub_lblock)));
    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                       sub_l_sub_lblock.size());
    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                       dash::end(sub_l_sub_lblock) -
                       dash::begin(sub_l_sub_lblock));
    // TODO: Assert

    for (int slsi = 0; slsi != sub_l_sub_lblock.size(); slsi++) {
      int sub_l_sub_elem = sub_l_sub_lblock[slsi];
      int l_sub_elem     = l_sub_lblock[slsi+1];
      DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                         l_sub_elem);
      DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                         sub_l_sub_elem);
      EXPECT_EQ(l_sub_elem, sub_l_sub_elem);
    }
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
    for (int si = 0; si != sub_range.size(); si++) {
      double sub_elem     = sub_range[si];
      DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockCyclicPatternLocalView",
                         sub_elem);
      // TODO: Assert
    }
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

  for (int lsi = 0; lsi != lsub_range.size(); lsi++) {
    double lsub_elem     = lsub_range[lsi];
    DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockCyclicPatternLocalView",
                       lsub_elem);
    // TODO: Assert
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
