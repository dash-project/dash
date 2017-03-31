/**
 * \example ex.06.std-algo/main.cpp
 * Example demonstrating the use of STL
 * algorithms on DASH global data structures.
 */

#include <iostream>
#include <cstddef>
#include <algorithm>

#include <libdash.h>

using namespace std;

template<typename T> void print_array(
  const std::string    & s,
  const dash::Array<T> & arr);
template<typename T> void test_copy(const dash::Array<T> & arr);
template<typename T> void test_copy_if(const dash::Array<T> & arr);
template<typename T> void test_all_of(const dash::Array<T> & arr);


int main(int argc, char * argv[])
{
  dash::init(&argc, &argv);

  const size_t nelem = 30;
  dash::Array<int> arr(nelem);

  if (dash::myid() == 0) {
    for (size_t i = 0; i < arr.size(); ++i) {
      arr[i] = nelem - i;
    }
  }
  dash::barrier();
  if (dash::myid() == static_cast<dart_unit_t>(dash::size()-1)) {
    print_array("init", arr);
  }

  test_copy(arr);
  test_copy_if(arr);
  test_all_of(arr);

  dash::finalize();
}

template<typename T>
void print_array(const std::string    & s,
                 const dash::Array<T> & arr)
{
  cout << s << " " << std::flush;
  for (auto el : arr) {
    T val = el;
    cout << val << " ";
  }
  cout << endl;
}

template<typename T>
void test_copy(const dash::Array<T> & arr)
{
  dash::Array<T> arr2(arr.size());
  if (dash::myid() == 0) {
    DASH_LOG_DEBUG("ex.06.std-algo",
                   "Start std::copy (global to global)");
    std::copy(arr.begin(), arr.end(), arr2.begin());
  }
  dash::barrier();

  if (dash::myid() == static_cast<dart_unit_t>(dash::size()-1)) {
    print_array("std::copy", arr2);
  }
}

template<typename T>
void test_copy_if(const dash::Array<T> & arr)
{
  dash::Array<T> arr2(arr.size());
  if (dash::myid() == 0) {
    std::copy_if(arr.begin(), arr.end(), arr2.begin(),
                 [](dash::GlobRef<const T> r) {
                   return r % 2 == 0;
                 });
  }
  dash::barrier();

  if (dash::myid() == static_cast<dart_unit_t>(dash::size()-1)) {
    print_array("std::copy_if", arr2);
  }
}

template<typename T>
void test_all_of(const dash::Array<T> & arr)
{
  if (dash::myid() == 0) {
    auto all_gt_0 = std::all_of(arr.begin(), arr.end(),
                      [](dash::GlobRef<const T> r) {
                        return r > 0;
                      });
    auto all_gt_5 = std::all_of(arr.begin(), arr.end(),
                      [](dash::GlobRef<const T> r) {
                        return r > 5;
                      });

    cout << "std::all_of > 0: " << all_gt_0 << endl;
    cout << "std::all_of > 5: " << all_gt_5 << endl;
  }
}
