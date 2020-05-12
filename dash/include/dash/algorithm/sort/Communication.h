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

namespace impl {
template <class InputIt, class OutputIt>
void alltoallv(
    InputIt             input,
    OutputIt            output,
    std::vector<size_t> sendCounts,
    std::vector<size_t> sendDispls,
    std::vector<size_t> targetCounts,
    std::vector<size_t> targetDispls,
    dart_team_t         dartTeam)
{
  using value_type = typename std::iterator_traits<InputIt>::value_type;

  // check whether value_type is supported by dart, else switch to byte
  auto dart_value_t = dash::dart_datatype<value_type>::value;
  if (dart_value_t == DART_TYPE_UNDEFINED) {
    dart_value_t = DART_TYPE_BYTE;

    auto to_bytes = [](auto v) { return v * sizeof(value_type); };

    std::transform(
        sendCounts.begin(), sendCounts.end(), sendCounts.begin(), to_bytes);
    std::transform(
        sendDispls.begin(), sendDispls.end(), sendDispls.begin(), to_bytes);
    std::transform(
        targetCounts.begin(),
        targetCounts.end(),
        targetCounts.begin(),
        to_bytes);
    std::transform(
        targetDispls.begin(),
        targetDispls.end(),
        targetDispls.begin(),
        to_bytes);
  }

  DASH_ASSERT_RETURNS(
      dart_alltoallv(
          input,
          output,
          sendCounts.data(),
          sendDispls.data(),
          targetCounts.data(),
          targetDispls.data(),
          dart_value_t,
          dartTeam),
      DART_OK);
}
}  // namespace impl

}  // namespace dash
#endif
