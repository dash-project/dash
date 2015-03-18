/* 
 * hello/main.cpp 
 *
 * author(s): Karl Fuerlinger, LMU Munich */
/* @DASH_HEADER@ */

#include <unistd.h>
#include <iostream>
#include <libdash.h>

using namespace std;

int sum(dash::Array<int>& arr, dash::Team& t);

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);
  
  auto myid = dash::myid();
  auto size = dash::size();

  dash::Team::All().split(2).split(2).split(2);

  dash::Array<int> arr(size);
  for( auto i=0; i<arr.size(); i++ ) arr[i]=i;
  
  arr.barrier();

  sum(arr, dash::Team::All());
  
  dash::finalize();
}


int sum(dash::Array<int>& arr, dash::Team& t)
{
  auto res=0;
  
  if( !t.isLeaf() ) {
    dash::Team& tsub = t.sub(1);
    
    dash::Array<int> sumarr(2, t);
    res = sum(arr, tsub);
    if( tsub.myid()==0 ) {
      sumarr[tsub.position()] = res;
    }
    t.barrier();

    if( t.myid()==0 ) {
      res = sumarr[0]+sumarr[1];
      cout<<"Internal sum: "<<res<<endl;
    }
    
    return res;
  }
 
  if( t.myid()==0 ) {
    for( auto i=0; i<t.size(); i++ ) {
      res+=arr[t.global_id(i)];
    }
    cout<<"Leaf sum: "<<res<<endl;
    return res;
  }
}
