#include <unistd.h>
#include <iostream>
#include <libdash.h>

using namespace std;

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);
  
  dash::Array<int> arr(100);

  if(dash::myid()==0) {
    for(auto i=0; i<arr.size(); i++ ) {
      arr[i]=i;
    }
  }
  arr.barrier();


  // progressively restrict the global range from the front until
  // reaching arr.end(); call min_element() for each sub-range
  auto it=arr.begin();
  while(it!=arr.end() ) {
    auto min = dash::min_element(it, arr.end());
    if( dash::myid()==0 ) {
      cout<<"Min: "<<(int)(*min)<<flush<<endl;
    }
    it++;
  }
  
  struct test_t {
    int a;
    double b;
  };
  
  dash::Array<test_t> arr2(100);
  for( auto& el: arr2.local ) {
    el = {rand()%100, 23.3};
  }
  arr2.barrier();

  // the following shows the usage of min_element with a composite
  // datatype (test_t). We're passing a comparator function as a
  // lambda expression
  auto min =
    dash::min_element<test_t>(arr2.begin(), arr2.end(),
			      [](const test_t& t1, const test_t& t2) -> bool {
				return t1.a<t2.a;
			      });

  
  if( dash::myid()==0 ) {
    cout<<"Min. test_t: "<<((test_t)*min).a<<" "<<((test_t)*min).b<<endl;
  }
  
  dash::finalize();
}
