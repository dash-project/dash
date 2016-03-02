#include <unistd.h>
#include <iostream>
#include <cstddef>

#include <libdash.h>

using namespace std;

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);

  auto   myid = dash::myid();
  size_t size = dash::size();

  dash::Array<int> arr(100, dash::CYCLIC);

  for (size_t i = 0; i < arr.local.size(); i++) {
    arr.local[i]=myid;
  }
  arr.barrier();
  if (static_cast<size_t>(myid) == size-1) {
    for (auto el: arr) {
      cout << el << " ";
    }
    cout << endl;
  }

  dash::finalize();
}
