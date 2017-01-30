#ifndef DASH__ATOMIC_OPERATION_H_
#define DASH__ATOMIC_OPERATION_H_

#include <dash/atomic/GlobRefAtomic.h>

namespace dash {

// forward decls
template<typename T>
class Atomic;

/**
 * Routines to perform atomic operations on atomics residing in the
 * global address space
 *
 * \code
 *  int N = dash::size();
 *  dash::Array<dash::Atomic<int>> array(N);
 *  dash::fill(array.begin(), array.end(), 0);
 *  // each unit adds 1 to each array position
 *  for(auto & el : array){
 *    dash::atomic::add(el, 1);
 *  }
 *  // postcondition:
 *  // array = {N,N,N,N,...}
 * \endcode
 */
namespace atomic {

/**
 * Get a reference on the shared atomic value.
 */
template<typename T>
T load(const dash::GlobRef<dash::Atomic<T>> & ref){
  return static_cast<T>(dash::AtomicAddress<T>(ref).get());
}

/**
 * Set the value of the atomic reference 
 */
template<typename T>
void store(const dash::GlobRef<dash::Atomic<T>> & ref,
           const T & value)
{
  dash::AtomicAddress<T>(ref).set(value);
}

/**
 * Atomically sets the value of the atomic reference and returns
 * the old value
 */
template<typename T>
T exchange(const dash::GlobRef<dash::Atomic<T>> & ref,
           const T & value)
{
  return dash::AtomicAddress<T>(ref).exchange(value);
}

/**
 * Atomically compares the value with the value of expected and if those are
 * bitwise-equal, replaces the former with desired.
 * 
 * \return  True if value is exchanged
 */
template<typename T>
bool compare_exchange(
  const dash::GlobRef<dash::Atomic<T>> & ref,
  const T & expected,
  const T & desired)
{
  return dash::AtomicAddress<T>(ref).compare_exchange(expected, desired);
}

/**
 * Atomically executes specified operation on the referenced shared value.
 */
template<
  typename T,
  typename BinaryOp >
void op(
  const dash::GlobRef<dash::Atomic<T>> & ref,
  const BinaryOp  binary_op,
  /// Value to be added to global atomic variable.
  const T & value)
{
  dash::AtomicAddress<T>(ref).op(binary_op, value);
}

/**
 * Atomic fetch-and-op operation on the referenced shared value.
 *
 * \return  The value of the referenced shared variable before the
 *          operation.
 */
template<
  typename T,
  typename BinaryOp >
T fetch_and_op(
  const dash::GlobRef<dash::Atomic<T>> & ref,
  const BinaryOp  binary_op,
  /// Value to be added to global atomic variable.
  const T & value)
{
  return dash::AtomicAddress<T>(ref).fetch_and_op(binary_op, value);
}

/**
 * Atomic add operation on the referenced shared value.
 */
template<typename T>
typename std::enable_if<
  std::is_integral<T>::value,
  void>::type
add(
  const dash::GlobRef<dash::Atomic<T>> & ref,
  const T & value)
{
  dash::AtomicAddress<T>(ref).add(value);
}

/**
 * Atomic subtract operation on the referenced shared value.
 */
template<typename T>
typename std::enable_if<
  std::is_integral<T>::value,
  void>::type
sub(
  const dash::GlobRef<dash::Atomic<T>> & ref,
  const T & value)
{
  dash::AtomicAddress<T>(ref).sub(value);
}

/**
 * Atomic fetch-and-add operation on the referenced shared value.
 *
 * \return  The value of the referenced shared variable before the
 *          operation.
 */
template<typename T>
typename std::enable_if<
  std::is_integral<T>::value,
  T>::type
fetch_add(
  const dash::GlobRef<dash::Atomic<T>> & ref,
  /// Value to be added to global atomic variable.
  const T & value)
{
  return dash::AtomicAddress<T>(ref).fetch_and_add(value);
}

/**
 * Atomic fetch-and-sub operation on the referenced shared value.
 *
 * \return  The value of the referenced shared variable before the
 *          operation.
 */
template<typename T>
typename std::enable_if<
  std::is_integral<T>::value,
  T>::type
fetch_sub(
  const dash::GlobRef<dash::Atomic<T>> & ref,
  /// Value to be subtracted from global atomic variable.
  const T & value)
{
  return dash::AtomicAddress<T>(ref).fetch_and_sub(value);
}
  
} // namespace atomic
} // namespace dash

#endif // DASH__ATOMIC_OPERATION_H_

