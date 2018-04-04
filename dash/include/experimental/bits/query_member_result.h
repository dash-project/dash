#ifndef STD_EXPERIMENTAL_BITS_QUERY_MEMBER_RESULT_H
#define STD_EXPERIMENTAL_BITS_QUERY_MEMBER_RESULT_H

#include <type_traits>
#include <utility>

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {
namespace query_member_result_impl {

template<class>
struct type_check
{
  typedef void type;
};

template<class Executor, class Property, class = void>
struct eval {};

template<class Executor, class Property>
struct eval<Executor, Property,
  typename type_check<decltype(
    std::declval<Executor>().query(std::declval<Property>())
  )>::type>
{
  typedef decltype(std::declval<Executor>().query(std::declval<Property>())) type;
};

} // namespace query_member_result_impl
} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_QUERY_MEMBER_RESULT_H
