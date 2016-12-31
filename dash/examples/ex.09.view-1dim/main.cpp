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

  for (int l = 0; l < array.local.size(); l++) {
    array.local[l] = (myid + 1) * 1000 + l;
  }

  array.barrier();

  if (myid == 0) {
    auto sub_0 = dash::sub(block_size * (nunits-1),
                           block_size * (nunits-1) + block_size,
                           array);
    auto sub_1 = dash::sub(2,
                           block_size - 2,
                           sub_0);
    cout << "sub_0 = sub(<block range>, array): \n"
         << "  index(begin):   " << dash::index(dash::begin(sub_0)) << '\n'
         << "  index(end):     " << dash::index(dash::end(sub_0))   << '\n'
         << "  size:           " << sub_0.size()                    << '\n'
         << endl;

    cout << "sub_1 = sub(<2,-2>, sub_0): \n"
         << "  index(begin):   " << dash::index(dash::begin(sub_1)) << '\n'
         << "  index(end):     " << dash::index(dash::end(sub_1))   << '\n'
         << "  size:           " << sub_1.size()                    << '\n'
         << endl;

    cout << "sub_1 values:\n";
    for (auto i = sub_1.begin(); i != sub_1.end(); ++i) {
      cout << array[dash::index(i)] << " ";
    }
    cout << endl;
  }

  dash::finalize();

  return EXIT_SUCCESS;
}
