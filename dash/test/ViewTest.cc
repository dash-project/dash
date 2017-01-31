
#include "ViewTest.h"

#include <dash/View.h>
#include <dash/Array.h>

#include <array>
#include <algorithm>


static_assert(dash::is_range<
                 dash::Array<int>
              >::value == true,
              "dash::is_range<dash::Array>::value not matched");

static_assert(dash::is_range<
                 typename dash::Array<int>::local_type
              >::value == true,
              "dash::is_range<dash::Array::local_type>::value not matched");

static_assert(dash::is_range<
                 typename dash::Array<int>::iterator
              >::value == false,
              "dash::is_range<dash::Array<...>>::value not matched");


TEST_F(ViewTest, ViewTraits)
{
  dash::Array<int> array(dash::size() * 10);
  auto v_sub = dash::sub(0, 10, array);
  auto v_loc = dash::local(array);

  static_assert(
      dash::view_traits<decltype(array)>::is_view::value == false,
      "view traits is_view for dash::Array not matched");
  static_assert(
      dash::view_traits<decltype(v_sub)>::is_view::value == true,
      "view traits is_view for sub(dash::Array) not matched");

  static_assert(
      dash::view_traits<decltype(array)>::is_origin::value == true,
      "view traits is_origin for dash::Array not matched");
  static_assert(
      dash::view_traits<decltype(v_sub)>::is_origin::value == false,
      "view traits is_origin for sub(dash::Array) not matched");
//static_assert(dash::view_traits<decltype(v_loc)>::is_view::value == true,
//              "view traits for local(dash::Array) not matched");

  static_assert(
      dash::is_range<decltype(array)>::value == true,
      "dash::is_range<dash::Array<...>>::value not matched");
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

TEST_F(ViewTest, BlocksView1Dim)
{
  typedef float                 value_t;
  typedef dash::default_index_t index_t;

  int block_size           = 4;
  int blocks_per_unit      = 2;
  int array_size           = dash::size()
                             * (blocks_per_unit * block_size)
                             // unbalanced size, last block underfilled:
                             - (block_size / 2);

  int sub_left_begin_gidx  = 0;
  int sub_left_end_gidx    = array_size - (block_size / 2);
  int sub_right_begin_gidx = (block_size * 3) / 2;
  int sub_right_end_gidx   = array_size;

  dash::Array<value_t> array(array_size, dash::BLOCKCYCLIC(block_size));

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

  DASH_LOG_DEBUG("ViewTest.BlocksView1Dim",
                 "array initialized");

  if (dash::myid() == 0) {
    std::vector<value_t> values(array.size());
    std::copy(array.begin(), array.end(), values.begin());
    DASH_LOG_DEBUG_VAR("ViewTest.BlocksView1Dim", values);
  }

  auto array_blocks = dash::blocks(dash::sub<0>(0, array.size(), array));

  DASH_LOG_DEBUG("ViewTest.BlocksView1Dim", "blocks(array):",
                 "index(blocks).begin, index(blocks).end:",
                 "(", *(dash::index(array_blocks).begin()),
                 ",", *(dash::index(array_blocks).end()),
                 ")", "size:",    array_blocks.size(),
                 "=", array_blocks.end() - array_blocks.begin(),
                 "=", "indices:", dash::index(array_blocks).size());

  EXPECT_EQ_U(array_blocks.size(),
              array_blocks.end() - array_blocks.begin());

  if (dash::myid() == 0) {
//  for (auto block : array_blocks) {
    for (auto b_it = array_blocks.begin();
         b_it != array_blocks.end(); ++b_it) {
      auto block = *b_it;
      auto b_idx = b_it.pos();

      DASH_LOG_DEBUG("ViewTest.BlocksView1Dim", "--",
                     "block index:", b_idx,
                     "offsets:", array.pattern().block(b_idx).offsets()[0],
                     "extents:", array.pattern().block(b_idx).extents()[0]);

      DASH_LOG_DEBUG("ViewTest.BlocksView1Dim", "--",
                     "blocks[b].begin.pos, blocks[b].end.pos", 
                     "(", (array_blocks[b_idx].begin()),
                     ",", (array_blocks[b_idx].end()),
                     ")");

      DASH_LOG_DEBUG("ViewTest.BlocksView1Dim", "----",
                     "index(block).begin, index(block).end:", 
                     "(", *(dash::index(block).begin()),
                     ",", *(dash::index(block).end()),
                     ")", "size:",    block.size(),
                     "=", "indices:", dash::index(block).size());

      std::vector<value_t> block_values(block.size());
      std::copy(block.begin(), block.end(), block_values.begin());
      DASH_LOG_DEBUG("ViewTest.BlocksView1Dim", "----", block_values);
    }
  }

  // View to first two thirds of global array:
  auto gview_left   = dash::sub(sub_left_begin_gidx,
                                     sub_left_end_gidx,
                                     array);
  // View to last two thirds of global array:
  auto gview_right  = dash::sub(sub_right_begin_gidx,
                                   sub_right_end_gidx,
                                   array);

  auto gview_isect  = dash::intersect(gview_left, gview_right);
//auto gview_isect  = dash::sub(
//                         block_size,
//                         array_size-block_size,
//                         array);

  DASH_LOG_DEBUG("ViewTest.BlocksView1Dim", "index(gview_isect(array)):",
                 "(begin, first, last, end):",
                 "(", *(dash::index(gview_isect).begin()),
                 ",", dash::index(gview_isect).first(),
                 ",", dash::index(gview_isect).last(),
                 ",", *(dash::index(gview_isect).end()),
                 ")", "size:", dash::index(gview_isect).size());

  if (dash::myid() == 0) {
    std::vector<value_t> values(gview_isect.size());
    std::copy(gview_isect.begin(),
              gview_isect.end(),
              values.begin());
    DASH_LOG_DEBUG_VAR("ViewTest.BlocksView1Dim", values);

    std::vector<index_t> indices(gview_isect.size());
    std::copy(dash::index(gview_isect).begin(),
              dash::index(gview_isect).end(),
              indices.begin());
    DASH_LOG_DEBUG_VAR("ViewTest.BlocksView1Dim", indices);
  }

  auto gview_blocks = dash::blocks(gview_isect);

  static_assert(
      dash::view_traits<
        std::remove_reference< decltype(gview_blocks) >::type 
      >::is_view::value == true,
      "view traits is_view for blocks(dash::Array) not matched");

  DASH_LOG_DEBUG("ViewTest.BlocksView1Dim",
                 "index(blocks(gview_isect(array))):",
                 "(begin, first, last, end):",
                 "(", *(dash::index(gview_blocks).begin()),
                 ",", dash::index(gview_blocks).first(),
                 ",", dash::index(gview_blocks).last(),
                 ",", *(dash::index(gview_blocks).end()),
                 ")", "size:", dash::index(gview_blocks).size());

  if (dash::myid() == 0) {
    int b_idx = 0;
    for (auto block : gview_blocks) {
      DASH_LOG_DEBUG("ViewTest.BlocksView1Dim", "--",
                     "lblock index:", b_idx,
                     "offsets:", array.pattern().block(b_idx).offsets(),
                     "extents:", array.pattern().block(b_idx).extents());

      DASH_LOG_DEBUG("ViewTest.BlocksView1Dim", "----",
                     "index(block.begin, block.end):", 
                     "(", *(dash::index(block).begin()),
                     ",", *(dash::index(block).end()), ")",
                     "size:", dash::index(block).size());

      std::vector<value_t> block_values(block.size());
      std::copy(block.begin(), block.end(), block_values.begin());
      DASH_LOG_DEBUG("ViewTest.BlocksView1Dim", "----", block_values);

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

  auto lview_isect  = dash::local(gview_isect);
  auto lindex_isect = dash::index(lview_isect);

  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimMultiple",
                     *dash::begin(lindex_isect));
  DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimMultiple",
                     *dash::end(lindex_isect));

  if (dash::myid() == 0) {
    std::vector<int> values(array.size());
    std::copy(array.begin(), array.end(), values.begin());
    DASH_LOG_DEBUG_VAR("ViewTest.Intersect1DimMultiple", values);
  }
}

TEST_F(ViewTest, ArrayBlockedPatternGlobalView)
{
  int block_size       = 37;
  int array_size       = dash::size() * block_size;
  int block_begin_gidx = block_size * dash::myid();
  int block_end_gidx   = block_size * (dash::myid() + 1);

  dash::Array<int> a(array_size);

  // View to global index range of local block:
  auto block_gview = dash::sub(block_begin_gidx,
                               block_end_gidx,
                               a);
  EXPECT_EQ(block_size, block_gview.size());

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
  int block_size       = 37;
  int array_size       = dash::size() * block_size;
  int block_begin_gidx = block_size * dash::myid();
  int block_end_gidx   = block_size * (dash::myid() + 1);

  dash::Array<int> a(array_size);

  // View to global index range of local block:
  auto block_gview_outer = dash::sub(block_begin_gidx,
                                     block_end_gidx,
                                     a);
  // Sub-range in block from block index 10 to -10:
  auto block_gview_inner = dash::sub(10,
                                     block_size-10,
                                     block_gview_outer);
  EXPECT_EQ(block_size - 10 - 10,  block_gview_inner.size());
  EXPECT_EQ(block_begin_gidx + 10,
            dash::index(dash::begin(block_gview_inner)));
  EXPECT_EQ(block_begin_gidx + block_size - 10,
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
  int block_size       = 37;
  int blocks_per_unit  = 3;
  int array_size       = dash::size() * block_size * blocks_per_unit;
  int block_begin_gidx = block_size * dash::myid();
  int block_end_gidx   = block_size * (dash::myid() + 1);

  dash::Array<int> a(array_size, dash::BLOCKCYCLIC(block_size));

  // View to global index range of local block:
  auto block_gview = dash::sub(block_begin_gidx,
                               block_end_gidx,
                               a);
  EXPECT_EQ(block_size, block_gview.size());

  // Origin of block view is array:
  auto & block_domain = dash::domain(block_gview);
  EXPECT_EQ(a.begin(),  dash::begin(block_domain));
  EXPECT_EQ(a.end(),    dash::end(block_domain));
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
                     *l_idx_set.end());
  DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                     l_idx_set.end() - l_idx_set.begin());
  DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                     l_idx_set.size());

  DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                     l_idx_set[0]);

  auto l_idx_set_begin = *dash::begin(l_idx_set);
  DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                     l_idx_set_begin);

  auto l_idx_set_end   = *dash::end(l_idx_set);
  DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                     l_idx_set_end);

  DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                     l_idx_set[0]);
  DASH_LOG_DEBUG_VAR("ViewTest.ArrayBlockedPatternLocalView",
                     l_idx_set.last());

#if __TODO__
  // Fails for nunits = 1:
  EXPECT_EQ(l_begin_gidx,              l_idx_set_begin);
  EXPECT_EQ(l_begin_gidx + block_size, l_idx_set_end);
#endif

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

    EXPECT_EQ(block_size - 4, sub_lblock.size());
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
  }
}

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

