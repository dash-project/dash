#ifndef DASH__ALGORITHM__ACCUMULATE_H__
#define DASH__ALGORITHM__ACCUMULATE_H__

#include <dash/GlobIter.h>
#include <dash/algorithm/LocalRange.h>
#include <dash/algorithm/Operation.h>

namespace dash {

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
template<
  class GlobInputIt,
  class ValueType,
  class BinaryOperation >
ValueType accumulate(
  GlobInputIt     in_first,
  GlobInputIt     in_last,
  ValueType       init,
  BinaryOperation binary_op = dash::plus<ValueType>())
{
  DASH_THROW(
    dash::exception::NotImplemented,
    "dash::accumulate is not implemented");
}

} // namespace dash

#endif // DASH__ALGORITHM__ACCUMULATE_H__
