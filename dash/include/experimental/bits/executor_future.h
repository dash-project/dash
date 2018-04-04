#ifndef STD_EXPERIMENTAL_BITS_EXECUTOR_FUTURE_H
#define STD_EXPERIMENTAL_BITS_EXECUTOR_FUTURE_H

#include <type_traits>

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {

template<class Executor, class T>
struct executor_future
{
  using type = decltype(execution::require(declval<const Executor&>(), execution::twoway).twoway_execute(declval<T(*)()>()));
};

} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_EXECUTOR_FUTURE_H
