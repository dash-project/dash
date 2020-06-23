#ifndef INCLUDED_TYPE_TRAITS_DOT_H
#define INCLUDED_TYPE_TRAITS_DOT_H

/// see https://en.cppreference.com/w/cpp/feature_test for recommended feature
/// tests

#if defined(SPEC) && defined(__NEC__) && !defined(__cpp_lib_is_swappable)
#define __cpp_lib_is_swappable 201603
#endif

#if __cpp_lib_is_swappable < 201603
#include <type_traits>
/// C++17 is_nothrow_swappable as proposed in the following proposal:
/// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0185r1.html#Appendix
namespace std {
struct do_is_nothrow_swappable {
  template <class T>
  static auto test(int) -> std::integral_constant<
      bool,
      noexcept(swap(std::declval<T&>(), std::declval<T&>()))>;

  template <class>
  static std::false_type test(...);
};

template <typename T>
struct is_nothrow_swappable : decltype(do_is_nothrow_swappable::test<T>(0)) {
};
}  // namespace std
#endif
#endif
