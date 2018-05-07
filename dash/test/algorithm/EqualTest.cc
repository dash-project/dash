
#include "EqualTest.h"

#include <dash/Array.h>
#include <dash/algorithm/Fill.h>
#include <dash/algorithm/Equal.h>

#include <array>

TEST_F(EqualTest, EqualDistribution){
  using value_type = int;
  using array_type = dash::Array<value_type>;

  /// Using a prime to cause inconvenient strides
  size_t num_local_elem = 513;

  array_type A(num_local_elem, dash::BLOCKED);
  array_type B(num_local_elem, dash::BLOCKED);

  auto beg_a = A.begin() + 10;
  auto end_a = A.begin() + 501;
  auto beg_b = B.begin() + 10;
  auto end_b = B.begin() + 501;

  dash::fill(beg_a, end_a, 1);
  dash::fill(beg_b, end_b, 1);
  A.flush();
  B.flush();
  dash::barrier();

  bool ab1_result = dash::equal(beg_a, end_a, beg_b);
  bool ab2_result = dash::equal(beg_a, end_a, beg_b-1);

  // matching
  EXPECT_EQ_U(ab1_result, true);
  // mismatch
  EXPECT_EQ_U(ab2_result, false);
}


// TODO: This is currently not supported.
// Re-enable testcase when implementation is ready
#if 0
TEST_F(EqualTest, OverlappingRanges)
{
  using value_type = int;
  using array_type = dash::Array<value_type>;
  /// Using a prime to cause inconvenient strides
  size_t num_local_elem = 513;

  array_type A(num_local_elem, dash::BLOCKED);
  array_type B(num_local_elem, dash::CYCLIC);

  // interesting ranges:
  // A: [10,501)
  // B: [ 3,494)
  auto beg_a = A.begin() + 10;
  auto end_a = A.begin() + 501;
  auto beg_b = B.begin() + 3;
  auto end_b = B.begin() + 494;

  dash::fill(beg_a, end_a, 1);
  dash::fill(beg_b, end_b, 1);
  A.flush();
  B.flush();
  dash::barrier();

  bool ab1_result = dash::equal(beg_a, end_a, beg_b);
  bool ab2_result = dash::equal(beg_a, end_a, beg_b+1);

  // matching
  EXPECT_EQ_U(ab1_result, true);
  // mismatch
  EXPECT_EQ_U(ab2_result, false);
}
#endif
