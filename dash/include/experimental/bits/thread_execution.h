#ifndef STD_EXPERIMENTAL_BITS_THREAD_EXECUTION_H
#define STD_EXPERIMENTAL_BITS_THREAD_EXECUTION_H

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {
namespace thread_execution_impl {

template<class Derived>
struct property_base
{
  static constexpr bool is_requirable = true;
  static constexpr bool is_preferable = true;

  using polymorphic_query_result_type = bool;

  template<class Executor, class = decltype(Executor::query(*static_cast<Derived*>(0)))>
    static constexpr bool static_query_v = Executor::query(Derived());

  static constexpr bool value() { return true; }
};

} // namespace thread_execution_impl

struct thread_execution_mapping_t : thread_execution_impl::property_base<thread_execution_mapping_t> {};
struct new_thread_execution_mapping_t : thread_execution_impl::property_base<new_thread_execution_mapping_t> {};

constexpr thread_execution_mapping_t thread_execution_mapping;
constexpr new_thread_execution_mapping_t new_thread_execution_mapping;

} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_THREAD_EXECUTION_H
