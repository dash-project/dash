#ifndef DASH__EXAMPLES__PATTERN_PARAMS_H__INCLUDED
#define DASH__EXAMPLES__PATTERN_PARAMS_H__INCLUDED

typedef dash::default_extent_t  extent_t;
typedef dash::default_index_t   index_t;

typedef struct cli_params_t {
  std::string             type           = "tile";
  std::array<extent_t, 2> size       {{ 12, 12 }};
  std::array<extent_t, 2> tile       {{  3,  4 }};
  std::array<extent_t, 2> units;
  bool                    blocked_display = false;
  bool                    balance_extents = false;
  bool                    cout            = false;
} cli_params;

cli_params default_params;

void print_usage(char **argv);
cli_params parse_args(int argc, char * argv[]);
void print_params(const cli_params & params);


cli_params parse_args(int argc, char * argv[], const cli_params & defaults)
{
  cli_params params = defaults;

  for (auto i = 1; i < argc; i += 3) {
    std::string flag = argv[i];
    if (flag == "-h") {
      print_usage(argv);
      exit(EXIT_SUCCESS);
    }
    if (flag == "-s") {
      params.type    = argv[i+1];
      i -= 1;
    } else if (flag == "-n") {
      params.size  = { static_cast<extent_t>(atoi(argv[i+1])),
                       static_cast<extent_t>(atoi(argv[i+2])) };
    } else if (flag == "-u") {
      params.units = { static_cast<extent_t>(atoi(argv[i+1])),
                       static_cast<extent_t>(atoi(argv[i+2])) };
    } else if (flag == "-t") {
      params.tile  = { static_cast<extent_t>(atoi(argv[i+1])),
                       static_cast<extent_t>(atoi(argv[i+2])) };
    } else if (flag == "-b") {
      params.balance_extents = true;
      i -= 2;
    } else {
      print_usage(argv);
      exit(EXIT_FAILURE);
    }
  }
  return params;
}

cli_params parse_args(int argc, char * argv[]) {
  return parse_args(argc, argv, default_params);
}

void print_usage(char **argv)
{
  if (dash::myid() == 0) {
    std::cerr << "Usage: \n"
         << basename(argv[0]) << " "
         << "-h | "
         << "[-s pattern] "
         << "[-n size_spec] "
         << "[-u unit_spec] "
         << "[-t tile_spec] "
         << "[-p] "
         << "\n\n";
    std::cerr << "-s pattern:   [summa|block|tile|seq|shift]\n"
         << "-n size_spec: <size_y>  <size_x>  [ "
         << default_params.size[0] << " " << default_params.size[1] << " ]\n"
         << "-u unit_spec: <units_y> <units_x> [  "
         << default_params.units[0] << "  " << default_params.units[1] << " ]\n"
         << "-t tile_spec: <tile_y>  <tile_x>  [ automatically determined ]\n"
         << "-p          : print to stdout instead of stderr\n"
         << "-h          : print help and exit"
         << std::endl;
  }
}

void print_params(const cli_params & params)
{
  int w_size  = std::log10(std::max(params.size[0],  params.size[1]));
  int w_units = std::log10(std::max(params.units[0], params.units[1]));
  int w_tile  = std::log10(std::max(params.tile[0],  params.tile[1]));
  int w       = std::max({ w_size, w_units, w_tile }) + 1;

  std::cerr << "Parameters:\n"
       << "    type (-s):                 " << params.type
       << '\n'
       << "    size (-n <rows> <cols>): ( "
       << std::fixed << std::setw(w) << params.size[0] << ", "
       << std::fixed << std::setw(w) << params.size[1] << " )"
       << '\n'
       << "    team (-u <rows> <cols>): ( "
       << std::fixed << std::setw(w) << params.units[0] << ", "
       << std::fixed << std::setw(w) << params.units[1] << " )"
       << '\n'
       << "    balance extents (-e): "
       << (params.balance_extents ? "yes" : "no")
       << '\n'
       << "    tile (-t <rows> <cols>): ( "
       << std::fixed << std::setw(w) << params.tile[0] << ", "
       << std::fixed << std::setw(w) << params.tile[1] << " )"
       << '\n'
       << std::endl;
}

