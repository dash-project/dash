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

  dash::Array<int> arr(dash::size() * 5);

  if (myid == 0) {
    for (size_t i = 0; i < arr.size(); ++i) {
      arr[i] = i;
    }
  }
  arr.barrier();
  if (myid == static_cast<dart_unit_t>(size-1)) {
    for (auto el: arr) {
      cout << static_cast<int>(el) << " ";
    }
    cout << endl;
  }

  dash::finalize();
}
