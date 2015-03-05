
#include <stdio.h>
#include <iostream>
#include <libdash.h>
#include <list>

#define Pattern Pattern1D
#define HUGE 33133

using namespace std;

void test_array(const dash::Pattern& pat,
		size_t writer=0, size_t reader=0);

int main(int argc, char* argv[])
{
  int i, j;

  dash::init(&argc, &argv);

  std::list<size_t> sizes;
  sizes.push_back(335);
  //  sizes.push_back(1);
  //sizes.push_back(2);
  sizes.push_back(dash::size());
  sizes.push_back(dash::size()-1);
  sizes.push_back(dash::size()+1);
  sizes.push_back(31133);
  sizes.push_back(HUGE);

  for( size_t size : sizes ) {
    dash::Pattern pat00(size, dash::CYCLIC);
    dash::Pattern pat01(size, dash::BLOCKCYCLIC(1));
    dash::Pattern pat02(size, dash::BLOCKCYCLIC(size));
    dash::Pattern pat03(size, dash::BLOCKCYCLIC(size-1));
    dash::Pattern pat04(size, dash::BLOCKED);
    
    for( i=0; i<dash::size(); i++ ) {
      for( j=0; j<dash::size(); j++ ) {
	dash::Team::All().barrier();
	if( dash::myid()==0 ) {
	  fprintf(stderr, "Testing size=%u reader=%d writer=%d\n", 
		  size, i, j);
	}
	test_array(pat00, i, j);
	test_array(pat01, i, j);
	test_array(pat02, i, j);
	test_array(pat03, i, j);
	test_array(pat04, i, j);
      }
    }
  }

  dash::finalize();
}



void test_array(const dash::Pattern& pat,
		size_t writer, size_t reader) 
{
  int myid = dash::myid();
  int size = dash::size();
  
  dash::Array<int> arr(pat);
  
  if( myid==(writer%size) ) {
    for( int i=0; i<arr.size(); i++ ) {
      arr[i]=i;
    }
  }
  
  arr.barrier();
  
  if( myid==(reader%size) ) {
    for( int i=0; i<arr.size(); i++ ) {
      if( arr[i]!=i ) {
	fprintf(stderr, "Mismatch at position: %d %d \n", 
		i, (int)arr[i]);
      }
    }
  }  
}

