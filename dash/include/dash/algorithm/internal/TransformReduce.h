#include <dash/algorithm/LocalRange.h>

namespace dash {
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

  auto accu = init;
  while (lbegin != lend) {
    accu = binary_op(accu, unary_op(*lbegin++));
  }
  return accu;
}
}  // namespace dash
