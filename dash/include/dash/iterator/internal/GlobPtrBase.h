#ifndef DASH__ITERATOR__INTERNAL__GLOBPTR_BASE_H__INCLUDED
#define DASH__ITERATOR__INTERNAL__GLOBPTR_BASE_H__INCLUDED

#include <type_traits>

namespace dash {
namespace internal {

template <class LHS, class RHS>
struct is_pointer_assignable {
  static constexpr bool value = std::is_assignable<LHS &, RHS &>::value;
};

template <class T>
struct is_pointer_assignable<void, T> {
  static constexpr bool value = std::true_type::value;
};

template <class T>
struct is_pointer_assignable<const void, T> {
  static constexpr bool value = std::true_type::value;
};

template <class U>
struct is_pointer_assignable<U, void> {
  static constexpr bool value = !std::is_const<U>::value;
};

template <>
struct is_pointer_assignable<void, void> {
  static constexpr bool value = std::true_type::value;
};
}  // namespace internal
}  // namespace dash
#endif
