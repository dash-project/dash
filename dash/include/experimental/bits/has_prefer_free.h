#ifndef STD_EXPERIMENTAL_BITS_HAS_PREFER_FREE_H
#define STD_EXPERIMENTAL_BITS_HAS_PREFER_FREE_H

#include <type_traits>
#include <utility>

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {
namespace has_prefer_free_impl {

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
    prefer(std::declval<Executor>(), std::declval<Property>())
  )>::type> : std::true_type {};

} // namespace has_prefer_free_impl
} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_HAS_PREFER_FREE_H
