#include <array>
#include <string>
#include <numeric>
#include <functional>

#include <libdash.h>

#include "TestBase.h"
#include "GlobStencilIterTest.h"


template<
  typename ValueType,
  typename FwdIt >
void print_region(
  const std::string & name,
  const FwdIt       & begin,
  const FwdIt       & end)
{
  std::vector<ValueType> values;
  for (auto h_it = begin; h_it != end; ++h_it) {
    ValueType v = *h_it;
    values.push_back(v);
  }
  for (int i = 0; i < values.size(); ++i) {
    ValueType v = values[i];
    DASH_LOG_TRACE("GlobStencilIterTest.print_region", name,
                   "region[", i, "] =", v);
  }
}

TEST_F(GlobStencilIterTest, Conversion)
{
  // Test conversion of GlobIter to GlobStencilIter:

  typedef double                         value_t;
  typedef dash::TilePattern<2>           pattern_t;
  typedef typename pattern_t::index_type index_t;
  typedef typename pattern_t::size_type  extent_t;

  auto     myid      = dash::myid();
  extent_t num_units = dash::size();

  if (num_units < 2) {
    LOG_MESSAGE("GlobStencilIterTest.Conversion requires at least 2 units");
    return;
  }

  // Default constructor creates team spec with extents (nunits, 1):
  dash::TeamSpec<2> teamspec;
  // Automatic balancing of team spec in two dimensions:
  teamspec.balance_extents();

  extent_t tilesize_rows   = 4;
  extent_t tilesize_cols   = 3;
  extent_t num_units_rows  = teamspec.extent(0);
  extent_t num_units_cols  = teamspec.extent(1);
  extent_t num_tiles_rows  = num_units_rows > 1 ? num_units_rows * 2 : 1;
  extent_t num_tiles_cols  = num_units_cols > 1 ? num_units_cols * 3 : 1;
  extent_t matrix_rows     = tilesize_rows * num_tiles_rows;
  extent_t matrix_cols     = tilesize_cols * num_tiles_cols;
  extent_t halo_rows       = tilesize_rows;
  extent_t halo_cols       = tilesize_cols;
  extent_t block_halo_size = 2 * halo_rows + 2 * (halo_cols - 2);
  extent_t stencil_points  = 5;

  pattern_t pattern(
    dash::SizeSpec<2>(
      matrix_rows,
      matrix_cols),
    dash::DistributionSpec<2>(
      num_units_rows < 2 ? dash::NONE : dash::TILE(tilesize_rows),
      num_units_cols < 2 ? dash::NONE : dash::TILE(tilesize_cols)),
    teamspec);

  dash::Matrix<value_t, 2, index_t, pattern_t> matrix(pattern);

  // Initialize values:
  auto n_local_blocks = pattern.local_blockspec().size();
  for (extent_t lbi = 0; lbi < n_local_blocks; ++lbi) {
    // submatrix view on local block obtained from matrix relative to global
    // memory space:
    auto g_matrix_block  = matrix.local.block(lbi);

    value_t * block_lbegin = g_matrix_block.lbegin();
    value_t * block_lend   = g_matrix_block.lend();
    DASH_LOG_DEBUG("MatrixTest.DelayedAlloc",
                   "local block idx:",   lbi,
                   "block offset:",      g_matrix_block.offsets(),
                   "block extents:",     g_matrix_block.extents(),
                   "block lend-lbegin:", block_lend - block_lbegin);
    // element phase, canonical element offset in block:
    index_t phase = 0;
    for (auto lbv = block_lbegin; lbv != block_lend; ++lbv, ++phase) {
      *lbv = myid + (0.01 * lbi) + (0.0001 * phase);
    }
  }
  matrix.barrier();

  if (myid == 0) {
    dash::test::print_matrix("Matrix<2>", matrix, 4);
    DASH_LOG_TRACE_VAR("GlobStencilIterTest.Conversion", teamspec.extents());

    std::array<index_t, 2> g_block_coords = {{
                             static_cast<index_t>(num_tiles_rows / 2),
                             static_cast<index_t>(num_tiles_cols / 2)
                           }};
    // Define halo for five-point stencil:
    dash::HaloSpec<2> halospec({{ { -1, 1 }, { -1, 1 } }});
    auto matrix_block   = matrix.block(g_block_coords);
    // Phase of element in the center of the block:
    auto b_center_idx   = dash::CartesianIndexSpace<2>(
                            matrix.pattern().block(g_block_coords).extents())
                          .at(tilesize_rows / 2,
                              tilesize_cols / 2);
    auto g_view_it      = matrix_block.begin() + b_center_idx;
    auto g_view_it_lpos = g_view_it.lpos();
    // Convert global view iterator to global stencil iterator:
    dash::GlobStencilIter<value_t, pattern_t> g_stencil_it(
                                                g_view_it,
                                                halospec);

    auto halo_view      = g_stencil_it.halo();
    ASSERT_EQ_U(stencil_points,   halo_view.npoints());
    ASSERT_EQ_U(stencil_points-1, halo_view.size());

    value_t north       = g_stencil_it.halo_cell({{ -1, 0 }});
    value_t east        = g_stencil_it.halo_cell({{  0, 1 }});
    value_t south       = g_stencil_it.halo_cell({{  1, 0 }});
    value_t west        = g_stencil_it.halo_cell({{  0,-1 }});

    LOG_MESSAGE("gvit = m.block(%d,%d).begin(), "
                "gvit.pos:%d gvit.gpos:%d gvit.rpos:%d gvit.lpos:(u:%d li:%d) "
                "value:%f halo(n:%f e:%f s:%f w:%f)",
                g_block_coords[0],
                g_block_coords[1],
                g_view_it.pos(),
                g_view_it.gpos(),
                g_view_it.rpos(),
                g_view_it_lpos.unit.id,
                g_view_it_lpos.index,
                static_cast<value_t>(*g_stencil_it),
                north, east, south, west);
  }
}

