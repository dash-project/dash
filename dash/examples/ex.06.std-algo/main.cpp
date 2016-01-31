#include <iostream>
#include <cstddef>
#include <algorithm>

#include <libdash.h>

using namespace std;

template<typename T> void print_array(const dash::Array<T>& arr);
template<typename T> void test_copy(const dash::Array<T>& arr);
template<typename T> void test_copy_if(const dash::Array<T>& arr);
template<typename T> void test_all_of(const dash::Array<T>& arr);


int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);

  const size_t nelem=30;

  auto myid = dash::myid();
  auto size = dash::size();

  dash::Array<int> arr1(nelem);
  if(myid==0) {
    for( auto i=0; i<arr1.size(); i++ )
      arr1[i]=nelem-i;
  }
  dash::barrier();

  test_copy(arr1);
  test_copy_if(arr1);
  test_all_of(arr1);

  dash::finalize();
}

template<typename T>
void print_array(const std::string s,
		 const dash::Array<T>& arr)
{
  cout<<s<<" ";
  for( auto el: arr ) {
    cout<<(T)el<<" ";
  }
  cout<<endl;
}


template<typename T>
void test_copy(const dash::Array<T>& arr)
{
  dash::Array<T> arr2(arr.size());
  if(dash::myid()==0) {
    std::copy(arr.begin(), arr.end(), arr2.begin());
  }
  dash::barrier();

  if(dash::myid()==dash::size()-1) {
    print_array("std::copy", arr2);
  }
}

template<typename T>
void test_copy_if(const dash::Array<T>& arr)
{
  dash::Array<T> arr2(arr.size());
  if(dash::myid()==0) {
    std::copy_if(arr.begin(), arr.end(), arr2.begin(),
		 [](dash::GlobRef<T> r) { return r%2==0; });
  }
  dash::barrier();

  if(dash::myid()==dash::size()-1) {
    print_array("std::copy_if", arr2);
  }
}

template<typename T>
void test_all_of(const dash::Array<T>& arr)
{
  if(dash::myid()==0) {
    auto res =
      std::all_of(arr.begin(), arr.end(),
		  [](dash::GlobRef<T> r){return r>0;});

    cout<<"std::all_of: "<<res<<endl;
  }
}
