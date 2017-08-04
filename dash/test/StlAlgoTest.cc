
#include "StlAlgoTest.h"

#include <dash/Array.h>
#include <algorithm>

TEST_F(StlAlgoTest, CompilerADLTest){
  dash::Array<int> arr(dash::size());
  arr.local[0] = dash::myid();
  arr.barrier();

  if(dash::myid() == 0){
    auto refbeg = *(arr.begin());
    auto refend = *(arr.end()-1);
    {
    using std::swap;
    using std::iter_swap;
    refbeg.swap(refend);
    swap(refbeg, refend);
    swap(*(arr.begin()), *(arr.end()-1));
    iter_swap(arr.begin(), arr.end()-1);
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
}

TEST_F(StlAlgoTest, Swap) {
  using std::swap;
  dash::Array<int> arr(dash::size());
  arr.local[0] = dash::myid();
  arr.barrier();

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
