#ifndef STD_EXPERIMENTAL_BITS_IS_EXECUTOR_H
#define STD_EXPERIMENTAL_BITS_IS_EXECUTOR_H

#include <type_traits>
#include <utility>

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {
namespace is_executor_impl {

template<class...>
struct type_check
{
  typedef void type;
};

template<class T, class = void>
struct eval : std::false_type {};

template<class T>
struct eval<T,
  typename type_check<
    typename std::enable_if<std::is_nothrow_copy_constructible<T>::value>::type,
    typename std::enable_if<std::is_nothrow_move_constructible<T>::value>::type,
    typename std::enable_if<noexcept(static_cast<bool>(std::declval<const T&>() == std::declval<const T&>()))>::type,
    typename std::enable_if<noexcept(static_cast<bool>(std::declval<const T&>() != std::declval<const T&>()))>::type
	>::type> : std::true_type {};

} // namespace is_executor_impl
} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_IS_EXECUTOR_H
