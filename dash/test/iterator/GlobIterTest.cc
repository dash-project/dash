
#include "GlobIterTest.h"

#include <dash/Array.h>
#include <algorithm>

TEST_F(GlobIterTest, CompilerADLTest){
  using std::swap;
  dash::Array<int> arr(dash::size());
  arr.local[0] = static_cast<int>(dash::myid());
  arr.barrier();

  if(dash::myid() == 0){
    auto refbeg = *(arr.begin());
    auto refend = *(arr.end()-1);
    refbeg.swap(refend);
    swap(refbeg, refend);
    swap(*(arr.begin()), *(arr.end()-1));

    std::swap(refbeg, refend);
    std::swap(*(arr.begin()), *(arr.end()-1));

    iter_swap(arr.begin(), arr.end()-1);
    std::iter_swap(arr.begin(), arr.end()-1);
  }

}
#if 1
TEST_F(GlobIterTest, Swap) {
  using std::swap;
  dash::Array<int> arr(dash::size());
  arr.local[0] = static_cast<int>(dash::myid());
  arr.barrier();

  if(dash::myid() == 0){
    auto refbeg = *(arr.begin());
    auto refend = *(arr.end()-1);
    refbeg.swap(refend);
    //swap(*(arr.begin()), *(arr.end()-1));
  }
  arr.barrier();
  int newbegval = arr[0];
  int newendval = arr[static_cast<int>(dash::size()-1)];
  ASSERT_EQ_U(newbegval, dash::size()-1);
  ASSERT_EQ_U(newendval, 0);
}
#endif
#if 1
TEST_F(GlobIterTest, IterSwap) {
  dash::Array<int> arr(dash::size());
  arr.local[0] = static_cast<int>(dash::myid());
  arr.barrier();

  if(dash::myid() == 0){
    std::iter_swap(arr.begin(), arr.end()-1);
  }
  arr.barrier();
  int newbegval = arr[0];
  int newendval = arr[static_cast<int>(dash::size()-1)];
  ASSERT_EQ_U(newbegval, dash::size()-1);
  ASSERT_EQ_U(newendval, 0);
}
#endif
#if 1
/**
 * This test does not work as there is a bug in many C++ STLs:
 * Internally std::iter_swap / std::swap is used, and hence
 * the user-defined swap operation is not found
 */
TEST_F(GlobIterTest, Sort) {
  // do not use that in production as this is inefficient as hell!
  dash::Array<int> arr(dash::size());
  // init array in reverse order [n-1,n-2,...,0]
  int locval = dash::size()-1 - dash::myid();
  arr.local[0] = locval;
  arr.barrier();

  std::sort(arr.begin(), arr.end());
  arr.barrier();

  locval = arr.local[0];
  ASSERT_EQ_U(locval, static_cast<int>(dash::myid()));
}
#endif
