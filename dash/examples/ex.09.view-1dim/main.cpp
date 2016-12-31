/**
 * \example ex.09.view-1dim/main.cpp
 * Illustrating view modifiers on a 1-dimensional array.
 */

#include <unistd.h>
#include <iostream>
#include <cstddef>
#include <sstream>

#include <libdash.h>

using namespace std;

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);

  auto myid   = dash::myid();
  auto nunits = dash::size();

  int block_size = 10;

  dash::Array<int> array(nunits * block_size);

  if (myid == 0) {
    auto sub_0 = dash::sub(block_size * (nunits-1),
                           block_size * (nunits-1) + block_size,
                           array);
    auto sub_1 = dash::sub(2,
                           block_size - 2,
                           sub_0);
    cout << "sub_0 = sub(<block range>, array): " << endl
         << "  index(begin):   " << dash::index(dash::begin(sub_0)) << endl
         << "  index(end):     " << dash::index(dash::end(sub_0))   << endl
         << "  size:           " << sub_0.size()                    << endl
         << endl;

#if 0
    cout << "sub_1 = sub(<2,-2>, sub_0): " << endl
         << "  index(begin):   " << dash::index(dash::begin(sub_1)) << endl
         << "  index(end):     " << dash::index(dash::end(sub_1))   << endl
         << "  index(g(begin)):"
               << dash::index(dash::global(dash::begin(sub_1))) << endl
         << "  index(g(end)):  "
               << dash::index(dash::global(dash::end(sub_1)))   << endl
         << "  size:           " << sub_1.size()                    << endl
         << endl;
#endif
  }

  dash::finalize();

  return EXIT_SUCCESS;
}
