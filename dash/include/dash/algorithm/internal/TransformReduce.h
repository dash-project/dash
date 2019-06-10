#include <dash/algorithm/LocalRange.h>
#include <dash/algorithm/Reduce.h>
#include <dash/iterator/IteratorTraits.h>

namespace dash {

namespace detail {

template <class InputIt, class T, class UnaryOperation, class BinaryOperation>
T local_transform_reduce_simple(
    InputIt         lbegin,
    InputIt         lend,
    T               init,
    BinaryOperation binary_op,
    UnaryOperation  unary_op)
{
  // Transform-reduce locally in one step
  auto accu = init;
  while (lbegin != lend) {
    accu = binary_op(accu, unary_op(*lbegin++));
  }
  return accu;
}
}  // namespace detail

/**
 * Transformes and reduces a range of values
 *
 * First unary_op is applied to each element and the result is reduced using
 * binary_op with the starting value of init.
 *
 * Note: binary_op must be communitative and associative.
 */
template <class InputIt, class T, class UnaryOperation, class BinaryOperation>
T transform_reduce(
    InputIt         in_first,
    InputIt         in_last,
    T               init,
    BinaryOperation binary_op,
    UnaryOperation  unary_op)
{
  auto local_range = dash::local_range(in_first, in_last);
  auto lbegin      = local_range.begin;
  auto lend        = local_range.end;

  auto local_result = detail::local_transform_reduce_simple(
      lbegin, lend, init, binary_op, unary_op);

  // Use dash's reduce for the non-local parts
  return reduce(
      std::addressof(local_result),
      std::next(std::addressof(local_result), 1),
      init,
      binary_op,
      true,
      in_first.team());
}

/**
 * Transformes and reduces a range of values into a root
 *
 * First unary_op is applied to each element and the result is reduced using
 * binary_op with the starting value of init.
 *
 * Note: The return value on all other units except root is undefined.
 *
 * Note: binary_op must be communitative and associative.
 */
template <class InputIt, class T, class UnaryOperation, class BinaryOperation>
T transform_reduce(
    InputIt         in_first,
    InputIt         in_last,
    T               init,
    BinaryOperation binary_op,
    UnaryOperation  unary_op,
    team_unit_t     root)
{
  auto local_range = dash::local_range(in_first, in_last);
  auto lbegin      = local_range.begin;
  auto lend        = local_range.end;

  auto local_result = detail::local_transform_reduce_simple(
      lbegin, lend, init, binary_op, unary_op);

  // Use dash's reduce for the non-local parts
  return reduce(
      std::addressof(local_result),
      std::next(std::addressof(local_result), 1),
      init,
      binary_op,
      true,
      in_first.team(),
      &root);
}
}  // namespace dash
