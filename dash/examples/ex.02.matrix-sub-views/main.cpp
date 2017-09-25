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
  using namespace dash;

  dash::init(&argc, &argv);

  auto myid   = dash::myid();
  auto nunits = dash::size();

  const size_t block_size_x  = 2;
  const size_t block_size_y  = 2;
  const size_t block_size    = block_size_x * block_size_y;
  size_t extent_x            = block_size_x * nunits;
  size_t extent_y            = block_size_y * nunits;

  typedef dash::TilePattern<2>           pattern_t;
  typedef typename pattern_t::index_type   index_t;
  typedef float                            value_t;

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

  int li = 0;
  std::generate(matrix.lbegin(),
                matrix.lend(),
                [&]() {
                  auto   u = dash::myid();
                  return u + 0.01 * li++;
                });

  dash::barrier();

  if (myid == 0) {
    print("matrix:" <<
          nview_str(matrix | sub(0,extent_y)) << '\n');
    print("matrix.local.size(): " << matrix.local.size());

    auto matrix_sub = matrix | sub<0>(3, extent_y-1)
                             | sub<1>(1, extent_x-1);

    print("matrix | sub<0>(3,-1) | sub<1>(1,-1)" <<
          nview_str(matrix_sub) << "\n\n");

    print("matrix | sub<0>(3,-1) | sub<1>(1,-1) | blocks()\n");
    {
      auto m_s_blocks     = matrix_sub | blocks();
      auto m_s_blocks_idx = m_s_blocks | index();
      int b_idx = 0;
      for (const auto & blk : m_s_blocks) {
        auto m_isect = matrix_sub | intersect(blk);

        print("--- matrix | sub {3,-1} {1-1} | " <<
              "block(" << m_s_blocks_idx[b_idx] << ") " <<
              nview_str(blk) << std::endl);

        ++b_idx;
      }
    }
    print("matrix | sub<0>(3,-1) | sub<1>(1,-1) | local | blocks()\n");
    {
      auto m_s_l_blocks     = matrix_sub   | local() | blocks();
      auto m_s_l_blocks_idx = m_s_l_blocks | index();
      int b_idx = 0;
      print("--- number of blocks: " << m_s_l_blocks.size());
      for (const auto & blk : m_s_l_blocks) {
        auto block_gidx = m_s_l_blocks_idx[b_idx];
        print("--- matrix | sub {3,-1} {1-1} | local | " <<
              "block(" << block_gidx << ") " <<
              "offsets: " << blk.offsets() << " " <<
              "extents: " << blk.extents());

//      print(nview_str(blk) << std::endl);

        ++b_idx;
      }
    }
  }
  dash::barrier();

  dash::finalize();
  return EXIT_SUCCESS;
};


