#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <cstddef>
#include <cmath>

#include <libdash.h>

#include <dash/tools/PatternVisualizer.h>

#include <dash/util/PatternMetrics.h>

#include <dash/pattern/internal/PatternLogging.h>

using std::cout;
using std::cerr;
using std::endl;

using namespace dash;

typedef dash::default_index_t  index_t;
typedef dash::default_extent_t extent_t;

typedef struct cli_params_t {
  std::string type;
  extent_t    size_x;
  extent_t    size_y;
  extent_t    units_x;
  extent_t    units_y;
  int         tile_x;
  int         tile_y;
  bool        blocked_display;
  bool        cout;
} cli_params;

template<typename PatternType>
std::string pattern_to_string(
  const PatternType & pattern);

template<typename PatternType>
std::string pattern_to_filename(
  const PatternType & pattern);

cli_params parse_args(int argc, char * argv[]);
void print_params(const cli_params & params);

template<typename PatternT>
void print_pattern_metrics(const PatternT & pattern);

template<typename PatternT>
void print_example(
  const PatternT   & pattern,
  const cli_params & params)
{
  typedef typename PatternT::index_type index_t;

  auto pattern_file = pattern_to_filename(pattern);
  auto pattern_desc = pattern_to_string(pattern);
  print_pattern_metrics(pattern);

  dash::tools::PatternVisualizer<PatternT> pv(pattern);
  pv.set_title(pattern_desc);

  cerr << "Generating visualization of "
       << endl
       << "    " << pattern_desc
       << endl;

  if (params.cout) {
    pv.draw_pattern(std::cout, params.blocked_display);
  } else {
    cerr << "Image file:"
         << endl
         << "    " << pattern_file
         << endl;
    std::ofstream out(pattern_file);
    pv.draw_pattern(out, params.blocked_display);
    out.close();
  }
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

  if (params.tile_x >= 0 || params.tile_y >= 0) {
    // change tile sizes of deduced pattern:
    typedef decltype(pattern) pattern_t;
    pattern_t custom_pattern(sizespec,
                             dash::DistributionSpec<2>(
                               params.tile_x > 0
                               ? dash::TILE(params.tile_x)
                               : dash::NONE,
                               params.tile_y > 0
                               ? dash::TILE(params.tile_y)
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
  dash::ShiftTilePattern<2> pattern(sizespec,
                                    dash::DistributionSpec<2>(
                                      dash::TILE(params.tile_y),
                                      dash::TILE(params.tile_x)),
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
  dash::SeqTilePattern<2> pattern(sizespec,
                                  dash::DistributionSpec<2>(
                                    dash::TILE(params.tile_y),
                                    dash::TILE(params.tile_x)),
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
  dash::Pattern<2> pattern(sizespec,
                           dash::DistributionSpec<2>(
                             (params.tile_y > 0
                              ? dash::BLOCKCYCLIC(params.tile_y)
                              : dash::NONE),
                             (params.tile_x > 0
                              ? dash::BLOCKCYCLIC(params.tile_x)
                              : dash::NONE)),
                           teamspec);
  return pattern;
}

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);

  auto params = parse_args(argc, argv);
  print_params(params);

  if (dash::myid() == 0) {
    try {
      dash::SizeSpec<2, extent_t> sizespec(params.size_y,  params.size_x);
      dash::TeamSpec<2, index_t>  teamspec(params.units_y, params.units_x);

      if (params.tile_x < 0 && params.tile_y < 0) {
        auto max_team_extent = std::max(teamspec.extent(0),
                                        teamspec.extent(1));
        params.tile_y = sizespec.extent(0) / max_team_extent;
        params.tile_x = sizespec.extent(1) / max_team_extent;
      }

      if (params.type == "summa") {
        auto pattern = make_summa_pattern(params, sizespec, teamspec);
        print_example(pattern, params);
      } else if (params.type == "block") {
        auto pattern = make_block_pattern(params, sizespec, teamspec);
        print_example(pattern, params);
      } else if (params.type == "shift") {
        auto pattern = make_shift_tile_pattern(params, sizespec, teamspec);
        print_example(pattern, params);
      } else if (params.type == "seq") {
        auto pattern = make_seq_tile_pattern(params, sizespec, teamspec);
        print_example(pattern, params);
      } else {
        DASH_THROW(
          dash::exception::InvalidArgument,
          "Invalid argument '" << params.type << "' for pattern type, " <<
          "expected 'summa', 'shift', or 'seq'");
      }

    } catch (std::exception & excep) {
      cerr << excep.what() << endl;
    }
  }

  dash::finalize();

  return EXIT_SUCCESS;
}

cli_params parse_args(int argc, char * argv[])
{
  cli_params params;
  params.size_x  = 110;
  params.size_y  = 110;
  params.units_x = 10;
  params.units_y = 10;
  params.tile_x  = -1;
  params.tile_y  = -1;
  params.blocked_display = false;
  params.cout    = false;
  params.type    = "summa";

  for (auto i = 1; i < argc; i += 3) {
    std::string flag = argv[i];
    if (flag == "-s") {
      params.type    = argv[i+1];
      i -= 1;
    } else if (flag == "-n") {
      params.size_y  = static_cast<extent_t>(atoi(argv[i+1]));
      params.size_x  = static_cast<extent_t>(atoi(argv[i+2]));
    } else if (flag == "-u") {
      params.units_y = static_cast<extent_t>(atoi(argv[i+1]));
      params.units_x = static_cast<extent_t>(atoi(argv[i+2]));
    } else if (flag == "-t") {
      params.tile_y  = static_cast<extent_t>(atoi(argv[i+1]));
      params.tile_x  = static_cast<extent_t>(atoi(argv[i+2]));
    } else if (flag == "-p") {
      params.cout    = true;
      i -= 2;
    } else if (flag == "-b") {
      params.blocked_display = true;
      i -= 2;
    }
  }
  return params;
}

void print_params(const cli_params & params)
{
  int w_size  = std::log10(std::max(params.size_x,  params.size_y));
  int w_units = std::log10(std::max(params.units_x, params.units_y));
  int w_tile  = std::log10(std::max(params.tile_x,  params.tile_y));
  int w       = std::max({ w_size, w_units, w_tile }) + 1;

  cerr << "Parameters:"
       << endl
       << "    type (-s):               " << params.type
       << endl
       << "    size (-n <rows> <cols>): ( "
       << std::fixed << std::setw(w) << params.size_y << ", "
       << std::fixed << std::setw(w) << params.size_x << " )"
       << endl
       << "    team (-u <rows> <cols>): ( "
       << std::fixed << std::setw(w) << params.units_y << ", "
       << std::fixed << std::setw(w) << params.units_x << " )"
       << endl
       << "    tile (-t <rows> <cols>): ( "
       << std::fixed << std::setw(w) << params.tile_y << ", "
       << std::fixed << std::setw(w) << params.tile_x << " )"
       << endl
       << endl;
}

/**
 * Create string describing of pattern instance.
 */
template<typename PatternType>
std::string pattern_to_string(
  const PatternType & pattern)
{
  typedef typename PatternType::index_type index_t;

  dim_t ndim = pattern.ndim();

  std::string storage_order = pattern.memory_order() == ROW_MAJOR
                              ? "ROW_MAJOR"
                              : "COL_MAJOR";

  std::array<index_t, 2> blocksize;
  blocksize[0] = pattern.blocksize(0);
  blocksize[1] = pattern.blocksize(1);

  std::ostringstream ss;
  ss << "dash::"
     << PatternType::PatternName
     << "<"
     << ndim << ","
     << storage_order << ","
     << typeid(index_t).name()
     << ">(" << endl
     << "        SizeSpec:  " << pattern.sizespec().extents()  << "," << endl
     << "        TeamSpec:  " << pattern.teamspec().extents()  << "," << endl
     << "        BlockSpec: " << pattern.blockspec().extents() << "," << endl
     << "        BlockSize: " << blocksize << " )";

  return ss.str();
}

/**
 * Create filename describing of pattern instance.
 */
template<typename PatternType>
std::string pattern_to_filename(
  const PatternType & pattern)
{
  typedef typename PatternType::index_type index_t;

  dim_t ndim = pattern.ndim();

  std::string storage_order = pattern.memory_order() == ROW_MAJOR
                              ? "ROW_MAJOR"
                              : "COL_MAJOR";

  auto sspc = pattern.sizespec();
  auto tspc = pattern.teamspec();
  auto bspc = pattern.blockspec();

  std::ostringstream ss;
  ss << PatternType::PatternName
     << "--"
     << ndim << "-"
     << storage_order << "-"
     << typeid(index_t).name()
     << "--"
     << "size-"   << sspc.extent(0) << "x" << sspc.extent(1) << "--"
     << "team-"   << tspc.extent(0) << "x" << tspc.extent(1) << "--"
     << "blocks-" << bspc.extent(0) << "x" << bspc.extent(1)
     << ".svg";

  return ss.str();
}

template<typename PatternT>
void print_pattern_metrics(const PatternT & pattern)
{
  dash::util::PatternMetrics<PatternT> pm(pattern);

  size_t block_kbytes = pattern.blocksize(0) * pattern.blocksize(1) *
                        sizeof(double) /
                        1024;

  cerr << "Pattern Metrics:"
       << endl
       << "    Partitioning:"
       << endl
       << "        block size:         " << block_kbytes << " KB"
       << endl
       << "        number of blocks:   " << pm.num_blocks()
       << endl
       << "    Mapping imbalance:"
       << endl
       << "        min. blocks/unit:   " << pm.min_blocks_per_unit()
       << " = " << pm.min_elements_per_unit() << " elements"
       << endl
       << "        max. blocks/unit:   " << pm.max_blocks_per_unit()
       << " = " << pm.max_elements_per_unit() << " elements"
       << endl
       << "        imbalance factor:   " << std::setprecision(4)
                                         << pm.imbalance_factor()
       << endl
       << "        balanced units:     " << pm.num_balanced_units()
       << endl
       << "        imbalanced units:   " << pm.num_imbalanced_units()
       << endl
       << endl;
}

template<typename PatternT>
void log_pattern_mapping(const PatternT & pattern)
{
  typedef PatternT pattern_t;

  dash::internal::print_pattern_mapping(
    "pattern.unit_at", pattern, 3,
    [](const pattern_t & _pattern, int _x, int _y) -> dart_unit_t {
      return _pattern.unit_at(std::array<index_t, 2> {{_x, _y}});
    });
  dash::internal::print_pattern_mapping(
    "pattern.global_at", pattern, 3,
    [](const pattern_t & _pattern, int _x, int _y) -> index_t {
      return _pattern.global_at(std::array<index_t, 2> {{_x, _y}});
    });
  dash::internal::print_pattern_mapping(
    "pattern.local", pattern, 10,
    [](const pattern_t & _pattern, int _x, int _y) -> std::string {
      auto lpos = _pattern.local(std::array<index_t, 2> {{_x, _y}});
        std::ostringstream ss;
        ss << lpos.unit << ":" << lpos.coords;
        return ss.str();
    });
  dash::internal::print_pattern_mapping(
    "pattern.at", pattern, 3,
    [](const pattern_t & _pattern, int _x, int _y) -> index_t {
      return _pattern.at(std::array<index_t, 2> {{_x, _y}});
    });
  dash::internal::print_pattern_mapping(
    "pattern.block_at", pattern, 3,
    [](const pattern_t & _pattern, int _x, int _y) -> index_t {
      return _pattern.block_at(std::array<index_t, 2> {{_x, _y}});
    });
  dash::internal::print_pattern_mapping(
    "pattern.block.offset", pattern, 5,
    [](const pattern_t & _pattern, int _x, int _y) -> std::string {
      auto block_idx = _pattern.block_at(std::array<index_t, 2> {{_x, _y}});
        auto block_vs  = _pattern.block(block_idx);
        std::ostringstream ss;
        ss << block_vs.offset(0) << "," << block_vs.offset(1);
        return ss.str();
    });
  dash::internal::print_pattern_mapping(
    "pattern.local_index", pattern, 3,
    [](const pattern_t & _pattern, int _x, int _y) -> index_t {
      return _pattern.local_index(std::array<index_t, 2> {{_x, _y}}).index;
    });
}

