#ifndef STD_EXPERIMENTAL_BITS_HAS_QUERY_MEMBER_H
#define STD_EXPERIMENTAL_BITS_HAS_QUERY_MEMBER_H

#include <type_traits>
#include <utility>

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {
namespace has_query_member_impl {

template<class>
struct type_check
{
  typedef void type;
};

template<class Executor, class Property, class = void>
struct eval : std::false_type {};

template<class Executor, class Property>
struct eval<Executor, Property,
  typename type_check<decltype(
    std::declval<Executor>().query(std::declval<Property>())
  )>::type> : std::true_type {};

} // namespace has_query_member_impl
} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_HAS_QUERY_MEMBER_H
