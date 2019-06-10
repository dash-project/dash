#include "TransformReduceTest.h"

#include <dash/Types.h>
#include <dash/algorithm/Operation.h>
#include <dash/algorithm/Transform.h>

#include <dash/Array.h>

#include <array>

TEST_F(TransformReduceTest, ArraySum)
{
  const size_t nlocal = 5;
  size_t       ntotal = nlocal * dash::size();
  using value_type    = int;

  dash::Array<value_type> array(ntotal);

  std::iota(array.lbegin(), array.lend(), nlocal * dash::myid() + 1);
  array.barrier();

  value_type const init = 0;
  using reduce_op_t     = dash::plus<value_type>;
  using map_op_t        = std::multiplies<>;

  auto map_op = std::bind(map_op_t(), std::placeholders::_1, 1);

  auto result = dash::transform_reduce(
      array.begin(), array.end(), init, reduce_op_t(), map_op);

  auto const expected = (ntotal * ntotal + ntotal) / 2;
  EXPECT_EQ_U(expected, result);
}

TEST_F(TransformReduceTest, ArraySumRooted)
{
  const size_t nlocal = 5;
  size_t       ntotal = nlocal * dash::size();
  using value_type    = int;

  dash::Array<value_type> array(ntotal);

  std::iota(array.lbegin(), array.lend(), nlocal * dash::myid() + 1);
  array.barrier();

  value_type const init = 0;
  using reduce_op_t     = dash::plus<value_type>;
  using map_op_t        = std::multiplies<>;

  auto map_op = std::bind(map_op_t(), std::placeholders::_1, 1);

  auto root = dash::team_unit_t{0};

  auto result = dash::transform_reduce(
      array.begin(), array.end(), init, reduce_op_t(), map_op, root);

  if (array.team().myid() == root) {
    auto const expected = (ntotal * ntotal + ntotal) / 2;
    EXPECT_EQ_U(expected, result);
  }
}
