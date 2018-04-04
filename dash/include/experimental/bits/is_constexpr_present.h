#ifndef STD_EXPERIMENTAL_BITS_IS_CONSTEXPR_PRESENT_H
#define STD_EXPERIMENTAL_BITS_IS_CONSTEXPR_PRESENT_H

#include <type_traits>

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {
namespace is_constexpr_present_impl {

template<class...>
struct type_check
{
  typedef void type;
};

template<class Executor, class Property, class = void>
struct eval : std::false_type {};

template<class Executor, class Property>
struct eval<Executor, Property,
  typename type_check<
    typename std::enable_if<Property::value() == Property::template static_query_v<Executor>>::type
	>::type> : std::true_type {};

} // namespace is_constexpr_present_impl
} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_IS_CONSTEXPR_PRESENT_H
