#ifndef DASH__ALGORITHM__ACCUMULATE_H__
#define DASH__ALGORITHM__ACCUMULATE_H__

#include <dash/Array.h>
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
  struct local_result {
    ValueType l_result;
    bool      l_valid;
  };

  auto & team      = in_first.team();
  auto myid        = team.myid();
  auto index_range = dash::local_range(in_first, in_last);
  auto l_first     = index_range.begin;
  auto l_last      = index_range.end;
  auto result      = init;
  dash::Array<local_result> l_results(team.size(), team);
  if (l_first != l_last) {
    auto l_result  = std::accumulate(std::next(l_first), l_last, *l_first);
    l_results.local[0].l_result = l_result;
    l_results.local[0].l_valid  = true;
  } else {
    l_results.local[0].l_valid  = false;
  }

  l_results.barrier();

  if (myid == 0) {
    for (size_t i = 0; i < team.size(); i++) {
      local_result lr = l_results[i];
      if (lr.l_valid) {
        result += lr.l_result;
      }
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
  struct local_result {
    ValueType l_result;
    bool      l_valid;
  };

  auto & team      = in_first.team();
  auto myid        = team.myid();
  auto index_range = dash::local_range(in_first, in_last);
  auto l_first     = index_range.begin;
  auto l_last      = index_range.end;
  auto result      = init;

  dash::Array<local_result> l_results(team.size(), team);
  if (l_first != l_last) {
    auto l_result  = std::accumulate(std::next(l_first), l_last, *l_first, binary_op);
    l_results.local[0].l_result = l_result;
    l_results.local[0].l_valid  = true;
  } else {
    l_results.local[0].l_valid  = false;
  }

  l_results.barrier();

  if (myid == 0) {
    for (size_t i = 0; i < team.size(); i++) {
      local_result lr = l_results[i];
      if (lr.l_valid) {
        result = binary_op(result, lr.l_result);
      }
    }
  }
  return result;
}

} // namespace dash

#endif // DASH__ALGORITHM__ACCUMULATE_H__
