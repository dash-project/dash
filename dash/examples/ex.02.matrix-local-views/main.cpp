
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

#define RUN_EXAMPLE(pattern_type__) do { \
  auto pattern = make_ ## pattern_type__ ## _pattern(   \
                   params, sizespec, teamspec);         \
  if (dash::myid() == 0) {                              \
    std::cout << "Pattern:\n   "                        \
              << pattern_to_string(pattern)             \
              << std::endl;                             \
  }                                                     \
  dash::Matrix<value_t, 2, index_t, decltype(pattern)>  \
    matrix(pattern);                                    \
  run_example(matrix);                                  \
} while(0)


int main(int argc, char **argv)
{
  using dash::sub;
  using dash::local;
  using dash::index;
  using dash::blocks;

  typedef dash::TilePattern<2>           pattern_t;
  typedef typename pattern_t::index_type   index_t;
  typedef float                            value_t;

  dash::init(&argc, &argv);

  auto myid   = dash::myid();
  auto nunits = dash::size();

  cli_params defaults = default_params;
  defaults.type            = "seq";
  defaults.size            = {{  8, 6 }};
  defaults.tile            = {{  0, 0 }};
  defaults.units           = {{  1, static_cast<unsigned>(nunits) }};
  defaults.blocked_display = false;
  defaults.balance_extents = false;
  defaults.cout            = false;

  auto params = parse_args(argc, argv, defaults);

  if (dash::myid() == 0) {
    print_params(params);
  }

  dash::SizeSpec<2, extent_t> sizespec(params.size[0],
                                       params.size[1]);
  dash::TeamSpec<2, index_t>  teamspec(params.units[0],
                                       params.units[1]);

  if(params.balance_extents) {
    teamspec.balance_extents();
  }
  if (params.tile[0] == 0 && params.tile[1] == 0) {
    auto max_team_extent = std::max(teamspec.extent(0),
                                    teamspec.extent(1));
    params.tile[0] = sizespec.extent(0) / max_team_extent;
    params.tile[1] = sizespec.extent(1) / max_team_extent;
  }

  if (params.type == "summa") {
    RUN_EXAMPLE(summa);
  } else if (params.type == "block") {
    RUN_EXAMPLE(block);
  } else if (params.type == "tile") {
    RUN_EXAMPLE(tile);
  } else if (params.type == "shift") {
    RUN_EXAMPLE(shift_tile);
  } else if (params.type == "seq") {
    RUN_EXAMPLE(seq_tile);
  } else {
    print_usage(argv);
    exit(EXIT_FAILURE);
  }

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

    std::vector<value_t> tmp(10);
    auto copy_end = std::copy(matrix.begin() + 11,
                              matrix.begin() + 21,
                              tmp.data());
    STEP("matrix.begin()[11...20]: " << range_str(tmp, 2));
  }

  dash::barrier();

  auto l_matrix = matrix | local();

  STEP("matrix | local() | index():" << dash::typestr(l_matrix));

  return;

  STEP("matrix | local() | index():" << dash::typestr(l_matrix | index()));
  STEP("matrix | local():" << nview_str(l_matrix));

  dash::barrier();

  // Copy local block
  {
    auto l_blocks = matrix | local() | blocks();
    STEP("-- matrix | local() | blocks(): " <<
          "size: "    << l_blocks.size()    << " " <<
          "offsets: " << l_blocks.offsets() << " " <<
          "extents: " << l_blocks.extents());

    int l_bi = 0;
    for (const auto & lb : l_blocks) {
      DASH_LOG_DEBUG("matrix | local() | blocks()", "[", l_bi, "]",
                     "size:",    lb.size(),
                     "offsets:", lb.offsets(),
                     "extents:", lb.extents());

      STEP("   matrix | local() | blocks()[" << l_bi << "]: " <<
            nview_str(lb));

      std::vector<value_t> tmp(lb.size());
      auto copy_end = std::copy(lb.begin(),
                                lb.end(),
                                tmp.data());
      STEP("   matrix | local() | blocks()[" << l_bi << "] copy: " <<
           range_str(tmp, 2));

      ++l_bi;
    }
    dash::barrier();
  }
}

