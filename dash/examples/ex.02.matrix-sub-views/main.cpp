#include <libdash.h>
#include "../util.h"

#include <iostream>
#include <sstream>


int main(int argc, char *argv[])
{
  using namespace dash;

  typedef dash::SeqTilePattern<2>          pattern_t;
  typedef typename pattern_t::index_type   index_t;
  typedef float                            value_t;

  dash::init(&argc, &argv);

  auto myid   = dash::Team::All().myid();
  auto nunits = dash::size();

  const size_t block_size_x  = 2;
  const size_t block_size_y  = 2;
  const size_t block_size    = block_size_x * block_size_y;
  size_t extent_x            = block_size_x * (nunits + 1);
  size_t extent_y            = block_size_y * (nunits + 1);

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
                  return dash::myid() + 0.01 * li++;
                });

  dash::barrier();

  if (myid == 0) {
    print("matrix:" <<
          nview_str(matrix | sub(0,extent_y)) << '\n');

    STEP("sub<0>(3,-1) | sub<1>(1,-1)");

    auto matrix_sub = matrix | sub<0>(3, extent_y-1)
                             | sub<1>(1, extent_x-1);

    print(nview_str(matrix_sub) << "\n\n");

    STEP("sub<0>(3,-1) | sub<1>(1,-1) | blocks()");
    {
      auto m_s_blocks     = matrix_sub | blocks();
      auto m_s_blocks_idx = m_s_blocks | index();
      int b_idx = 0;
      for (const auto & blk : m_s_blocks) {
        STEP("block "  << std::left << std::setw(2) 
                       << m_s_blocks_idx[b_idx] << '\n');
        print("      " <<
              (blk.is_strided()      ? "strided, " : "contiguous, ") <<
              (blk.is_local_at(myid) ? "local"     : "remote") <<
              nview_str(blk) << std::endl);
        ++b_idx;
      }
    }

    STEP("sub<0>(3,-1) | sub<1>(1,-1) | local() | blocks()");
    {
      print("matrix | sub | local:" <<
            nview_str(matrix_sub | local()));
      print("matrix | sub | local: type: " <<
            dash::typestr(matrix_sub | local()));
      print("matrix | sub | local: strided: " <<
            (matrix_sub | local() | index()).is_strided());

      auto m_s_l_blocks     = matrix_sub   | local() | blocks();
      auto m_s_l_blocks_idx = m_s_l_blocks | index();
      print("matrix | sub | local | blocks: \n" <<
            "size: " << m_s_l_blocks.size());

      print("type:"         << dash::typestr(m_s_l_blocks));
      print("origin type: " << dash::typestr(
                                 dash::origin(m_s_l_blocks))
                            << std::endl);
      int b_idx = 0;

      for (const auto & blk : m_s_l_blocks) {
        STEP("sub<0>(3,-1) | sub<1>(1,-1) | local() | blocks()[b_idx]");
        auto block_gidx = m_s_l_blocks_idx[b_idx];
        print("--- block(" << block_gidx << ") " <<
              "offsets: " << blk.offsets() << " " <<
              "extents: " << blk.extents());

        print(nview_str(blk) << std::endl);

        ++b_idx;
      }
    }
  }

  dash::finalize();
  return EXIT_SUCCESS;
};


