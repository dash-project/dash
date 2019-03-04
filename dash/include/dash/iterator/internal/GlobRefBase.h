#ifndef DASH__ITERATOR__INTERNAL__GLOBREF_BASE_H__INCLUDED
#define DASH__ITERATOR__INTERNAL__GLOBREF_BASE_H__INCLUDED

#include <type_traits>

namespace dash {
namespace internal {

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

/*
 * A type is implicitly convertible to the other if
 * we can
 */
template<class LHS, class RHS>
using is_implicitly_convertible = std::is_convertible<
        typename std::add_lvalue_reference<RHS>::type,
        typename std::add_lvalue_reference<LHS>::type>;

template <class LHS, class RHS>
using enable_implicit_copy_ctor = null_v<
    typename std::enable_if<
      is_implicitly_convertible<LHS, RHS>::value,
      LHS>::type>;


template <class LHS, class RHS>
using enable_explicit_copy_ctor = null_v<
    typename std::enable_if<
      // 1) not implicitly convertible
      !is_implicitly_convertible<LHS, RHS>::value &&
      // 2.1) follows the rules of const correctness
      (std::is_constructible<
        typename std::add_lvalue_reference<LHS>::type, RHS>
        ::value ||
      // 2.2) RHS is base of LHS and RHS must not be polymorphic...
      std::is_base_of<RHS, LHS>::value || !std::is_polymorphic<RHS>::value),
      LHS>::type>;

// clang-format on

}  // namespace internal
}  // namespace dash
#endif
