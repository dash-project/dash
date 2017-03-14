#ifndef DASH__ATOMIC__TYPE_TRAITS_INCLUDED
#define DASH__ATOMIC__TYPE_TRAITS_INCLUDED

namespace dash {

// forward decl atomic type for only importing traits without full blown atomic
// type support
template<typename T>
class Atomic;

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

/**
 * type traits for \c dash::Atomic
 *
 * returns the sub type of a \cdash::Atomic
 */
template<typename T>
struct remove_atomic {
  typedef T type;
};
template<typename T>
struct remove_atomic<dash::Atomic<T>> {
  typedef T type;
};

} // namespace dash

#endif // DASH__ATOMIC__TYPE_TRAITS_INCLUDED

