#ifndef DASH__TYPE_TRAITS_H_
#define DASH__TYPE_TRAITS_H_

#include <type_traits>

///////////////////////////////////////////////////////////////////////////////
// Container Traits
///////////////////////////////////////////////////////////////////////////////

namespace dash {
/**
 * Type trait indicating whether the specified type is eligible for
 * elements of DASH containers.
 */
template <class T>
struct is_container_compatible
  : public std::integral_constant<
        bool,
        std::is_default_constructible<T>::value &&
            std::is_trivially_copyable<T>::value> {
};

}  // namespace dash

///////////////////////////////////////////////////////////////////////////////
// Atomic Traits
///////////////////////////////////////////////////////////////////////////////

/// Forward decls
namespace dash {

template <class T>
class Atomic;

}  // namespace dash

namespace std {
/**
 * type traits for \c dash::Atomic
 *
 * returns an atomic with a const subtype
 */
template <class T>
struct add_const<dash::Atomic<T>> {
  using type = dash::Atomic<typename std::add_const<T>::type>;
};

}  // namespace std

namespace dash {

namespace detail {
template <typename T>
struct remove_atomic_impl {
  using type = T;
};

template <typename T>
struct remove_atomic_impl<dash::Atomic<T>> {
  using type = T;
};

template <typename T>
struct is_atomic_impl : std::false_type {
};
template <typename T>
struct is_atomic_impl<dash::Atomic<T>> : std::true_type {
};

}  // namespace detail

/**
 * type traits for \c dash::Atomic
 *
 * returns the sub type of a \cdash::Atomic
 */
template <typename T>
struct remove_atomic {
  using type = typename detail::remove_atomic_impl<T>::type;
};

/**
 * type traits for \c dash::Atomic
 *
 * true if type is atomic
 * false otherwise
 */
template <typename T>
struct is_atomic : detail::is_atomic_impl<T> {
};

/**
 * Type trait indicating whether a type can be used for global atomic
 * operations.
 */
template <typename T>
struct is_atomic_compatible
  : public std::integral_constant<bool, std::is_arithmetic<T>::value> {
};

}  // namespace dash

#endif
