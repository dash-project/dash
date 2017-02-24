#ifndef DASH__EXECUTION_H__INCLUDED
#define DASH__EXECUTION_H__INCLUDED

#include <type_traits>


namespace dash {

/**
 * Execution policy type trait.
 */
template<class T>
struct is_execution_policy
: public std::integral_constant<bool, false>
{ };

namespace execution {

  /**
   * Sequential execution policy.
   */
  class sequenced_policy { };

  /**
   * Parallel execution policy.
   */
  class parallel_policy { };

  /**
   * Parallel non-sequential execution policy.
   */
  class parallel_unsequenced_policy { };

  constexpr sequenced_policy             seq;
  constexpr parallel_policy              par;
  constexpr parallel_unsequenced_policy  par_unseq;

} // namespace execution

/**
 * Specialization of execution policy type trait for sequential execution
 * policy.
 */
template<>
struct is_execution_policy<dash::execution::sequenced_policy>
: public std::integral_constant<bool, true>
{ };

/**
 * Specialization of execution policy type trait for parallel execution
 * policy.
 */
template<>
struct is_execution_policy<dash::execution::parallel_policy>
: public std::integral_constant<bool, true>
{ };

/**
 * Specialization of execution policy type trait for parallel non-sequential
 * execution policy.
 */
template<>
struct is_execution_policy<dash::execution::parallel_unsequenced_policy>
: public std::integral_constant<bool, true>
{ };

} // namespace dash

#endif // DASH__EXECUTION_H__INCLUDED
