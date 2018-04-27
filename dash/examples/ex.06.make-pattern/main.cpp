
#include <libdash.h>
#include "../util.h"
#include "../pattern_params.h"

using namespace dash;


int main(int argc, char * argv[])
{
  dash::init(&argc, &argv);

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
      } else if (params.type == "block") {
        auto pattern = make_block_pattern(params, sizespec, teamspec);
        std::cout << "Pattern type:\n   "
                  << pattern_to_string(pattern) << std::endl;
      } else if (params.type == "tile") {
        auto pattern = make_tile_pattern(params, sizespec, teamspec);
        std::cout << "Pattern type:\n   "
                  << pattern_to_string(pattern) << std::endl;
      } else if (params.type == "shift") {
        auto pattern = make_shift_tile_pattern(params, sizespec, teamspec);
        std::cout << "Pattern type:\n   "
                  << pattern_to_string(pattern) << std::endl;
      } else if (params.type == "seq") {
        auto pattern = make_seq_tile_pattern(params, sizespec, teamspec);
        std::cout << "Pattern type:\n   "
                  << pattern_to_string(pattern) << std::endl;
      } else {
        print_usage(argv);
        exit(EXIT_FAILURE);
      }
    } catch (std::exception & excep) {
      std::cerr << excep.what() << std::endl;
    }
  }
  dash::finalize();
  return EXIT_SUCCESS;
}

