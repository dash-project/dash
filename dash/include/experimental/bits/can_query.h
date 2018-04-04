#ifndef STD_EXPERIMENTAL_BITS_CAN_QUERY_H
#define STD_EXPERIMENTAL_BITS_CAN_QUERY_H

#include <type_traits>
#include <tuple>
#include <utility>

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {
namespace can_query_impl {

template<class>
struct type_check
{
  typedef void type;
};

template<class Executor, class Properties, class = void>
struct eval : std::false_type {};

template<class Executor, class Property>
struct eval<Executor, std::tuple<Property>,
  typename type_check<decltype(
    ::std::experimental::executors_v1::execution::query(std::declval<Executor>(), std::declval<Property>())
  )>::type> : std::true_type {};

} // namespace can_query_impl

template<class Executor, class Property>
struct can_query : can_query_impl::eval<Executor, std::tuple<Property>> {};

} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_CAN_QUERY_H