TEST_F(GlobStencilIterTest, FivePoint2DimHaloBlock)
{
  //TODO adapt test to new HaloBlock
#if 0
  typedef double                         value_t;
  typedef dash::TilePattern<2>           pattern_t;
  typedef typename pattern_t::index_type index_t;
  typedef typename pattern_t::size_type  extent_t;

  auto     myid      = dash::myid();
  extent_t num_units = dash::size();

  if (num_units < 4 || num_units % 2 != 0) {
    LOG_MESSAGE("GlobStencilIterTest.HaloBlock requires at least 4 units");
    return;
  }

  // Default constructor creates team spec with extents (nunits, 1):
  dash::TeamSpec<2> teamspec;
  // Automatic balancing of team spec in two dimensions:
  teamspec.balance_extents();

  extent_t tilesize_rows   = 4;
  extent_t tilesize_cols   = 3;
  extent_t num_units_rows  = teamspec.extent(0);
  extent_t num_units_cols  = teamspec.extent(1);
  extent_t num_tiles_rows  = num_units_rows > 1 ? num_units_rows * 2 : 1;
  extent_t num_tiles_cols  = num_units_cols > 1 ? num_units_cols * 3 : 1;
  extent_t matrix_rows     = tilesize_rows * num_tiles_rows;
  extent_t matrix_cols     = tilesize_cols * num_tiles_cols;
  extent_t halo_rows       = tilesize_rows;
  extent_t halo_cols       = tilesize_cols;
  extent_t block_halo_size = 2 * halo_rows + 2 * (halo_cols - 2);
  extent_t stencil_points  = 5;

  pattern_t pattern(
    dash::SizeSpec<2>(
      matrix_rows,
      matrix_cols),
    dash::DistributionSpec<2>(
      num_units_rows < 2 ? dash::NONE : dash::TILE(tilesize_rows),
      num_units_cols < 2 ? dash::NONE : dash::TILE(tilesize_cols)),
    teamspec);

  dash::Matrix<value_t, 2, index_t, pattern_t> matrix(pattern);

  // Initialize values:
  auto n_local_blocks = pattern.local_blockspec().size();
  for (extent_t lbi = 0; lbi < n_local_blocks; ++lbi) {
    // submatrix view on local block obtained from matrix relative to global
    // memory space:
    auto g_matrix_block    = matrix.local.block(lbi);
    value_t * block_lbegin = g_matrix_block.lbegin();
    value_t * block_lend   = g_matrix_block.lend();
    // element phase, canonical element offset in block:
    index_t phase = 0;
    for (auto lbv = block_lbegin; lbv != block_lend; ++lbv, ++phase) {
      *lbv = myid + (0.01 * lbi) + (0.0001 * phase);
    }
  }
  matrix.barrier();

  if (myid == 0) {
    dash::test::print_matrix("Matrix<2>", matrix, 4);
  }

  matrix.barrier();
  // clear matrix values:
  dash::fill(matrix.begin(), matrix.end(), 0.0);

  value_t halo_n_value = 1.1111;
  value_t halo_s_value = 2.2222;
  value_t halo_w_value = 3.3333;
  value_t halo_e_value = 4.4444;
  value_t bnd_n_value  = 0.1;
  value_t bnd_s_value  = 0.2;
  value_t bnd_w_value  = 0.3;
  value_t bnd_e_value  = 0.4;

  if (myid == 0) {
    DASH_LOG_TRACE_VAR("GlobStencilIterTest.HaloBlock", teamspec.extents());
    DASH_LOG_TRACE_VAR("GlobStencilIterTest.HaloBlock", pattern.extents());
    DASH_LOG_TRACE_VAR("GlobStencilIterTest.HaloBlock", pattern.blocksize(0));
    DASH_LOG_TRACE_VAR("GlobStencilIterTest.HaloBlock", pattern.blocksize(1));
    DASH_LOG_TRACE_VAR("GlobStencilIterTest.HaloBlock",
                       pattern.blockspec().extents());

    std::array<index_t, 2> g_block_coords = {{
                             static_cast<index_t>(num_tiles_rows / 2),
                             static_cast<index_t>(num_tiles_cols / 2)
                           }};
    // Define halo for five-point stencil:
    dash::HaloSpec<2> halospec({{ { -1, 1 }, { -1, 1 } }});

    ASSERT_EQ_U(1, halospec.width(0));
    ASSERT_EQ_U(1, halospec.width(1));

    auto matrix_block = matrix.block(g_block_coords);
    auto block_view   = matrix_block.viewspec();

    // write values in halo- and boundary cells only:
    DASH_LOG_TRACE("GlobStencilIterTest.HaloBlock",
                   "write to north and south boundary- and halo cells.",
                   "halospec rows:", halospec.width(0),
                   "block columns:", matrix_block.extent(1));
    for (int hi = 0; hi < halospec.width(0); ++hi) {
      for (int hj = 0; hj < matrix_block.extent(1); ++hj) {
        DASH_LOG_TRACE("GlobStencilIterTest.HaloBlock",
                       "( hi:", hi, "hj:", hj, ")");
        // write values into north halo:
        auto nhi = block_view.offset(0) - halospec.width(0) + hi;
        auto nhj = block_view.offset(1) + hj;
        matrix[nhi][nhj] = halo_n_value;
        // write values into south halo:
        auto shi = block_view.offset(0) + block_view.extent(0) + hi;
        auto shj = block_view.offset(1) + hj;
        matrix[shi][shj] = halo_s_value;
        // write values into north boundary:
        auto nbi = block_view.offset(0) + hi;
        auto nbj = block_view.offset(1) + hj;
        matrix[nbi][nbj] = bnd_n_value;
        // write values into south boundary:
        auto sbi = block_view.offset(0) + block_view.extent(0) -
                   halospec.width(0) + hi;
        auto sbj = block_view.offset(1) + hj;
        matrix[sbi][sbj] = bnd_s_value;
      }
    }
    DASH_LOG_TRACE("GlobStencilIterTest.HaloBlock",
                   "write to west and east boundary- and halo cells.",
                   "block rows:",       matrix_block.extent(0),
                   "halospec columns:", halospec.width(1));
    for (int hi = 0; hi < matrix_block.extent(0); ++hi) {
      for (int hj = 0; hj < halospec.width(1); ++hj) {
        DASH_LOG_TRACE("GlobStencilIterTest.HaloBlock",
                       "( hi:", hi, "hj:", hj, ")");
        // write values into west halo:
        auto whi = block_view.offset(0) + hi;
        auto whj = block_view.offset(1) - halospec.width(1) + hj;
        matrix[whi][whj] = halo_w_value;
        // write values into east halo:
        auto ehi = block_view.offset(0) + hi;
        auto ehj = block_view.offset(1) + block_view.extent(1) + hj;
        matrix[ehi][ehj] = halo_e_value;
        // write values into west boundary:
        auto bi_max = block_view.offset(0) + block_view.extent(0) -
                      halospec.width(0);
        auto wbi = block_view.offset(0) + hi + halospec.width(0);
        auto wbj = block_view.offset(1) + hj;
        if (wbi < bi_max) {
          matrix[wbi][wbj] = bnd_w_value;
        }
        // write values into east boundary:
        auto ebi = block_view.offset(0) + hi + halospec.width(0);
        auto ebj = block_view.offset(1) + block_view.extent(1) -
                   halospec.width(1)+ hj;
        if (ebi < bi_max) {
          matrix[ebi][ebj] = bnd_e_value;
        }
      }
    }
    dash::test::print_matrix("Matrix<2>", matrix, 2);

    dash::HaloBlock<value_t, pattern_t> halo_block(
                                          &matrix.begin().globmem(),
                                          pattern,
                                          block_view,
                                          halospec);
    auto block_boundary   = halo_block.boundary();
    auto block_bnd_begin  = block_boundary.begin();
    auto block_bnd_end    = block_boundary.end();
    auto block_halo       = halo_block.halo();
    auto block_halo_begin = block_halo.begin();
    auto block_halo_end   = block_halo.end();
    // inner cells in boundary:
    auto exp_bnd_size     = (tilesize_rows * 2) + ((tilesize_cols-2) * 2);
    // outer cells in halo:
    auto exp_halo_size    = (tilesize_rows * 2) + (tilesize_cols * 2);

    std::array<index_t, 2>  inner_block_offsets = {{
        block_view.offset(0) + halospec.offset_range(0).min,
        block_view.offset(1) + halospec.offset_range(1).min
    }};
    std::array<extent_t, 2> inner_block_extents = {{
        block_view.extent(0) + halospec.width(0) * 2,
        block_view.extent(1) + halospec.width(1) * 2
    }};
    dash::ViewSpec<2> exp_inner_view(block_view);
    dash::ViewSpec<2> exp_outer_view(inner_block_offsets,
                                     inner_block_extents);

    DASH_LOG_TRACE("GlobStencilIterTest.HaloBlock", "original block:",
                   "size:",          block_view.size(),
                   "offsets:",       block_view.offsets(),
                   "extents:",       block_view.extents());
    DASH_LOG_TRACE("GlobStencilIterTest.HaloBlock", "inner block view:",
                   "boundary size:", block_boundary.size(),
                   "offsets:",       halo_block.inner().offsets(),
                   "extents:",       halo_block.inner().extents());
    DASH_LOG_TRACE("GlobStencilIterTest.HaloBlock", "outer block view:",
                   "halo size:",     block_halo.size(),
                   "offsets:",       halo_block.outer().offsets(),
                   "extents:",       halo_block.outer().extents());

    ASSERT_EQ_U(exp_bnd_size,   block_boundary.size());
    ASSERT_EQ_U(exp_bnd_size,   block_bnd_end - block_bnd_begin);
    ASSERT_EQ_U(exp_halo_size,  block_halo.size());
    ASSERT_EQ_U(exp_halo_size,  block_halo_end - block_halo_begin);
    ASSERT_EQ_U(exp_inner_view, halo_block.inner());
    ASSERT_EQ_U(exp_outer_view, halo_block.outer());

    print_region<value_t>("block_halo", block_halo_begin, block_halo_end);
    print_region<value_t>("block_halo", block_bnd_begin,  block_bnd_end);

    // create local copy of halo regions and validate values:

    auto h_region_n = halo_block.halo_region({ -1,  0 });
    auto h_region_s = halo_block.halo_region({  1,  0 });
    auto h_region_w = halo_block.halo_region({  0, -1 });
    auto h_region_e = halo_block.halo_region({  0,  1 });

    print_region<value_t>("halo_n_region",
                          h_region_n.begin(), h_region_n.end());
    print_region<value_t>("halo_s_region",
                          h_region_s.begin(), h_region_s.end());
    print_region<value_t>("halo_w_region",
                          h_region_w.begin(), h_region_w.end());
    print_region<value_t>("halo_e_region",
                          h_region_e.begin(), h_region_e.end());
#endif
    // validate values in halo cells:

    // TODO: Yields incorrect failures for uneven tiles:
#if 0
    ASSERT_EQ_U(std::count(
                  h_region_n.begin(), h_region_n.end(), halo_n_value),
                tilesize_cols);
    ASSERT_EQ_U(std::count(
                  h_region_s.begin(), h_region_s.end(), halo_s_value),
                tilesize_cols);
    ASSERT_EQ_U(std::count(
                  h_region_w.begin(), h_region_w.end(), halo_w_value),
                tilesize_rows);
    ASSERT_EQ_U(std::count(
                  h_region_e.begin(), h_region_e.end(), halo_e_value),
                tilesize_rows);
#endif
#if 0
  }
#endif
}

