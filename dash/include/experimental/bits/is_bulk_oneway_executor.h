#ifndef STD_EXPERIMENTAL_BITS_IS_BULK_ONEWAY_EXECUTOR_H
#define STD_EXPERIMENTAL_BITS_IS_BULK_ONEWAY_EXECUTOR_H

#include <experimental/bits/is_executor.h>

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {
namespace is_bulk_oneway_executor_impl {

template<class...>
struct type_check
{
  typedef void type;
};

struct shared_state {};

struct shared_factory
{
  shared_state operator()() const { return {}; }
};

struct bulk_function
{
  void operator()(std::size_t, shared_state&) {}
};

template<class T, class = void>
struct eval : std::false_type {};

template<class T>
struct eval<T,
  typename type_check<
    typename std::enable_if<std::is_same<void, decltype(std::declval<const T&>().bulk_execute(
          std::declval<bulk_function>(), 1, std::declval<shared_factory>()))>::value>::type,
    typename std::enable_if<std::is_same<void, decltype(std::declval<const T&>().bulk_execute(
          std::declval<bulk_function&>(), 1, std::declval<shared_factory>()))>::value>::type,
    typename std::enable_if<std::is_same<void, decltype(std::declval<const T&>().bulk_execute(
          std::declval<const bulk_function&>(), 1, std::declval<shared_factory>()))>::value>::type,
    typename std::enable_if<std::is_same<void, decltype(std::declval<const T&>().bulk_execute(
          std::declval<bulk_function&&>(), 1, std::declval<shared_factory>()))>::value>::type
	>::type> : is_executor_impl::eval<T> {};

} // namespace is_bulk_oneway_executor_impl

template<class Executor>
struct is_bulk_oneway_executor : is_bulk_oneway_executor_impl::eval<Executor> {};

} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_IS_BULK_ONEWAY_EXECUTOR_H