dash::TilePattern<2, dash::ROW_MAJOR, index_t>
make_summa_pattern(
  const cli_params                  & params,
  const dash::SizeSpec<2, extent_t> & sizespec,
  const dash::TeamSpec<2, index_t>  & teamspec)
{
  auto pattern = dash::make_pattern<
                   dash::summa_pattern_partitioning_constraints,
                   dash::summa_pattern_mapping_constraints,
                   dash::summa_pattern_layout_constraints >(
                     sizespec,
                     teamspec);

  if (params.tile[1] >= 0 || params.tile[0] >= 0) {
    // change tile sizes of deduced pattern:
    typedef decltype(pattern) pattern_t;
    pattern_t custom_pattern(sizespec,
                             dash::DistributionSpec<2>(
                               params.tile[0] > 0
                               ? dash::TILE(params.tile[0])
                               : dash::NONE,
                               params.tile[1] > 0
                               ? dash::TILE(params.tile[1])
                               : dash::NONE),
                             teamspec);
    pattern = custom_pattern;
  }
  return pattern;
}

dash::ShiftTilePattern<2, dash::ROW_MAJOR, index_t>
make_shift_tile_pattern(
  const cli_params                  & params,
  const dash::SizeSpec<2, extent_t> & sizespec,
  const dash::TeamSpec<2, index_t>  & teamspec)
{
  // Example: -n 1680 1680 -u 28 1 -t 60 60
  typedef dash::ShiftTilePattern<2> pattern_t;
  pattern_t pattern(sizespec,
                    dash::DistributionSpec<2>(
                      dash::TILE(params.tile[0]),
                      dash::TILE(params.tile[1])),
                    teamspec);
  return pattern;
}

dash::SeqTilePattern<2, dash::ROW_MAJOR, index_t>
make_seq_tile_pattern(
  const cli_params                  & params,
  const dash::SizeSpec<2, extent_t> & sizespec,
  const dash::TeamSpec<2, index_t>  & teamspec)
{
  // Example: -n 30 30 -u 4 1 -t 10 10
  typedef dash::SeqTilePattern<2> pattern_t;
  pattern_t pattern(sizespec,
                    dash::DistributionSpec<2>(
                      dash::TILE(params.tile[0]),
                      dash::TILE(params.tile[1])),
                    teamspec);
  return pattern;
}

dash::TilePattern<2, dash::ROW_MAJOR, index_t>
make_tile_pattern(
  const cli_params                  & params,
  const dash::SizeSpec<2, extent_t> & sizespec,
  const dash::TeamSpec<2, index_t>  & teamspec)
{
  // Example: -n 30 30 -u 4 1 -t 10 10
  typedef dash::TilePattern<2> pattern_t;
  pattern_t pattern(sizespec,
                    dash::DistributionSpec<2>(
                      dash::TILE(params.tile[0]),
                      dash::TILE(params.tile[1])),
                    teamspec);
  return pattern;
}

dash::Pattern<2, dash::ROW_MAJOR, index_t>
make_block_pattern(
  const cli_params                  & params,
  const dash::SizeSpec<2, extent_t> & sizespec,
  const dash::TeamSpec<2, index_t>  & teamspec)
{
  // Example: -n 30 30 -u 4 1 -t 10 10
  typedef dash::Pattern<2> pattern_t;
  pattern_t pattern(sizespec,
                    dash::DistributionSpec<2>(
                      (params.tile[0] > 0
                       ? dash::BLOCKCYCLIC(params.tile[0])
                       : dash::NONE),
                      (params.tile[1] > 0
                       ? dash::BLOCKCYCLIC(params.tile[1])
                       : dash::NONE)),
                    teamspec);
  return pattern;
}

#endif // DASH__EXAMPLES__PATTERN_PARAMS_H__INCLUDED
