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

template <class LHS, class RHS>
using common_condition = std::is_same<
    typename std::remove_cv<LHS>::type,
    typename std::remove_cv<RHS>::type>;

template <class LHS, class RHS>
using enable_explicit_copy_ctor = null_v<
    typename std::enable_if<
        common_condition<LHS, RHS>::value &&
        !std::is_const<LHS>::value &&
        std::is_const<RHS>::value,
      LHS>::type>;

template <class LHS, class RHS>
using enable_implicit_copy_ctor = null_v<
    typename std::enable_if<
        std::is_same<LHS, RHS>::value ||
        (common_condition<LHS, RHS>::value &&
        std::is_const<LHS>::value),
      LHS>::type>;

// clang-format on

}  // namespace internal
}  // namespace dash
#endif
