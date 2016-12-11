#ifndef DASH__ALGORITHM__ACCUMULATE_H__
#define DASH__ALGORITHM__ACCUMULATE_H__

#include <dash/iterator/GlobIter.h>
#include <dash/algorithm/LocalRange.h>
#include <dash/algorithm/Operation.h>

namespace dash {

/**
 * Accumulate values in range \c [first, last) as the sum of all values
 * in the range.
 *
 * Note: For equivalent of semantics of \c MPI_Accumulate, see
 * \c dash::transform.
 *
 * Semantics:
 *
 *     acc = init (+) in[0] (+) in[1] (+) ... (+) in[n]
 *
 * \see      dash::transform
 *
 * \ingroup  DashAlgorithms
 */
template <
  class GlobInputIt,
  class ValueType >
ValueType accumulate(
  GlobInputIt     in_first,
  GlobInputIt     in_last,
  ValueType       init)
{
  typedef typename GlobInputIt::index_type index_t;

  auto & team      = in_first.team();
  auto myid        = team.myid();
  auto index_range = dash::local_range(in_first, in_last);
  auto l_first     = index_range.begin;
  auto l_last      = index_range.end;
  auto l_result    = std::accumulate(l_first, l_last, init);
  auto result      = 0;

  dash::Array<index_t> l_results(team.size());
  l_results.local[0] = l_result;

  team.barrier();

  if (myid == 0) {
    for (int i = 0; i < team.size(); i++) {
      result += l_results[i];
    }
  }
  return result;
}

/**
 * Accumulate values in range \c [first, last) using the given binary
 * reduce function \c op.
 *
 * Collective operation.
 *
 * Note: For equivalent of semantics of \c MPI_Accumulate, see
 * \c dash::transform.
 *
 * Semantics:
 *
 *     acc = init (+) in[0] (+) in[1] (+) ... (+) in[n]
 *
 * \see      dash::transform
 *
 * \ingroup  DashAlgorithms
 */
template <
  class GlobInputIt,
  class ValueType,
  class BinaryOperation >
ValueType accumulate(
  GlobInputIt     in_first,
  GlobInputIt     in_last,
  ValueType       init,
  BinaryOperation binary_op = dash::plus<ValueType>())
{
  typedef typename GlobInputIt::index_type index_t;

  auto & team      = in_first.team();
  auto myid        = team.myid();
  auto index_range = dash::local_range(in_first, in_last);
  auto l_first     = index_range.begin;
  auto l_last      = index_range.end;
  auto l_result    = std::accumulate(l_first, l_last, init, binary_op);
  auto result      = 0;

  dash::Array<index_t> l_results(team.size());
  l_results.local[0] = l_result;

  team.barrier();

  if (myid == 0) {
    for (int i = 0; i < team.size(); i++) {
      result = binary_op(result, l_results[i]);
    }
  }
  return result;
}

} // namespace dash

#endif // DASH__ALGORITHM__ACCUMULATE_H__
