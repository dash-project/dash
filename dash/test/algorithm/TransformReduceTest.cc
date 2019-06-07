#include "TransformReduceTest.h"

#include <dash/algorithm/Transform.h>

#include <dash/Array.h>

#include <array>


TEST_F(TransformReduceTest, ArrayMinReduce)
{
  const size_t nlocal = 5;
  size_t ntotal = nlocal * dash::size();
  using value_type    = int;

  dash::Array<value_type> array(ntotal);

  std::iota(array.lbegin(), array.lend(), nlocal * dash::myid());
  array.barrier();

  auto const init      = std::numeric_limits<value_type>::max();
  auto       reduce_op = [](const value_type &lhs, const value_type &rhs) {
    return std::min(lhs, rhs);
  };
  auto map_op = [](const value_type &a) { return a + nlocal; };

  std::cout << "init: " << init << std::endl;

  auto result = dash::transform_reduce(
      array.begin(), array.end(), init, reduce_op, map_op);

  EXPECT_EQ_U(result, nlocal);
}
