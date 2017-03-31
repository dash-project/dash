/**
 * \example ex.03.globref/main.cpp
 * Example illustrating the use of DASH global 
 * references, i.e.,  \c dash::GlobRef
 */

#include <unistd.h>
#include <iostream>
#include <libdash.h>

#define SIZE 10

using namespace std;

int main(int argc, char * argv[])
{
  dash::init(&argc, &argv);

  auto myid = dash::myid();
  auto size = dash::size();

  dash::Array<int> arr(SIZE);

  if (myid == 0) {
    dash::GlobRef<int> r1 = arr[0];
    dash::GlobRef<int> r2 = arr[1];
    dash::GlobRef<int> r3 = arr[2];
    dash::GlobRef<int> r4 = arr[3];
    dash::GlobRef<int> r5 = arr[4];

    r1 = 33;
    r2 = -1;
    r3 = 42; // on lhs

    int a = 0;
    a  = r3; // on rhs
    DASH_ASSERT(a  == 42);

    r3 = r1; // lhs and rhs
    DASH_ASSERT(r3 == 33);

    r3 += 5; // r3 is 38

    r4 = r3; // r4 is 38
    r5 = r4 += r3; // r5 is 76; r4 too

    ++r5; // r5 is 77
  }

  arr.barrier();

  if (myid == size - 1) {
    DASH_ASSERT(arr[0] == 33);
    DASH_ASSERT(arr[1] == -1);
    DASH_ASSERT(arr[2] == 38);
    DASH_ASSERT(arr[3] == 76);
    DASH_ASSERT(arr[4] == 77);

    for ( auto i = 0; i < arr.size(); i++ ) {
      cout << (int) arr[i] << " ";
    }
    cout << endl;
  }

  dash::finalize();
}
