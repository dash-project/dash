#include <libdash.h>
#include "../util.h"

using std::cout;
using std::cerr;
using std::cin;
using std::endl;
using std::vector;

using uint = unsigned int;


int main(int argc, char *argv[])
{
  dash::init(&argc, &argv);

  auto myid   = dash::myid();
  auto nunits = dash::size();

  const size_t block_size_x  = 2;
  const size_t block_size_y  = 2;
  const size_t block_size    = block_size_x * block_size_y;
  size_t num_local_blocks_x  = 1;
  size_t num_local_blocks_y  = 2;
  size_t num_blocks_x        = nunits * num_local_blocks_x;
  size_t num_blocks_y        = nunits * num_local_blocks_y;
  size_t num_blocks_total    = num_blocks_x * num_blocks_y;
  size_t extent_x            = block_size_x * num_blocks_x;
  size_t extent_y            = block_size_y * num_blocks_y;
  size_t num_elem_total      = extent_x * extent_y;
  // Assuming balanced mapping:
  size_t num_elem_per_unit   = num_elem_total / nunits;
  size_t num_blocks_per_unit = num_elem_per_unit / block_size;

  typedef dash::ShiftTilePattern<2>         pattern_t;
  typedef typename pattern_t::index_type    index_t;
  typedef float                             value_t;

  pattern_t pattern(
    dash::SizeSpec<2>(
      extent_y,
      extent_x),
    dash::DistributionSpec<2>(
      dash::TILE(block_size_y),
      dash::TILE(block_size_x))
  );

  dash::Matrix<value_t, 2, dash::default_index_t, pattern_t>
    matrix(pattern);

  // Initialize matrix values:
  for (size_t lb = 0; lb < num_blocks_per_unit; ++lb) {
    auto lblock         = matrix.local.block(lb);
    auto lblock_view    = lblock.begin().viewspec();
    auto lblock_extents = lblock_view.extents();
    auto lblock_offsets = lblock_view.offsets();
    dash__unused(lblock_offsets);
    for (auto bx = 0; bx < static_cast<int>(lblock_extents[0]); ++bx) {
      for (auto by = 0; by < static_cast<int>(lblock_extents[1]); ++by) {
        // Phase coordinates (bx,by) to global coordinates (gx,gy):
        value_t value  = static_cast<value_t>(dash::myid().id)
                         + 0.01 * lb
                         + 0.0001 * (bx * lblock_extents[0] + by);
        lblock[bx][by] = value;
      }
    }
  }
  dash::barrier();

  if (myid == 0) {
    auto matrix_view = dash::sub(0, matrix.extents()[0], matrix);
    print("matrix" << nview_str(matrix_view, 4));

    DASH_LOG_DEBUG("MatrixViewsExample", "matrix",
                   "offsets:", matrix_view.offsets(),
                   "extents:", matrix_view.extents());

    auto matrix_blocks = dash::blocks(matrix);
    auto matrix_b_idx  = matrix_blocks | dash::index();
    int  b_idx         = 0;
    for (const auto & m_block : matrix_blocks) {
      auto b_offsets = m_block.offsets();
      auto b_extents = m_block.extents();
      // matrix block view:
      print("\n-- matrix | block[" << matrix_b_idx[b_idx] << "]:" <<
            "\n       " << dash::typestr(m_block) <<
            "\n       " <<
            "offsets: " << b_offsets[0] << "," << b_offsets[1] << " " <<
            "extents: " << b_extents[0] << "," << b_extents[1] <<
            nview_str(m_block, 4));
      // matrix block halo view:
      auto b_halo = m_block | dash::expand<0>(-1, 1)
                            | dash::expand<1>(-1, 1);

      auto bh_offsets = b_halo.offsets();
      auto bh_extents = b_halo.extents();
      print("\n-- matrix | block[" << matrix_b_idx[b_idx] << "] | " <<
            "expand({ -1,1 }, { -1,1 }):" <<
            "\n       " << dash::typestr(b_halo) <<
            "\n       " <<
            "offsets: " << bh_offsets[0] << "," << bh_offsets[1] << " " <<
            "extents: " << bh_extents[0] << "," << bh_extents[1] <<
            nview_str(b_halo, 4));
#if 0
      auto bh_blocks = b_halo | dash::blocks();
      for (const auto & bh_block : bh_blocks) {
        print("matrix | block[" << matrix_b_idx[b_idx] << "] | " <<
              "expand({ -1,1 }, { -1,1 }) | block(n)" <<
              nview_str(bh_block, 4));
      }
#endif
      ++b_idx;
    }
  }
  dash::barrier();

  dash::finalize();
  return EXIT_SUCCESS;
}
