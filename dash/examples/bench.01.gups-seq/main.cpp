/* 
 * Sequential GUPS benchmark for various containers
 *
 * author(s): Karl Fuerlinger, LMU Munich */
/* @DASH_HEADER@ */

#include "../bench.h"
#include <libdash.h>

#include <array>
#include <vector>
#include <deque>
#include <iostream>

using namespace std;

#ifndef TYPE
#define TYPE           int
#endif 

#ifndef REPEAT
#define REPEAT         200
#endif 

#ifndef ELEM_PER_UNIT
#define ELEM_PER_UNIT  10000
#endif


void init_array(dash::Array<TYPE>& a);
void validate_array(dash::Array<TYPE>& a);

double test_dash_global_iter(dash::Array<TYPE>& a);
double test_dash_local_iter(dash::Array<TYPE>& a);
double test_dash_local_subscript(dash::Array<TYPE>& a);
double test_stl_array();
double test_stl_vector();
double test_stl_deque();
double test_raw_array();
template<typename Iter>
void validate(Iter begin, Iter end);

double gups(unsigned N, double dur) {
  double res = (double) N * ELEM_PER_UNIT * REPEAT;
  res *= 1.0e-9; 
  res /= dur;
  return res;
}

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);

  auto size = dash::size();
  
  dash::Array<int> arr(ELEM_PER_UNIT*size);
  
  double t1 = test_dash_global_iter(arr);
  double t2 = test_dash_local_iter(arr);
  double t3 = test_dash_local_subscript(arr);
  double t4 = test_stl_array();
  double t5 = test_stl_vector();
  double t6 = test_stl_deque();
  double t7 = test_raw_array();

  if(dash::myid()==0 ) {
    cout<<"Results (in sequential Giga Updates Per Second)"<<endl;
    cout<<"global_iterator : "<<gups(size, t1)<<endl;
    cout<<"local_iterator  : "<<gups(size, t2)<<endl;
    cout<<"local_subscript : "<<gups(size, t3)<<endl;
    cout<<"stl_array       : "<<gups(size, t4)<<endl;
    cout<<"stl_vector      : "<<gups(size, t5)<<endl;
    cout<<"stl_deque       : "<<gups(size, t6)<<endl;
    cout<<"raw_array       : "<<gups(size, t7)<<endl;
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

void validate_array(dash::Array<TYPE>& arr)
{
  arr.barrier();
  if(dash::myid()==0) {
    validate(arr.begin(), arr.end());
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
  
  validate_array(a);
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

  validate_array(a);
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

  validate_array(a);

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

  validate(arr.begin(), arr.end());

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

  validate(arr.begin(), arr.end());
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
  
  validate(arr.begin(), arr.end());

  return tend-tstart;
}

double test_raw_array()
{
  double tstart, tend;
  
  TYPE arr[ELEM_PER_UNIT];
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

  validate(arr, arr+ELEM_PER_UNIT);

  return tend-tstart;
}


template<typename Iter>
void validate(Iter begin, Iter end) 
{
  bool valid=true; auto i=0;
  for(auto it=begin; it!=end; it++, i++) {
    if( (*it)!=(i+REPEAT) ) {
      valid=false; 
      break;
    }
  }
  if(!valid) 
    cerr<<"Validation FAILED!"<<endl;
}
