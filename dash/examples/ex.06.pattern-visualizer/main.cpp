#include <unistd.h>
#include <iostream>
#include <fstream>
#include <cstddef>

#include <libdash.h>

using std::cout;
using std::cerr;
using std::endl;

using namespace dash;

template<typename PatternT>
void print_example(
  PatternT    pat,
  std::string fname,
  std::string title);

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);

  if (dash::size() != 4) {
    dash::finalize();
    cerr << "Pattern visualizer must be called with 4 units." << endl;
    return EXIT_FAILURE;
  }

  if (dash::myid() == 0)
  {
    TeamSpec<2> ts(2,2);

    print_example(TilePattern<2>(20,15, TILE(1),TILE(5), ts),
      "TilePattern_tile-1x5_rowmaj.svg",
      "TilePattern<2>(20,15, TILE(1), TILE(5), TeamSpec<2>(2,2))");

    print_example(TilePattern<2>(20,15, TILE(2),TILE(5), ts),
      "TilePattern_tile-2x5_rowmaj.svg",
      "TilePattern<2>(20,15, TILE(2), TILE(5), TeamSpec<2>(2,2))");

    print_example(TilePattern<2, COL_MAJOR>(20,15, TILE(5),TILE(5), ts),
      "TilePattern_tile-5x5_colmaj.svg",
      "TilePattern<2, COL_MAJOR>(20,15, TILE(5),TILE(5), TeamSpec<2>(2,2))");

    print_example(TilePattern<2>(20,15, TILE(5),TILE(1), ts),
      "TilePattern_tile-5x1_team-2x2.svg",
      "TilePattern<2>(20,15, TILE(5), TILE(1), TeamSpec<2>(2,2))");

    print_example(Pattern<2>(20,15, BLOCKED,CYCLIC, ts),
      "BlockPattern_blocked-cyclic_rowmaj.svg",
      "Pattern<2>(20,15, BLOCKED,CYCLIC, TeamSpec<2>(2,2))");

    print_example(ShiftTilePattern<2>(32,24, TILE(4), TILE(3)),
      "ShiftTilePattern_4x5_rowmaj.svg",
      "ShiftTilePattern<2>(32,24, TILE(4),TILE(5))");
  }

  dash::finalize();

  return EXIT_SUCCESS;
}

template<typename PatternT>
void print_example(
  PatternT    pat,
  std::string fname,
  std::string title)
{
  typedef typename PatternT::index_type index_t;

  dash::tools::PatternVisualizer<decltype(pat)> pv(pat);
  pv.set_title(title);

  std::ofstream out(fname);

  std::array<index_t, pat.ndim()> coords = {};
  pv.draw_pattern(out, coords, 1, 0);
  out.close();
}

