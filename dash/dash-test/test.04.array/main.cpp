/* 
 * array/main.cpp 
 *
 * author(s): Karl Fuerlinger, LMU Munich */
/* @DASH_HEADER@ */

#include <unistd.h>
#include <iostream>
#include <libdash.h>

#define SIZE 10

using namespace std;

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);
  
  auto myid = dash::myid();
  auto size = dash::size();
  int nelem = 11;
  dash::Pattern<1> pat(nelem);

  dash::Array<int> arr(SIZE);
  for( auto it = arr.lbegin(); it!=arr.lend(); it++ ) {
    (*it)=myid;
  }
  arr.barrier();
  
  if(myid==0 ) {  
    for( auto it = arr.begin(); it!=arr.end(); it++ ) {
      cout<<(*it)<<" ";
      fprintf(stdout, "Owner of %d: %d at %d atunit %d max %d nelem %d  \n", i, res, pat.index_to_elem(i), arr1.pattern().index_to_unit(i), arr1.pattern().max_elem_per_unit(), arr1.pattern().nelem());
    }
    cout<<endl;
  }

  
  dash::finalize();
}
