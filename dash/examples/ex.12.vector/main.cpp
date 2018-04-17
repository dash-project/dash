/**
 * \example ex.02.array/main.cpp
 * Example illustrating access to elements in a \c dash::Array by
 * global index.
 */

#include <unistd.h>
#include <iostream>
#include <cstddef>

#include <libdash.h>

using std::cout;
using std::endl;

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);

  auto myid = dash::myid();
  auto size = dash::size();

  dash::Vector<int> vec(5);

  if (myid == 0) {
    for (size_t i = 0; i < vec.size(); ++i) {
      vec[i] = i;
    }
  }
  vec.barrier();
  if (myid == (size-1)) {
    for (auto el: vec) {
      cout << static_cast<int>(el) << " ";
    }
    cout << endl;
  }

  dash::finalize();
}
