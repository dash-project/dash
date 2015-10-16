#ifndef DASH__ALGORITHM__TRANSFORM_H__
#define DASH__ALGORITHM__TRANSFORM_H__

#include <dash/GlobIter.h>
#include <dash/algorithm/LocalRange.h>
#include <dash/algorithm/Operation.h>
#include <dash/dart/if/dart_communication.h>

namespace dash {
namespace internal {

/**
 * Generic interface of the blocking DART accumulate operation.
 */
template< typename ValueType >
dart_ret_t accumulate_blocking_impl(
  dart_gptr_t        dest,
  ValueType        * values,
  size_t             nvalues,
  dart_operation_t   op,
  dart_team_t        team);

/**
 * Specialization of the blocking DART accumulate operation for value type
 * \c int.
 */
template<>
inline dart_ret_t accumulate_blocking_impl<int>(
  dart_gptr_t        dest,
  int              * values,
  size_t             nvalues,
  dart_operation_t   op,
  dart_team_t        team) {
  dart_ret_t result = dart_accumulate_int(dest, values, nvalues, op, team);
  dart_flush(dest);
  return result;
}

/**
 * Generic interface of the non-blocking DART accumulate operation.
 */
template< typename ValueType >
dart_ret_t accumulate_impl(
  dart_gptr_t        dest,
  ValueType        * values,
  size_t             nvalues,
  dart_operation_t   op,
  dart_team_t        team);

/**
 * Specialization of the non-blocking DART accumulate operation for value type
 * \c int.
 */
template<>
inline dart_ret_t accumulate_impl<int>(
  dart_gptr_t        dest,
  int              * values,
  size_t             nvalues,
  dart_operation_t   op,
  dart_team_t        team) {
  dart_ret_t result = dart_accumulate_int(dest, values, nvalues, op, team);
  dart_flush_local(dest);
  return result;
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
 *
 * \ingroup  DashAlgorithms
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
 *   gptr_diff_t num_transformed_elements =
 *                 dash::distance(
 *                   dash::transform(in.begin(), in.end(),  // A
 *                                   out.begin(),           // B
 *                                   out.begin(),           // C = op(A, B)
 *                                   dash::plus<int>()),    // op
 *                   out.end());
 *
 * \endcode
 *
 * \returns  Output iterator to the element past the last element transformed.
 * \see      dash::accumulate
 * \see      DashReduceOperations
 *
 * \tparam   InputIt         Iterator on first (typically local) input range
 * \tparam   GlobInputIt     Iterator on second (typically global) input range
 * \tparam   GlobOutputIt    Iterator on global result range
 * \tparam   BinaryOperation Reduce operation type
 *
 * \ingroup  DashAlgorithms
 */
template<
  typename ValueType,
  class InputIt,
  class GlobInputIt,
  class GlobOutputIt,
  class BinaryOperation >
GlobOutputIt transform(
  InputIt         in_a_first,
  InputIt         in_a_last,
  GlobInputIt     in_b_first,
  GlobOutputIt    out_first,
  BinaryOperation binary_op     = dash::plus<ValueType>()) {
  // Outut range different from rhs input range is not supported yet
  DASH_ASSERT(in_b_first == out_first);
  // Resolve team from global iterator:
  dash::Team & team             = out_first.pattern().team();
  // Resolve local range from global range:
  // Number of elements in local range:
  size_t num_local_elements     = std::distance(in_a_first, in_a_last);
  // Global iterator to dart_gptr_t:
  dart_gptr_t dest_gptr         = ((GlobPtr<ValueType>) out_first).dart_gptr();
  // Send accumulate message:
  dash::internal::accumulate_blocking_impl(
      dest_gptr,
      in_a_first,
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

/**
 * Specialization of \c dash::transform for global lhs input range.
 */
template<
  typename ValueType,
  class GlobInputIt,
  class GlobOutputIt,
  class BinaryOperation >
GlobOutputIt transform(
  GlobIter<ValueType> in_a_first,
  GlobIter<ValueType> in_a_last,
  GlobInputIt         in_b_first,
  GlobOutputIt        out_first,
  BinaryOperation     binary_op = dash::plus<ValueType>()) {
  // Outut range different from rhs input range is not supported yet
  DASH_ASSERT(in_b_first == out_first);
  // Pattern from global iterator, assume identical pattern in all ranges:
  auto pattern                  = out_first.pattern();
  // Resolve team from global iterator:
  dash::Team & team             = pattern.team();
  // Resolve local range from global range:
  auto l_index_range            = local_index_range(in_a_first, in_a_last);
  DASH_LOG_TRACE_VAR("dash::transform", l_index_range.begin);
  DASH_LOG_TRACE_VAR("dash::transform", l_index_range.end);
  // Local range to global offset:
  auto global_offset            = pattern.global(
                                    l_index_range.begin);
  DASH_LOG_TRACE_VAR("dash::transform", global_offset);
  // Number of elements in local range:
  size_t num_local_elements     = l_index_range.end - l_index_range.begin;
  DASH_LOG_TRACE_VAR("dash::transform", num_local_elements);
  // Global iterator to dart_gptr_t:
  dart_gptr_t dest_gptr         = ((GlobPtr<ValueType>)
                                   (out_first + global_offset)).dart_gptr();
  // Native pointer to local sub-range:
  ValueType * l_values          = ((GlobPtr<ValueType>)
                                   (in_a_first + global_offset));
  // Send accumulate message:
  dash::internal::accumulate_blocking_impl(
      dest_gptr,
      l_values,
      num_local_elements,
      binary_op.dart_operation(),
      team.dart_id());
  return out_first + global_offset + num_local_elements;
}

/**
 * Specialization of \c dash::transform as non-blocking operation.
 *
 * \tparam   InputIt         Iterator on first (typically local) input range
 * \tparam   GlobInputIt     Iterator on second (typically global) input range
 * \tparam   GlobOutputIt    Iterator on global result range
 * \tparam   BinaryOperation Reduce operation type
 */
template<
  typename ValueType,
  class InputIt,
  class GlobInputIt,
  class BinaryOperation >
GlobAsyncRef<ValueType> transform(
  InputIt                 in_a_first,
  InputIt                 in_a_last,
  GlobInputIt             in_b_first,
  GlobAsyncRef<ValueType> out_first,
  BinaryOperation         binary_op   = dash::plus<ValueType>()) {
}

} // namespace dash

#endif // DASH__ALGORITHM__TRANSFORM_H__
