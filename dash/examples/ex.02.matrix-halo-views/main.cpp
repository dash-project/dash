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
  using dash::local;
  using dash::expand;
  using dash::shift;
  using dash::sub;
  using dash::blocks;
  using dash::index;

  dash::init(&argc, &argv);

  auto myid   = dash::myid();
  auto nunits = dash::size();

  const size_t block_size_x  = 2;
  const size_t block_size_y  = 2;
  size_t num_blocks_x        = nunits - 1;
  size_t num_blocks_y        = nunits;
  size_t extent_x            = block_size_x * num_blocks_x;
  size_t extent_y            = block_size_y * num_blocks_y;

  typedef dash::TilePattern<2>         pattern_t;
  typedef typename pattern_t::index_type    index_t;
  typedef float                             value_t;

  dash::TeamSpec<2> teamspec(dash::Team::All());
  teamspec.balance_extents();

  pattern_t pattern(
    dash::SizeSpec<2>(
      extent_y,
      extent_x),
    dash::DistributionSpec<2>(
      dash::TILE(block_size_y),
      dash::TILE(block_size_x)),
    teamspec
  );

  dash::Matrix<value_t, 2, dash::default_index_t, pattern_t>
    matrix(pattern);

  // Initialize matrix values:
  int li = 0;
  std::generate(matrix.lbegin(),
                matrix.lend(),
                [&]() {
                  return dash::myid() + 0.01 * li++;
                });
  dash::barrier();

  if (myid == 0) {
    auto matrix_view = dash::sub(0, matrix.extents()[0], matrix);
    print("matrix" << nview_str(matrix_view, 4));

    auto matrix_blocks = dash::blocks(matrix);
    auto matrix_b_idx  = matrix_blocks | dash::index();
    int  b_idx         = 0;
    for (const auto & m_block : matrix_blocks) {
      auto b_offsets = m_block.offsets();
      auto b_extents = m_block.extents();

      // matrix block view:
      STEP("\n-- matrix | block[" << matrix_b_idx[b_idx] << "]:" <<
            "\n       " <<
            "offsets: " << b_offsets[0] << "," << b_offsets[1] << " " <<
            "extents: " << b_extents[0] << "," << b_extents[1] <<
            nview_str(m_block, 4));

      // matrix block halo view:
      auto b_halo = m_block | dash::expand<0>(-1, 1)
                            | dash::expand<1>(-1, 1);

      auto bh_offsets = b_halo.offsets();
      auto bh_extents = b_halo.extents();
      STEP("   matrix | block[" << matrix_b_idx[b_idx] << "] | " <<
            "expand({ -1,1 }, { -1,1 }):" <<
            "\n       " <<
            "offsets: " << bh_offsets[0] << "," << bh_offsets[1] << " " <<
            "extents: " << bh_extents[0] << "," << bh_extents[1] <<
            nview_str(b_halo, 4));

#if 0
      auto bh_blocks = b_halo | dash::blocks();
      for (const auto & bh_block : bh_blocks) {
        STEP("  -- matrix | block[" << matrix_b_idx[b_idx] << "] | " <<
              "expand({ -1,1 }, { -1,1 }) | block(n)" <<
              nview_str(bh_block, 4));
      }
#endif

      // matrix shifted block halo:
      auto b_halo_s    = b_halo | shift<1>(1);

      auto bhs_offsets = b_halo_s.offsets();
      auto bhs_extents = b_halo_s.extents();
      STEP("   matrix | block[" << matrix_b_idx[b_idx] << "] | " <<
            "expand({ -1,1 }, { -1,1 }) | shift(1):" <<
            "\n       " <<
            "offsets: " << bhs_offsets[0] << "," << bhs_offsets[1] << " " <<
            "extents: " << bhs_extents[0] << "," << bhs_extents[1] <<
            nview_str(b_halo_s, 4));
      ++b_idx;
    }
  }
  dash::barrier();

  dash::finalize();
  return EXIT_SUCCESS;
}
