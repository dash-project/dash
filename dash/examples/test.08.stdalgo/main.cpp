/* 
 * stdalgo/main.cpp 
 *
 * author(s): Karl Fuerlinger, LMU Munich */
/* @DASH_HEADER@ */

#include <algorithm>
#include <iostream>
#include <libdash.h>

using namespace std;

bool test_for_each(size_t n);
bool test_count(size_t n);
void test_sequence_predicates();


int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);
  
  test_for_each(100);
  test_count(100);
  test_sequence_predicates();

  dash::finalize();
}

bool test_for_each(size_t n)
{
  int myid = dash::myid();
  int size = dash::size();
  
  dash::Array<int> arr(n);
  //std::vector<int> arr(10);

  if( myid==0 ) {
    for( int i=0; i<n; i++ ) { arr[i]=i; }
  }
  arr.barrier();

  // for_each with lambdas and references:
  //
  // this doesn't work with dash::array because we can't return an
  // integer reference; auto should work, and then we could implement
  // operators on GlobRef, but g++ 4.6.3 did not like it
  //
  // for_each(arr.begin(), arr.end(), [](int& x) {++x;});
  if( myid==1 ) {
    int sum=0;
    // for_each with global iterators
    for_each(arr.begin(), arr.end(), [&sum](int x) {sum+=x;});
    cout<<"Sum is: "<<sum<<endl;
  }

  // for_each_with local iterators
  int mysum=0;
  for_each(arr.lbegin(), arr.lend(), [&mysum](int x) {mysum+=x;});
  cout<<"["<<myid<<"] mysum is: "<<mysum<<endl;

#if 0
  if( myid==0 ) {
    for( auto v: arr) {
      cout<<v<<" ";
    }
    cout<<endl;
  }
#endif

  return true;
}

bool test_count(size_t n)
{
  int myid = dash::myid();
  int size = dash::size();
  
  dash::Array<int> arr(n);
  //std::vector<int> arr(10);

  if( myid==0 ) {
    for( int i=0; i<n; i++ ) { arr[i]=i; }
  }
  arr.barrier();

  int count = std::count(arr.lbegin(), arr.lend(), 5);
  cout<<"["<<myid<<"] Found the number: "<<count<<endl;

  return true;
}


void test_sequence_predicates()
{
  int myid = dash::myid();
  int size = dash::size();
  
  dash::Array<int> arr(10);
  //vector<int> arr(10);
  std::fill( arr.begin(), arr.end(), 1);

  //
  // testing std::all_of, std::none_of, and std::any_of
  // with global iterators
  //
  if( myid==1 ) {
    if( std::all_of(arr.begin(), arr.end(), [](int x){ return x>0;}) ) {
      cout<<"All are greater than 0"<<endl;
    } else {
      cout<<"Some are not greater than 0"<<endl;
    }
    if( std::any_of(arr.begin(), arr.end(), [](int x){ return x>0;}) ) {
      cout<<"Some are greater than 0"<<endl;
    } else {
      cout<<"None are greater than 0"<<endl;
    }
    if( std::none_of(arr.begin(), arr.end(), [](int x){ return x>0;}) ) {
      cout<<"None are greater than 0"<<endl;
    } else {
      cout<<"Some are greater than 0"<<endl;
    }
  }

  bool myresult;
  myresult = std::all_of(arr.lbegin(), arr.lend(), [](int x){ return x>0;});
  myresult = std::any_of(arr.lbegin(), arr.lend(), [](int x){ return x>0;});
  myresult = std::none_of(arr.lbegin(), arr.lend(), [](int x){ return x>0;});
  cout<<"["<<myid<<"] myresult is: "<<myresult<<endl;
  
}
