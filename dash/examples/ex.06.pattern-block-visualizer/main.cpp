#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <cstddef>
#include <cmath>

#include <libdash.h>

#include <dash/tools/PatternBlockVisualizer.h>

using std::cout;
using std::cerr;
using std::endl;

using namespace dash;

typedef dash::default_index_t  index_t;
typedef dash::default_extent_t extent_t;

typedef struct cli_params_t {
  extent_t    size_x;
  extent_t    size_y;
  extent_t    units_x;
  extent_t    units_y;
  int         tile_x;
  int         tile_y;
  bool        cout;
} cli_params;

template<typename PatternType>
std::string pattern_to_string(
  const PatternType & pattern);
template<typename PatternType>
std::string pattern_to_filename(
  const PatternType & pattern);
template <typename T, std::size_t N>
std::ostream & operator<<(
  std::ostream           & o,
  const std::array<T, N> & arr);

cli_params parse_args(int argc, char * argv[]);
void print_params(const cli_params & params);

template<typename PatternT>
void print_example(
  PatternT           pat,
  std::string        fname,
  std::string        title,
  const cli_params & params);

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);

  auto params = parse_args(argc, argv);
  print_params(params);

  if (dash::myid() == 0)
  {
    try {
      dash::SizeSpec<2, extent_t> sizespec(params.size_y, params.size_x);
      dash::TeamSpec<2, index_t>  teamspec(params.units_y, params.units_x);

      auto pattern = dash::make_pattern<
                       dash::summa_pattern_partitioning_constraints,
                       dash::summa_pattern_mapping_constraints,
                       dash::summa_pattern_layout_constraints >(
                         sizespec,
                         teamspec);

      if (params.tile_x >= 0 && params.tile_y >= 0) {
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

      int n_blocks_j   = pattern.blockspec().extent(0);
      int imb_blocks_i = pattern.blockspec().extent(0) % teamspec.extent(0);
      int imb_blocks_j = pattern.blockspec().extent(1) % teamspec.extent(1);
      int bal_blocks_i = pattern.blockspec().extent(0) - imb_blocks_i;
      int bal_blocks_j = pattern.blockspec().extent(1) - imb_blocks_j;
      int imb_blocks   = imb_blocks_i * n_blocks_j +
                         bal_blocks_i * imb_blocks_j;
      int bal_blocks   = bal_blocks_i * bal_blocks_j;
      float imb_factor = static_cast<float>(imb_blocks) /
                         static_cast<float>(bal_blocks);

      cerr << "mapping imbalance: "
           << endl
           << "    balanced blocks:   ("
           << std::fixed << std::setw(3) << bal_blocks_i << ","
           << std::fixed << std::setw(3) << bal_blocks_j << " )"
           << " = " << bal_blocks
           << endl
           << "    imbalanced blocks: ("
           << std::fixed << std::setw(3) << imb_blocks_i << ","
           << std::fixed << std::setw(3) << imb_blocks_j << " )"
           << " = " << imb_blocks
           << endl
           << "    imbalance factor:  "  << std::setprecision(4) << imb_factor
           << endl
           << endl;

      auto pattern_desc = pattern_to_string(pattern);
      auto pattern_file = pattern_to_filename(pattern);

      print_example(pattern, pattern_file, pattern_desc, params);
    } catch (std::exception & excep) {
      cerr << excep.what() << endl;
    }
  }

  dash::finalize();

  return EXIT_SUCCESS;
}

template<typename PatternT>
void print_example(
  PatternT           pat,
  std::string        fname,
  std::string        title,
  const cli_params & params)
{
  typedef typename PatternT::index_type index_t;

  dash::tools::PatternBlockVisualizer<decltype(pat)> pv(pat);
  pv.set_title(title);

  std::array<index_t, pat.ndim()> coords = {0, 0};

  cerr << "Generating visualization of "
       << endl
       << "    " << title
       << endl;

  if (params.cout) {
    pv.draw_pattern(std::cout, coords, 1, 0);
  } else {
    cerr << "image file:"
         << endl
         << "    " << fname
         << endl;
    std::ofstream out(fname);
    pv.draw_pattern(out, coords, 1, 0);
    out.close();
  }
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
  params.cout    = false;

  for (auto i = 1; i < argc; i += 3) {
    std::string flag = argv[i];
    if (flag == "-n") {
      params.size_x  = static_cast<extent_t>(atoi(argv[i+1]));
      params.size_y  = static_cast<extent_t>(atoi(argv[i+2]));
    } else if (flag == "-u") {
      params.units_x = static_cast<extent_t>(atoi(argv[i+1]));
      params.units_y = static_cast<extent_t>(atoi(argv[i+2]));
    } else if (flag == "-t") {
      params.tile_x  = static_cast<extent_t>(atoi(argv[i+1]));
      params.tile_y  = static_cast<extent_t>(atoi(argv[i+2]));
    } else if (flag == "-p") {
      params.cout    = true;
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
       << "    size (-n x y): ( "
       << std::fixed << std::setw(w) << params.size_x << ", "
       << std::fixed << std::setw(w) << params.size_y << " )"
       << endl
       << "    team (-u x y): ( "
       << std::fixed << std::setw(w) << params.units_x << ", "
       << std::fixed << std::setw(w) << params.units_y << " )"
       << endl
       << "    tile (-t x y): ( "
       << std::fixed << std::setw(w) << params.tile_x << ", "
       << std::fixed << std::setw(w) << params.tile_y << " )"
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
     << pattern.PatternName
     << "<"
     << ndim << ","
     << storage_order << ","
     << typeid(index_t).name()
     << ">"
     << "("
     << "SizeSpec:"  << pattern.sizespec().extents()  << ", "
     << "TeamSpec:"  << pattern.teamspec().extents()  << ", "
     << "BlockSpec:" << pattern.blockspec().extents() << ", "
     << "BlockSize:" << blocksize
     << ")";

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
  ss << pattern.PatternName
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

template <typename T, std::size_t N>
std::ostream & operator<<(
  std::ostream & o,
  const std::array<T, N> & arr)
{
  std::ostringstream ss;
  auto nelem = arr.size();
  ss << "{ ";
  int i = 1;
  for (auto e : arr) {
    ss << e
       << (i++ < nelem ? "," : "");
  }
  ss << " }";
  operator<<(o, ss.str());
  return o;
}

