#include <libdash.h>
#include <array>
#include <numeric>
#include <functional>
#include "TestBase.h"
#include "GlobStencilIterTest.h"

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
    LOG_MESSAGE("GlobStencilIter.Conversion requires at least 2 units");
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
    auto matrix_block   = matrix.block(
                            g_block_coords
                         // halospec
                          );
    // Phase of element in the center of the block:
    auto b_center_idx   = dash::CartesianIndexSpace<2>(
                            matrix.pattern().block(g_block_coords).extents())
                          .at(tilesize_rows / 2,
                              tilesize_cols / 2);
    auto g_view_it      = matrix_block.begin() + b_center_idx;
    auto g_view_it_lpos = g_view_it.lpos();
    // Convert global view iterator to global stencil iterator:
    dash::GlobStencilIter<value_t, pattern_t> g_stencil_it(
                                                g_view_it, halospec);

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
                g_view_it_lpos.unit,
                g_view_it_lpos.index,
                static_cast<value_t>(*g_stencil_it),
                north, east, south, west);
  }
}
