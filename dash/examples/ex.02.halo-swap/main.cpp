#include <unistd.h>
#include <iostream>
#include <cstddef>
#include <sstream>

#include <libdash.h>

using namespace std;

template<typename MatrixT>
void print2d(MatrixT & m)
{
  using ElemT = typename MatrixT::value_type;

  for ( auto i = 0; i < m.extent(0); ++i ) {
    for ( auto j = 0; j < m.extent(1); ++j ) {
      if ( std::is_same<ElemT, double>::value) {
        cout << std::fixed << std::right << std::setprecision(2);
      }
      cout << std::setw(6);
      cout << (ElemT) m(i, j) << " ";
    }
    cout << endl;
  }
}


template<typename MatrixT>
void halo_swap2d(MatrixT & mat)
{
  using ElemT  = typename MatrixT::value_type;
  using IndexT = typename MatrixT::index_type;

  auto ts = mat.pattern().teamspec();
  auto size = ts.extent(0) * ts.extent(1);
  auto myid = dash::myid();

  auto ext0 = mat.local.extent(0);
  auto ext1 = mat.local.extent(1);

  dash::Array<ElemT> corners( 4 * size );   // 4 corners
  dash::Array<ElemT> edge0( 2 * ext0 * size ); // 2 edges dim0
  dash::Array<ElemT> edge1( 2 * ext1 * size ); // 2 edges dim1

  auto tl = mat.lbegin();
  auto tr = tl + ext1 - 1;
  auto bl = tl + ext1 * (ext0 - 1);
  auto br = bl + ext1 - 1;

  auto my_x = ts.x(myid);
  auto my_y = ts.y(myid);

  dash::team_unit_t bot(  (my_x + 1) < ts.extent(0) ? ts.at(my_x + 1, my_y  ) : -1);
  dash::team_unit_t top(  (my_x > 0)                ? ts.at(my_x - 1, my_y  ) : -1);
  dash::team_unit_t left( (my_y > 0)                ? ts.at(my_x  , my_y - 1) : -1);
  dash::team_unit_t right((my_y + 1) < ts.extent(1) ? ts.at(my_x  , my_y + 1) : -1);

  /*
  cout << "I'm " << myid << "(" << my_x << " " << my_y << ") "
       << "r=" << right << " "
       << "l=" << left << " "
       << "t=" << top << " "
       << "b=" << bot << endl;
  */

  if (top >= 0) {
    auto gidx = edge1.pattern().global(top, std::array<IndexT, 1> {{ 0 }});
    dash::copy( tl, tl + ext1, edge1.begin() + gidx[0] );
  }
  if (bot >= 0) {
    auto gidx = edge1.pattern().global(bot, std::array<IndexT, 1> {{ 0 }});
    dash::copy( tl, tl + ext1, edge1.begin() + gidx[0] + ext1 );
  }
  dash::barrier();

  if ( top >= 0 ) {
    std::copy(edge1.lbegin() + ext1 + 1, edge1.lbegin() + 2 * ext1 - 1, tl + 1);
  }

  if ( bot >= 0 ) {
    std::copy(edge1.lbegin() + 1     , edge1.lbegin() + ext1 - 1  , bl + 1);
  }
}


int main(int argc, char * argv[])
{
  dash::init(&argc, &argv);

  auto myid = dash::myid();
  auto size = dash::size();

  size_t tilex = 6;
  size_t tiley = 4;

  dash::TeamSpec<2> ts(3, 3);
  dash::TilePattern<2> pat(tilex * ts.extent(0),
                           tiley * ts.extent(1),
                           dash::TILE(tilex),
                           dash::TILE(tiley), ts);

  dash::Matrix < double, 2,
       decltype(pat)::index_type,
       decltype(pat) > mat(pat);

  for (size_t i = 0; i < mat.local.size(); i++) {
    mat.lbegin()[i] = (double)dash::myid() + (double)(i) / 100.0;
  }

  dash::barrier();

  halo_swap2d(mat);

  if ( myid == 1) {
    print2d(mat);
  }

  dash::finalize();
}
