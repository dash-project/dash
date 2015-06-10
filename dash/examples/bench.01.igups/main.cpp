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

void init_array(dash::Array<TYPE>& a, unsigned, unsigned);
void validate_array(dash::Array<TYPE>& a, unsigned, unsigned);

double test_dash_global_iter(dash::Array<TYPE>& a, unsigned, unsigned);
double test_dash_local_iter(dash::Array<TYPE>& a, unsigned, unsigned);
double test_dash_local_subscript(dash::Array<TYPE>& a, unsigned, unsigned);
double test_dash_local_pointer(dash::Array<TYPE>& a, unsigned, unsigned);
//double test_stl_array(unsigned, unsigned);
double test_stl_vector(unsigned, unsigned);
double test_stl_deque(unsigned, unsigned);
double test_raw_array(unsigned, unsigned);

template<typename Iter>
void validate(Iter begin, Iter end, unsigned, unsigned);

void perform_test(unsigned ELEM_PER_UNIT, unsigned REPEAT);

double gups(unsigned N, double dur, 
	    unsigned ELEM_PER_UNIT, 
	    unsigned REPEAT) {
  double res = (double) N * ELEM_PER_UNIT * REPEAT;
  res *= 1.0e-9; 
  res /= dur;
  return res;
}

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);

  std::deque<std::pair<int, int>> tests;

  tests.push_back({0          , 0}); // this prints the header
  tests.push_back({4          , 100000});
  tests.push_back({16         , 10000});
  tests.push_back({64         , 10000});
  tests.push_back({256        , 10000});
  tests.push_back({1024       , 1000});
  tests.push_back({4096       , 1000});
  tests.push_back({4*4096     , 100});
  tests.push_back({16*4096    , 100});
  tests.push_back({64*4096    , 50});

  //tests.push_back({256*4096   , 20});
  //tests.push_back({512*4096   , 20});
  //tests.push_back({1024*4096  , 20});
  //tests.push_back({4096*4096  , 20});

  for( auto test: tests ) {
    perform_test(test.first, test.second);
  }

  dash::finalize();
}


void perform_test(unsigned ELEM_PER_UNIT,
		  unsigned REPEAT)
{
  auto size = dash::size();

  if( ELEM_PER_UNIT==0 ) {
    if(dash::myid()==0 ) {
      cout<<ELEM_PER_UNIT<<","<<REPEAT;
      cout<<","<<"dash_glob_iter";
      cout<<","<<"dash_local_iter";
      cout<<","<<"dash_local_subscript";
      cout<<","<<"dash_local_pointer";
      cout<<","<<"stl_vector";
      cout<<","<<"stl_deque";
      cout<<","<<"raw_array";
      cout<<endl;
    }
    return;
  }
  
  dash::Array<int> arr(ELEM_PER_UNIT*size);
  
  double t1 = test_dash_global_iter(arr, ELEM_PER_UNIT, REPEAT);
  double t2 = test_dash_local_iter(arr, ELEM_PER_UNIT, REPEAT);
  double t3 = test_dash_local_subscript(arr, ELEM_PER_UNIT, REPEAT);
  double t4 = test_dash_local_pointer(arr, ELEM_PER_UNIT, REPEAT);
  double t5 = test_stl_vector(ELEM_PER_UNIT, REPEAT);
  double t6 = test_stl_deque(ELEM_PER_UNIT, REPEAT);
  double t7 = test_raw_array(ELEM_PER_UNIT, REPEAT);
  
  if(dash::myid()==0 ) {

    double gups1 = gups(size, t1, ELEM_PER_UNIT, REPEAT);
    double gups2 = gups(size, t2, ELEM_PER_UNIT, REPEAT);
    double gups3 = gups(size, t3, ELEM_PER_UNIT, REPEAT);
    double gups4 = gups(size, t4, ELEM_PER_UNIT, REPEAT);
    double gups5 = gups(size, t5, ELEM_PER_UNIT, REPEAT);
    double gups6 = gups(size, t6, ELEM_PER_UNIT, REPEAT);
    double gups7 = gups(size, t7, ELEM_PER_UNIT, REPEAT);

    cout<<ELEM_PER_UNIT<<","<<REPEAT;
    cout<<","<<gups1;
    cout<<","<<gups2;
    cout<<","<<gups3;
    cout<<","<<gups4;
    cout<<","<<gups5;
    cout<<","<<gups6;
    cout<<","<<gups7;
    cout<<endl;

    /*
    cout<<"Results (in sequential Giga Updates Per Second)"<<endl;
    cout<<"global_iterator : "<<gups(size, t1, ELEM_PER_UNIT, REPEAT)<<endl;
    cout<<"local_iterator  : "<<gups(size, t2, ELEM_PER_UNIT, REPEAT)<<endl;
    cout<<"local_subscript : "<<gups(size, t3, ELEM_PER_UNIT, REPEAT)<<endl;
    //    cout<<"stl_array       : "<<gups(size, t4, ELEM_PER_UNIT, REPEAT)<<endl;
    cout<<"stl_vector      : "<<gups(size, t5, ELEM_PER_UNIT, REPEAT)<<endl;
    cout<<"stl_deque       : "<<gups(size, t6, ELEM_PER_UNIT, REPEAT)<<endl;
    cout<<"raw_array       : "<<gups(size, t7, ELEM_PER_UNIT, REPEAT)<<endl;
    */
  }
}


