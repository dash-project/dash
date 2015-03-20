/* 
 * Sequential GUPS benchmark for various containers
 *
 * author(s): Karl Fuerlinger, LMU Munich */
/* @DASH_HEADER@ */

#include <iostream>
#include <array>
#include <vector>
#include <libdash.h>
#include "../bench.h"
using namespace std;

#ifndef TYPE
#define TYPE           int
#endif 

#ifndef REPEAT
#define REPEAT         100
#endif 

#ifndef ELEM_PER_UNIT
#define ELEM_PER_UNIT  100000
#endif

void init_array(dash::Array<TYPE>& a);
void verify_array(dash::Array<TYPE>& a);

double test_dash_global_iter(dash::Array<TYPE>& a);
double test_dash_local_iter(dash::Array<TYPE>& a);
double test_dash_local_subscript(dash::Array<TYPE>& a);
double test_stl_array();
double test_stl_vector();
double test_stl_deque();

double gups(unsigned n, double dur) {
  double res = (double)n*ELEM_PER_UNIT*REPEAT;
  res/=dur;
  res*=1.0e-9; 
  return res;
}

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);

  auto size = dash::size();

  dash::Array<int> arr(ELEM_PER_UNIT*dash::size());
  
  double t1 = test_dash_global_iter(arr);
  double t2 = test_dash_local_iter(arr);
  double t3 = test_dash_local_subscript(arr);
  double t4 = test_stl_array();
  double t5 = test_stl_vector();
  double t6 = test_stl_deque();

  if(dash::myid()==0 ) {
    cout<<"Global iterator : "<<gups(size,t1)<<endl;
    cout<<"Local  iterator : "<<gups(size,t2)<<endl;
    cout<<"Local  subscript: "<<gups(size,t3)<<endl;
    cout<<"STL array       : "<<gups(1,t4)<<endl;
    cout<<"STL vector      : "<<gups(1,t5)<<endl;
    cout<<"STL deque       : "<<gups(1,t6)<<endl;
  }

  dash::finalize();
}


void init_array(dash::Array<TYPE>& arr)
{
  auto myid = dash::myid();
  auto size = dash::size();
  
  if(myid==0) {
    for(auto i=0; i<arr.size(); i++) {
      arr[i]=i;
    }
  }
  arr.barrier();
}

void verify_array(dash::Array<TYPE>& arr)
{
  auto myid = dash::myid();
  auto size = dash::size();

  arr.barrier();
  if(myid==0) {
    bool valid=true;
    for(auto i=0; i<arr.size(); i++) {
      if( arr[i]!=(i+REPEAT) ) {
	valid=false; 
	break;
      }
    }
    if(!valid) {
      cerr<<"Validation failed!"<<endl;
    }
  }
}

double test_dash_global_iter(dash::Array<TYPE>& a)
{
  init_array(a);

  double tstart, tend;
  TIMESTAMP(tstart);
  for(auto i=0; i<REPEAT; i++ ) {
    for(auto it=a.begin(); it!=a.end(); it++) {
      if( (*it).is_local() ) 
	(*it)=(*it)+1;
    }
  }
  TIMESTAMP(tend);
  
  verify_array(a);
  return tend-tstart;
}

double test_dash_local_iter(dash::Array<TYPE>& a)
{
  init_array(a);

  double tstart, tend;
  TIMESTAMP(tstart);
  for(auto i=0; i<REPEAT; i++ ) {
    for(auto it=a.lbegin(); it!=a.lend(); it++) {
      (*it)++;
    }
  }
  TIMESTAMP(tend);

  verify_array(a);
  return tend-tstart;
}

double test_dash_local_subscript(dash::Array<TYPE>& a)
{
  init_array(a);

  double tstart, tend;
  TIMESTAMP(tstart);
  auto loc = a.local;
  for(auto i=0; i<REPEAT; i++ ) {
    for(auto j=0; j<ELEM_PER_UNIT/*j<arr.local.size()*/; j++ ) {
      loc[j]++;
    }
  }  
  TIMESTAMP(tend);  

  verify_array(a);

  return tend-tstart;
}

double test_stl_array()
{
  double tstart, tend;
  
  std::array<TYPE, ELEM_PER_UNIT> arr;
  for(int i=0; i<ELEM_PER_UNIT; i++ ) {
    arr[i]=i;
  }
  
  TIMESTAMP(tstart);
  for(auto i=0; i<REPEAT; i++ ) {
    for(auto j=0; j<ELEM_PER_UNIT; j++ ) {
      arr[j]++;
    }
  }
  TIMESTAMP(tend);  

  cout<<arr[ELEM_PER_UNIT-1]<<endl;

  return tend-tstart;
}

double test_stl_vector()
{
  double tstart, tend;
  
  std::vector<TYPE> arr(ELEM_PER_UNIT);
  for(int i=0; i<ELEM_PER_UNIT; i++ ) {
    arr[i]=i;
  }
  
  TIMESTAMP(tstart);
  for(auto i=0; i<REPEAT; i++ ) {
    for(auto j=0; j<ELEM_PER_UNIT; j++ ) {
      arr[j]++;
    }
  }
  TIMESTAMP(tend);  

  cout<<arr[ELEM_PER_UNIT-1]<<endl;

  return tend-tstart;
}


double test_stl_deque()
{
  double tstart, tend;
  
  std::deque<TYPE> arr(ELEM_PER_UNIT);
  for(int i=0; i<ELEM_PER_UNIT; i++ ) {
    arr[i]=i;
  }
  
  TIMESTAMP(tstart);
  for(auto i=0; i<REPEAT; i++ ) {
    for(auto j=0; j<ELEM_PER_UNIT; j++ ) {
      arr[j]++;
    }
  }
  TIMESTAMP(tend);  

  cout<<arr[ELEM_PER_UNIT-1]<<endl;

  return tend-tstart;
}
