#ifndef DASH__ALGORITHM__TRANSFORM_H__
#define DASH__ALGORITHM__TRANSFORM_H__

#include <dash/GlobIter.h>
#include <dash/algorithm/LocalRange.h>
#include <dash/algorithm/Operation.h>
#include <dash/dart/if/dart_communication.h>

namespace dash {
namespace internal {

/**
 * Generic interface of the DART accumulate operation.
 */
template< typename ValueType >
dart_ret_t accumulate_blocking_impl(
  dart_gptr_t        dest,
  ValueType        * values,
  size_t             nvalues,
  dart_operation_t   op,
  dart_team_t        team);

/**
 * Specialization of DART accumulate operation for value type \c int.
 */
template<>
inline dart_ret_t accumulate_blocking_impl<int>(
  dart_gptr_t        dest,
  int              * values,
  size_t             nvalues,
  dart_operation_t   op,
  dart_team_t        team) {
  return dart_accumulate_int(dest, values, nvalues, op, team);
}

} // namespace internal

/**
 * Apply a given function to elements in a range and store the result in
 * another range, beginning at \c out_first.
 * Corresponding to \c MPI_Accumulate, the binary operation is executed
 * atomically on single elements.
 *
 * Precondition: All elements in the input range are contained in a single
 * block so that
 *
 *   g_out_last == g_out_first + (l_in_last - l_in_first)
 *
 * Semantics:
 *
 *   unary_op(in_first[0]), unary_op(in_first[1]), ..., unary_op(in_first[n])
 *
 * \returns  Output iterator to the element past the last element transformed.
 */
template<
  typename ValueType,
  class GlobInputIt,
  class GlobOutputIt,
  class UnaryOperation >
GlobOutputIt transform(
  GlobInputIt    in_first,
  GlobInputIt    in_last,
  GlobOutputIt   out_first,
  UnaryOperation unary_op) {
  // TODO: Implement using user-defined reduce function
  return in_last;
}

/**
 * Apply a given function to pairs of elements from two ranges and store the
 * result in another range, beginning at \c out_first.
 *
 * Corresponding to \c MPI_Accumulate, the binary operation is executed
 * atomically on single elements.
 *
 * Precondition: All elements in the input range are contained in a single
 * block so that
 *
 *   g_out_last == g_out_first + (l_in_last - l_in_first)
 *
 * Semantics:
 *
 *   binary_op(in_a[0], in_b[0]),
 *   binary_op(in_a[1], in_b[1]),
 *   ...,
 *   binary_op(in_a[n], in_b[n])
 *
 * Example:
 * \code
 *   gptr_diff_t num_transformed_values =
 *                 dash::distance(
 *                   dash::transform(in.begin(), in.end(),
 *                                   out.begin(),
 *                                   dash::plus<int>()),
 *                   out.end());
 *
 * \endcode
 *
 * \returns  Output iterator to the element past the last element transformed.
 * \see      dash::accumulate
 */
template<
  typename ValueType,
  class GlobInputIt,
  class GlobOutputIt,
  class BinaryOperation >
GlobOutputIt transform(
  GlobInputIt     in_a_first,
  GlobInputIt     in_a_last,
  GlobInputIt     in_b_first,
  GlobOutputIt    out_first,
  BinaryOperation binary_op     = dash::plus<ValueType>()) {
  // Resolve team from input iterator:
  dash::Team & team             = in_a_first.team().dart_team();
  // Resolve local range from global range:
  LocalRange<ValueType> l_range = local_range(in_a_first, in_a_last);
  // Number of elements in local range:
  size_t num_local_elements     = std::distance(l_range.begin, l_range.end);
  // Global iterator to dart_gptr_t:
  dart_gptr_t dest_gptr         = (GlobPtr<ValueType>)(out_first).dart_gptr();
  // Send accumulate message:
  dash::internal::accumulate_blocking_impl(
      dest_gptr,
      l_range.begin,
      num_local_elements,
      binary_op.dart_operation(),
      team.dart_id());
  // The position past the last element transformed in global element space
  // cannot be resolved from the size of the local range if the local range
  // spans over more than one block. Otherwise, the difference of two global
  // iterators is not well-defined. The corresponding invariant is:
  //   g_out_last == g_out_first + (l_in_last - l_in_first)
  // Example: 
  //   unit:            0       1       0
  //   local offset:  | 0 1 2 | 0 1 2 | 3 4 5 | ...
  //   global offset: | 0 1 2   3 4 5   6 7 8   ...
  //   range:          [- - -           - -]
  // When iterating in local memory range [0,5[ of unit 0, the position of the
  // global iterator to return is 8 != 5
  // For ranges over block borders, we would have to resolve the global
  // position past the last element transformed from the iterator's pattern
  // (see dash::PatternIterator).
  return out_first + num_local_elements;
}

} // namespace dash

#endif // DASH__ALGORITHM__TRANSFORM_H__