void init_array(dash::Array<TYPE>& arr,
		unsigned ELEM_PER_UNIT, 
		unsigned REPEAT)		    
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

void validate_array(dash::Array<TYPE>& arr,
		    unsigned ELEM_PER_UNIT, 
		    unsigned REPEAT)		    
{
  arr.barrier();
  if(dash::myid()==0) {
    validate(arr.begin(), arr.end(), 
	     ELEM_PER_UNIT, REPEAT);
  }
}

double test_dash_global_iter(dash::Array<TYPE>& a,
			    unsigned ELEM_PER_UNIT, 
			    unsigned REPEAT)
{
  init_array(a, ELEM_PER_UNIT, REPEAT);

  double tstart, tend;
  TIMESTAMP(tstart);
  for(auto i=0; i<REPEAT; i++ ) {
    for(auto it=a.begin(); it!=a.end(); it++) {
      if( (*it).is_local() ) 
	(*it)=(*it)+1;
    }
  }
  TIMESTAMP(tend);
  
  validate_array(a, ELEM_PER_UNIT, REPEAT);
  return tend-tstart;
}

double test_dash_local_iter(dash::Array<TYPE>& a,
			    unsigned ELEM_PER_UNIT, 
			    unsigned REPEAT)
{
  init_array(a, ELEM_PER_UNIT, REPEAT);

  double tstart, tend;
  TIMESTAMP(tstart);
  for(auto i=0; i<REPEAT; i++ ) {
    for(auto it=a.lbegin(); it!=a.lend(); it++) {
      (*it)++;
    }
  }
  TIMESTAMP(tend);

  validate_array(a, ELEM_PER_UNIT, REPEAT);
  return tend-tstart;
}

double test_dash_local_subscript(dash::Array<TYPE>& a,
				 unsigned ELEM_PER_UNIT, 
				 unsigned REPEAT)
{
  init_array(a, ELEM_PER_UNIT, REPEAT);

  double tstart, tend;
  TIMESTAMP(tstart);
  auto loc = a.local;
  for(auto i=0; i<REPEAT; i++ ) {
    for(auto j=0; j<ELEM_PER_UNIT/*j<arr.local.size()*/; j++ ) {
      loc[j]++;
    }
  }  
  TIMESTAMP(tend);  

  validate_array(a, ELEM_PER_UNIT, REPEAT);

  return tend-tstart;
}

double test_dash_local_pointer(dash::Array<TYPE>& a,
			       unsigned ELEM_PER_UNIT, 
			       unsigned REPEAT)
{
  init_array(a, ELEM_PER_UNIT, REPEAT);
  
  double tstart, tend;
  TIMESTAMP(tstart);
  auto lbegin = a.lbegin();
  auto lend   = a.lend();

  for(auto i=0; i<REPEAT; i++ ) {
    for(auto j=lbegin; j!=lend; j++ ) {
      (*j)++;
    }
  }  
  TIMESTAMP(tend);  

  validate_array(a, ELEM_PER_UNIT, REPEAT);

  return tend-tstart;
}

/*
double test_stl_array(unsigned ELEM_PER_UNIT, 
		      unsigned REPEAT)
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

  validate(arr.begin(), arr.end(), 
	   ELEM_PER_UNIT, REPEAT););

  return tend-tstart;
}
*/

double test_stl_vector(unsigned ELEM_PER_UNIT, 
		       unsigned REPEAT)
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

  validate(arr.begin(), arr.end(),
	   ELEM_PER_UNIT, REPEAT );
  return tend-tstart;
}


double test_stl_deque(unsigned ELEM_PER_UNIT, 
		      unsigned REPEAT)
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
  
  validate(arr.begin(), arr.end(), ELEM_PER_UNIT, REPEAT);

  return tend-tstart;
}

double test_raw_array(unsigned ELEM_PER_UNIT, 
		      unsigned REPEAT)
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

  validate(arr, arr+ELEM_PER_UNIT, 
	   ELEM_PER_UNIT, REPEAT);
  return tend-tstart;
}


template<typename Iter>
void validate(Iter begin, Iter end, 
	      unsigned ELEM_PER_UNIT, 
	      unsigned REPEAT)
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
