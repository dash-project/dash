#ifndef DASH__ATOMIC_H__INCLUDED
#define DASH__ATOMIC_H__INCLUDED

#include <dash/Meta.h>

#include <type_traits>
#include <sstream>


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
template <typename T>
class Atomic
{
  static_assert(
    dash::is_atomic_compatible<T>::value,
    "Type not supported for atomic operations");

private:
  T _value;
  typedef Atomic<T> self_t;

public:
  typedef T value_type;

  constexpr Atomic()                                 = default;
  constexpr Atomic(const Atomic<T> & other)          = default;
  self_t & operator=(const self_t & other)           = default;

  /**
   * Initializes the underlying value with desired.
   * The initialization is not atomic
   */
  constexpr Atomic(T value)
  : _value(value) { }

  /**
   * Disabled assignment as this violates the atomic semantics
   *
   * \todo
   * Assignment semantics are not well-defined:
   * - Constructor Atomic(T)  is default-defined
   * - Assignment  Atomic=(T) is deleted
   */
  T operator=(T value) = delete;

  /**
   * As \c Atomic is implemented as phantom type,
   * the value has to be queried using the \c dash::GlobRef
   */
  operator T()         = delete;

  constexpr bool operator==(const self_t & other) const {
    return _value == other._value;
  }

  constexpr bool operator!=(const self_t & other) const {
    return !(*this == other);
  }

  template<typename T_>
  friend std::ostream & operator<<(
    std::ostream     & os,
    const Atomic<T_> & at);

}; // class Atomic

template<typename T>
std::ostream & operator<<(
  std::ostream    & os,
  const Atomic<T> & at)
{
  std::ostringstream ss;
  ss << dash::typestr(at) << "<phantom>";
  return operator<<(os, ss.str());
}

} // namespace dash

#include <dash/atomic/Type_traits.h>
#include <dash/atomic/GlobAtomicRef.h>
#include <dash/atomic/GlobAtomicAsyncRef.h>
#include <dash/atomic/Operation.h>

#endif // DASH__ATOMIC_H__INCLUDED

