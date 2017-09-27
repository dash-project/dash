
#include <libdash.h>
#include "../util.h"
#include "../pattern_params.h"

#include <vector>
#include <iostream>

using std::cout;
using std::endl;

using namespace dash;

template <typename MatrixT>
void run_example(MatrixT & matrix);


int main(int argc, char **argv)
{
  using dash::sub;
  using dash::local;
  using dash::index;
  using dash::blocks;

  typedef dash::TilePattern<2>           pattern_t;
  typedef typename pattern_t::index_type   index_t;
  typedef float                            value_t;

  int blocksize_x = 2;
  int blocksize_y = 2;

  dash::init(&argc, &argv);

  auto myid   = dash::myid();
  auto nunits = dash::size();

  auto params = parse_args(argc, argv);

  if (dash::myid() == 0) {
    print_params(params);

    try {
      dash::SizeSpec<2, extent_t> sizespec(params.size[0],  params.size[1]);
      dash::TeamSpec<2, index_t>  teamspec(params.units[0], params.units[1]);

      if(params.balance_extents) {
        teamspec.balance_extents();
      }
      if (params.tile[0] < 0 && params.tile[1] < 0) {
        auto max_team_extent = std::max(teamspec.extent(0),
                                        teamspec.extent(1));
        params.tile[0] = sizespec.extent(0) / max_team_extent;
        params.tile[1] = sizespec.extent(1) / max_team_extent;
      }
      if (params.type == "summa") {
        auto pattern = make_summa_pattern(params, sizespec, teamspec);
        std::cout << "Pattern type:\n   "
                  << pattern_to_string(pattern) << std::endl;
        dash::Matrix<value_t, 2, index_t, decltype(pattern)>
          matrix(pattern);
        run_example(matrix);
      } else if (params.type == "block") {
        auto pattern = make_block_pattern(params, sizespec, teamspec);
        std::cout << "Pattern type:\n   "
                  << pattern_to_string(pattern) << std::endl;
        dash::Matrix<value_t, 2, index_t, decltype(pattern)>
          matrix(pattern);
        run_example(matrix);
      } else if (params.type == "tile") {
        auto pattern = make_tile_pattern(params, sizespec, teamspec);
        std::cout << "Pattern type:\n   "
                  << pattern_to_string(pattern) << std::endl;
        dash::Matrix<value_t, 2, index_t, decltype(pattern)>
          matrix(pattern);
        run_example(matrix);
      } else if (params.type == "shift") {
        auto pattern = make_shift_tile_pattern(params, sizespec, teamspec);
        std::cout << "Pattern type:\n   "
                  << pattern_to_string(pattern) << std::endl;
        dash::Matrix<value_t, 2, index_t, decltype(pattern)>
          matrix(pattern);
        run_example(matrix);
      } else if (params.type == "seq") {
        auto pattern = make_seq_tile_pattern(params, sizespec, teamspec);
        std::cout << "Pattern type:\n   "
                  << pattern_to_string(pattern) << std::endl;
        dash::Matrix<value_t, 2, index_t, decltype(pattern)>
          matrix(pattern);
        run_example(matrix);
      } else {
        print_usage(argv);
        exit(EXIT_FAILURE);
      }
    } catch (std::exception & excep) {
      std::cerr << excep.what() << std::endl;
    }
  }

#if 0
  dash::TeamSpec<2>         teamspec(dash::Team::All());
  teamspec.balance_extents();

  dash::DistributionSpec<2> block_ds(dash::BLOCKED,
                                     dash::BLOCKED);
  dash::DistributionSpec<2> tiled_ds(dash::TILE(blocksize_y),
                                     dash::TILE(blocksize_x));
  dash::SizeSpec<2>         sizespec((teamspec.extent(0) * 2) * blocksize_y,
                                     (teamspec.extent(1) * 2) * blocksize_x);

  pattern_t block_pattern(
    sizespec,
    block_ds,
    teamspec
  );

  pattern_t tiled_pattern(
    sizespec,
    tiled_ds,
    teamspec
  );

  // Block-distributed Matrix:
  dash::Matrix<value_t, 2, index_t, pattern_t>
    matrix_blocked(block_pattern);

  // Block-distributed Matrix:
  dash::Matrix<value_t, 2, index_t, pattern_t>
    matrix_tiled(tiled_pattern);

  run_example(matrix_blocked);
  run_example(matrix_tiled);
#endif
  dash::finalize();

  return 0;
}

template <typename MatrixT>
void run_example(MatrixT & matrix) {
  typedef typename MatrixT::pattern_type pattern_t;
  typedef typename pattern_t::index_type   index_t;
  typedef typename MatrixT::value_type     value_t;

  int li = 0;
  std::generate(matrix.lbegin(),
                matrix.lend(),
                [&]() {
                  return dash::myid() + 0.01 * li++;
                });

  dash::barrier();

  if (dash::myid() == 0) {
    print("matrix:" <<
          nview_str(matrix | sub(0,matrix.extents()[0])));

    std::vector<value_t> tmp(6);
    auto copy_end = std::copy(matrix.begin() + 3,
                              matrix.begin() + 9,
                              tmp.data());
    print("matrix.begin()[3...9]: " << tmp);
  }

  dash::barrier();

  auto l_matrix = matrix | local();

  print("matrix | local:" << nview_str(l_matrix));

  dash::barrier();

  // Copy local row
  {
    auto l_row = matrix | sub(0, matrix.extent(0)) | local() | sub(1,2);
    DASH_LOG_DEBUG("matrix.local.row(1)",
          "type:",     dash::typestr(l_row));

    print("matrix.local.row(1) " << nview_str(l_row));
  }
  matrix.barrier();

  // Copy local block
  {
    auto l_blocks = matrix | local() | blocks();
    print("matrix.local.blocks(): " <<
          "size: "    << l_blocks.size()    << " " <<
          "offsets: " << l_blocks.offsets() << " " <<
          "extents: " << l_blocks.extents());

    auto l_blocks_idx = l_blocks | index();

    int l_bi = 0;
    for (const auto & lb : l_blocks) {
      DASH_LOG_DEBUG("matrix.local.blocks()", "[", l_bi, "]",
                     "size:",    lb.size(),
                     "offsets:", lb.offsets(),
                     "extents:", lb.extents());

      print("matrix.local.block(" << l_bi << "): " <<
            "block[" << l_blocks_idx[l_bi] << "]" <<
            nview_str(lb));

      std::vector<value_t> tmp(lb.size());
      auto copy_end = dash::copy(lb,
                                 tmp.data());
      print("matrix.local.block(" << l_bi << ") copy: " << tmp);

      ++l_bi;
    }

  }
  matrix.barrier();
}

