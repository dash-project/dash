
#include "StlAlgoTest.h"

#include <dash/Array.h>
#include <algorithm>

TEST_F(StlAlgoTest, CompilerADLTest){
  //printf(">>>>>>> STLAlgoTest mit CompilerADLTest in rank %d <<<<< \n", dash::myid());
  dash::Array<int> arr(dash::size());
  arr.local[0] = dash::myid();
  arr.barrier();
  //printf(">>>>>>> after 1st barrier and init of id_array in rank %d <<<<< \n", dash::myid());
  if(dash::myid() == 0){
    auto refbeg = *(arr.begin());
    auto refend = *(arr.end()-1);
    //printf(">>>>>>> after init of refbeg und refend in rank %d <<<<< \n", dash::myid());
    {
    using std::swap;
    using std::iter_swap;
    refbeg.swap(refend);
    //printf(">>>>>>> after refbeg.swap(refend) in rank %d <<<<< \n", dash::myid());
    swap(refbeg, refend);
    //printf(">>>>>>> after swap(refbeg, refend) in rank %d <<<<< \n", dash::myid());
    swap(*(arr.begin()), *(arr.end()-1));
    //printf(">>>>>>> after swap(*(arr.begin()), *(arr.end()-1)) in rank %d <<<<< \n", dash::myid());
    iter_swap(arr.begin(), arr.end()-1);
    //printf(">>>>>>> after iter_swap(arr.begin(), arr.end()-1) in rank %d <<<<< \n", dash::myid());
    }
    // without ADL
    {
    // (1) uses move semantics, hence references are swapped
    // but not values of references
    std::swap(refbeg, refend);
    // (2) not possible as it would use move semantics on temporaries
    // std::swap(*(arr.begin()), *(arr.end()-1));
    // (3) works as internally unqualified swap is used, values are swapped
    std::iter_swap(arr.begin(), arr.end()-1);
    // same as (1)
    ::std::swap(refbeg, refend);
    // same as (2)
    // ::std::swap(*(arr.begin()), *(arr.end()-1));
    // same as (3)
    ::std::iter_swap(arr.begin(), arr.end()-1);
    }
  }
    //printf(">>>>>>>>>>>>> END OF StlAlgoTest mit CompilerADLTest in rank %d <<<<< \n", dash::myid());
  
}

TEST_F(StlAlgoTest, Swap) {
  //printf(">>>>>>> STLAlgoTest mit Swap in rank %d <<<<< \n", dash::myid());
  using std::swap;
  dash::Array<int> arr(dash::size());
  arr.local[0] = dash::myid();
  arr.barrier();
  //printf("After 1st swap barrier");

  if(dash::myid() == 0){
    auto refbeg = *(arr.begin());
    auto refend = *(arr.end()-1);
    refbeg.swap(refend);
    //swap(*(arr.begin()), *(arr.end()-1));
  }
  arr.barrier();
  int newbegval = arr[0];
  int newendval = arr[dash::size()-1];
  ASSERT_EQ_U(newbegval, dash::size()-1);
  ASSERT_EQ_U(newendval, 0);
}

TEST_F(StlAlgoTest, IterSwap) {
  //printf(">>>>>>> STLAlgoTest mit IterSwap in rank %d <<<<< \n", dash::myid());
  dash::Array<int> arr(dash::size());
  arr.local[0] = dash::myid();
  arr.barrier();

  if(dash::myid() == 0){
    std::iter_swap(arr.begin(), arr.end()-1);
  }
  arr.barrier();
  int newbegval = arr[0];
  int newendval = arr[dash::size()-1];
  ASSERT_EQ_U(newbegval, dash::size()-1);
  ASSERT_EQ_U(newendval, 0);
}

TEST_F(StlAlgoTest, Sort) {
  //printf(">>>>>>> STLAlgoTest mit Sort in rank %d <<<<< \n", dash::myid());
  // do not use that in production as this is inefficient as hell!
  dash::Array<int> arr(dash::size());
  // init array in reverse order [n-1,n-2,...,0]
  int locval = dash::size()-1 - dash::myid();
  arr.local[0] = locval;
  arr.barrier();

  if(dash::myid() == 0){
    std::sort(arr.begin(), arr.end());
  }
  arr.barrier();

  locval = arr.local[0];
  ASSERT_EQ_U(locval, static_cast<int>(dash::myid()));
}
