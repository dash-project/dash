/**
 * \example ex.06.pattern-block-visualizer/main.cpp
 * Example demonstrating the instantiation of 
 * different patterns and their visualization
 * using \c dash::tools::PatternVisualizer.
 */

#include <unistd.h>
#include <iostream>
#include <fstream>
#include <cstddef>

#include <libdash.h>

#include <dash/tools/PatternVisualizer.h>


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
    TeamSpec<2> ts2d(2,2);
    TeamSpec<2> ts1d(2,1);

    print_example(TilePattern<2>(20,15, TILE(1),TILE(5), ts2d),
      "TilePattern_tile-1x5_team-2x2_rowmaj.svg",
      "TilePattern<2>(20,15, TILE(1), TILE(5), TeamSpec<2>(2,2))");

    print_example(TilePattern<2>(20,15, TILE(2),TILE(5), ts2d),
      "TilePattern_tile-2x5_team-2x2_rowmaj.svg",
      "TilePattern<2>(20,15, TILE(2), TILE(5), TeamSpec<2>(2,2))");

    print_example(TilePattern<2, COL_MAJOR>(20,15, TILE(5),TILE(5), ts2d),
      "TilePattern_tile-5x5_team-2x2_colmaj.svg",
      "TilePattern<2, COL_MAJOR>(20,15, TILE(5),TILE(5), TeamSpec<2>(2,2))");

    print_example(TilePattern<2>(20,15, TILE(5),TILE(1), ts2d),
      "TilePattern_tile-5x1_team-2x2_rowmajor.svg",
      "TilePattern<2>(20,15, TILE(5), TILE(1), TeamSpec<2>(2,2))");

    print_example(Pattern<2>(20,15, BLOCKED,CYCLIC, ts2d),
      "BlockPattern_blocked-cyclic_team-2x2_rowmaj.svg",
      "Pattern<2>(20,15, BLOCKED,CYCLIC, TeamSpec<2>(2,2))");

    print_example(ShiftTilePattern<2>(32,24, TILE(4), TILE(3), ts2d),
      "ShiftTilePattern_4x5_team-2x2_rowmaj.svg",
      "ShiftTilePattern<2>(32,24, TILE(4),TILE(5))");

    print_example(ShiftTilePattern<2>(20,20, TILE(1), TILE(5), ts1d),
      "ShiftTilePattern_4x5_team-2x1_rowmaj.svg",
      "ShiftTilePattern<2>(20,20, TILE(1),TILE(5), TeamSpec<2>(2,1))");
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
  std::string fname_b = fname;
  fname_b.insert(std::find(fname_b.rbegin(),fname_b.rend(),'.').base()-1, {'_','b','l','o','c','k','e','d'});
  std::ofstream out_b(fname_b);

  pv.draw_pattern(out,  false);
  pv.draw_pattern(out_b, true);
  out.close();
  out_b.close();
}

