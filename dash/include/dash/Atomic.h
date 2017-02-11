#ifndef DASH__ATOMIC_H__INCLUDED
#define DASH__ATOMIC_H__INCLUDED

namespace dash {

/**
 * Type wrapper to mark any trivial type atomic.
 *
 * If one unit writes to an atomic object while another unit reads from it,
 * the behavior is well-defined. The DASH version follows as closely as
 * possible the interface of \c std::atomic
 * However as data has to be transferred between
 * units using DART, the atomicity guarantees are set by the DART
 * implementation.
 *
 * \note \c Atomic objects have to be placed in a DASH container,
 *       and can only be accessed using \c GlobRef<dash::Atomic<T>> .
 *       Local accesses to atomic elements are not allowed.
 *
 *       \code
 *       dash::Array<dash::Atomic<int>> array(100);
 *       array.local[10].load()               // not allowed
 *       dash::atomic::load(array.local[10])  // not allowed
 *       dash::atomic::load(array.lbegin())   // not allowed
 *       \endcode
 * \endnote
 * 
 * \code
 *   dash::Array<dash::Atomic<int>> array(100);
 *   // supported as Atomic<value_t>(value_t T) is available
 *   dash::fill(array.begin(), array.end(), 0);
 *
 *   if(dash::myid() == 0){
 *     array[10].store(5);
 *   }
 *   dash::barrier();
 *   array[10].add(1);
 *   // postcondition:
 *   // array[10] == dash::size() + 5
 * \endcode
 */
template<typename T>
class Atomic {
public:
  typedef T value_type;

  Atomic(const Atomic<T> & other) = delete;

  /**
   * Initializes the underlying value with desired.
   * The initialization is not atomic
   */
  Atomic(T value)
  : _value(value) { }

  /// disabled as this violates the atomic semantics
  T operator= (T value) = delete;

  /**
   * As \c Atomic is implemented as phantom type,
   * the value has to be queried using the \c dash::GlobRef
   */
  operator T()          = delete;

private:
  T _value;
}; // class Atomic

/**
 * type traits for \c dash::Atomic
 *
 * true if type is atomic
 * false otherwise
 */
template<typename T>
struct is_atomic {
  static constexpr bool value = false;
};
template<typename T>
struct is_atomic<dash::Atomic<T>> {
  static constexpr bool value = true;
};

} // namespace dash

#include <dash/atomic/GlobAtomicRef.h>
#include <dash/atomic/Operation.h>

#endif // DASH__ATOMIC_H__INCLUDED

