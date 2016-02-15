#include <unistd.h>
#include <iostream>
#include <fstream>
#include <cstddef>

#include <libdash.h>

using std::cout;
using std::endl;

using namespace dash;

template<typename PatternT>
void print_example(PatternT pat, std::string fname, std::string title);

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);

  if( dash::myid()==0 ) {

    auto ts = TeamSpec<2>(2,2);
    auto pat = TilePattern<2>(20,15, TILE(5), TILE(1), ts);

    print_example(pat,
		  "Test1.svg",
		  "Test");
    
    print_example(TilePattern<2>(20,15, TILE(1), TILE(5)),
		  "TilePattern1.svg",
		  "TilePattern<2>(20,15, TILE(1), TILE(5))");


    print_example(TilePattern<2>(20,15, TILE(2), TILE(5)),
		  "TilePattern2.svg",
		  "TilePattern<2>(20,15, TILE(2), TILE(5))");

    print_example(TilePattern<2, COL_MAJOR>(20,15, TILE(5), TILE(5)),
		  "TilePattern3.svg",
		  "TilePattern<2, COL_MAJOR>(20,15, TILE(5), TILE(5))");

    print_example(Pattern<2>(20,15, BLOCKED, CYCLIC),
		  "TilePattern4.svg",
		  "Pattern<2>(20,15, BLOCKED, CYCLIC),");


  }

  dash::finalize();
}

template<typename PatternT>
void print_example(PatternT pat, std::string fname, std::string title)
{
  dash::PatternVisualizer<decltype(pat)> pv(pat);
  pv.set_title(title);

  std::ofstream out(fname);

  std::array<int, pat.ndim()> coords = {};
  pv.draw_pattern(out, coords, 1, 0);
  out.close();
}

