#ifndef STD_EXPERIMENTAL_BITS_EXECUTOR_SHAPE_H
#define STD_EXPERIMENTAL_BITS_EXECUTOR_SHAPE_H

#include <cstddef>

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {
namespace executor_shape_impl {

template<class>
struct type_check
{
  typedef void type;
};

template<class Executor, class = void>
struct eval
{
  using type = std::size_t;
};

template<class Executor>
struct eval<Executor, typename type_check<typename Executor::shape_type>::type>
{
  using type = typename decltype(execution::require(std::declval<const Executor&>(), execution::bulk))::shape_type;
};

} // namespace executor_shape_impl

template<class Executor>
struct executor_shape : executor_shape_impl::eval<Executor> {};

} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_EXECUTOR_SHAPE_H
