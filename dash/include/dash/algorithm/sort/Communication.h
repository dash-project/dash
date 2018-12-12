#ifndef DASH__ALGORITHM__SORT__COMMUNICATION_H
#define DASH__ALGORITHM__SORT__COMMUNICATION_H

#include <dash/algorithm/Operation.h>
#include <dash/iterator/IteratorTraits.h>

namespace dash {

template <
    class LocalInputIter,
    class LocalOutputIter,
    class BinaryOperation = dash::plus<
        typename dash::iterator_traits<LocalInputIter>::value_type>,
    typename = typename std::enable_if<
        !dash::iterator_traits<LocalInputIter>::is_global_iterator::value &&
        !dash::iterator_traits<LocalOutputIter>::is_global_iterator::value>::
        type>
LocalOutputIter exclusive_scan(
    LocalInputIter                                             in_first,
    LocalInputIter                                             in_last,
    LocalOutputIter                                            out_first,
    typename dash::iterator_traits<LocalInputIter>::value_type init,
    BinaryOperation   op   = BinaryOperation{},
    dash::Team const& team = dash::Team::All())
{
  using value_t = typename dash::iterator_traits<LocalInputIter>::value_type;

  auto nel = std::distance(in_first, in_last);

  DASH_ASSERT_EQ(nel, team.size(), "invalid number of elements to scan");

  DASH_ASSERT_RETURNS(
      dart_exscan(
          // send buffer
          std::addressof(*in_first),
          // receive buffer
          std::addressof(*out_first),
          // buffer size
          nel,
          // data type
          dash::dart_datatype<value_t>::value,
          // operation
          dash::internal::dart_reduce_operation<BinaryOperation>::value,
          // team
          team.dart_id()),
      DART_OK);

  return std::next(out_first, nel);
}

}  // namespace dash
#endif
