
#include "GlobIterTest.h"

#include <dash/Array.h>


TEST_F(GlobIterTest, Swap) {
  dash::Array<int> arr(dash::size());
  arr.local[0] = static_cast<int>(dash::myid());
  arr.barrier();

  auto beg = arr.begin();
  auto end = arr.end();
  if(dash::myid() == 0){
    auto begref = *beg;
    auto endref = *end;
    std::swap(begref, endref);
  }
  arr.barrier();
  int newbegval = arr[0];
  int newendval = arr[static_cast<int>(dash::size()-1)];
  ASSERT_EQ_U(newbegval, dash::size()-1);
  ASSERT_EQ_U(newendval, 0);
}

TEST_F(GlobIterTest, Sort) {

}

