
#include <libdash.h>

typedef int extent_t;

typedef struct cli_params_t {
  std::array<extent_t, 2> domainsize {{ 12, 12 }};
  std::array<extent_t, 2> teamsize   {{ 3,   4 }};
  std::array<extent_t, 2> tilesize   {{ 3,   4 }};
  bool                    blocked_display = false;
  bool                    balance_extents = false;
  bool                    cout            = false;
} cli_params;

cli_params parse_args(int argc, char * argv[]) {
  cli_params params;
  return params;
}

int main(int argc, char * argv[])
{
  dash::init(&argc, &argv);


  

  dash::finalize();
  return EXIT_SUCCESS;
}

