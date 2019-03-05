#ifndef DASH__ITERATOR__INTERNAL__GLOBREF_BASE_H__INCLUDED
#define DASH__ITERATOR__INTERNAL__GLOBREF_BASE_H__INCLUDED

#include <cstdint>
#include <memory>
#include <type_traits>

namespace dash {
namespace detail {

template <typename ReferenceT, typename TargetT>
struct add_const_from_type {
  using type = TargetT;
};

template <typename ReferenceT, typename TargetT>
struct add_const_from_type<const ReferenceT, TargetT> {
  using type = typename std::add_const<TargetT>::type;
};

template <class...>
struct null_v : std::integral_constant<int, 0> {
};

// clang-format off

template<class LHS, class RHS>
using is_implicitly_convertible = std::is_convertible<
        typename std::add_lvalue_reference<LHS>::type,
        typename std::add_lvalue_reference<RHS>::type>;

#if 0
template <class LHS, class RHS>
using is_explicitly_convertible = std::integral_constant<bool,
      // 1) not implicitly convertible
      !is_implicitly_convertible<LHS, RHS>::value &&
      // 2.1) It is constructible  or...
      (std::is_constructible<
        typename std::add_lvalue_reference<RHS>::type,
        typename std::add_lvalue_reference<LHS>::type>::value ||
      // 2.2) if RHS is a base of RHS and both classes are non-polymorphic
      (std::conditional<std::is_const<LHS>::value,
        std::is_const<RHS>,
        std::true_type>::type::value

       && std::is_base_of<LHS, RHS>::value
        && !std::is_polymorphic<LHS>::value
        && !std::is_polymorphic<RHS>::value))>;
#else
template <typename From, typename To>
struct is_explicitly_convertible {
  template <typename T>
  static void f(T);

  template <typename F, typename T>
  static constexpr auto test(int)
      -> decltype(f(static_cast<T>(std::declval<F>())), true)
  {
    return true;
  }

  template <typename F, typename T>
  static constexpr auto test(...) -> bool
  {
    return false;
  }

  static bool const value = test<From, To>(0);
};
#endif


template <class LHS, class RHS>
using enable_implicit_copy_ctor = null_v<
    typename std::enable_if<
      is_implicitly_convertible<LHS, RHS>::value,
      LHS>::type>;

template <class LHS, class RHS>
using enable_explicit_copy_ctor = null_v<
    typename std::enable_if<
      !is_implicitly_convertible<LHS, RHS>::value &&
      is_explicitly_convertible<
        typename std::add_lvalue_reference<LHS>::type,
        typename std::add_lvalue_reference<RHS>::type>::value,
      LHS>::type>;

// clang-format on

template <typename T1, typename T2>
inline std::size_t constexpr offset_of(T1 T2::*member) noexcept
{
  constexpr T2 dummy{};
  return std::size_t{std::uintptr_t(std::addressof(dummy.*member))} -
         std::size_t{std::uintptr_t(std::addressof(dummy))};
}

}  // namespace detail
}  // namespace dash
#endif
