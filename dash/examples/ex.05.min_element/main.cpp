/**
 * \example ex.05.min_element/main.cpp
 * Example demonstrating different use-cases
 * of \c dash::min_element.
 */

#include <unistd.h>
#include <iostream>
#include <sstream>
#include <libdash.h>

using namespace std;

typedef struct test_t_s {
  int    a;
  double b;
} test_t;

std::ostream & operator<<(std::ostream & os, const test_t & test)
{
  std::ostringstream ss;
  ss << "test_t(a:" << test.a << " b:" << test.b << ")";
  return operator<<(os, ss.str());
}

int main(int argc, char * argv[])
{
  dash::init(&argc, &argv);

  dash::Array<int> arr(100);

  cout << "Unit " << dash::myid() << " PID: " << getpid()
       << endl;
  arr.barrier();

  if (dash::myid() == 0) {
    for (auto i = 0; i < arr.size(); i++ ) {
      arr[i] = i;
    }
  }
  arr.barrier();

  if (dash::myid() == 0) {
    cout << "dash::min_element on dash::Array<int>" << endl;
  }
  // progressively restrict the global range from the front until
  // reaching arr.end(); call min_element() for each sub-range
  auto it = arr.begin();
  while (it != arr.end()) {
    auto min = dash::min_element(it, arr.end());
    if (dash::myid() == 0) {
      std::stringstream ss;
      ss   << "Min: " << (int)(*min) << endl;
      cout << ss.str();
    }
    it++;
  }

  dash::Array<test_t> arr2(100);
  for (auto & el : arr2.local) {
    el = {rand() % 100, 23.3};
  }
  arr2.barrier();

  if (dash::myid() == 0) {
    cout << "dash::min_element on dash::Array<test_t>" << endl;
  }
  arr2.barrier();

  // the following shows the usage of min_element with a composite
  // datatype (test_t). We're passing a comparator function as a
  // lambda expression
  auto min = dash::min_element(
               arr2.begin(),
               arr2.end(),
               [](const test_t & t1, const test_t & t2) -> bool {
                 return t1.a < t2.a;
               }
             );

  if (dash::myid() == 0) {
    cout << "Min. test_t: " << ((test_t)*min).a
         << " "             << ((test_t)*min).b
         << endl;
  }

  dash::finalize();
}
