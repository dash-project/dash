#ifndef STD_EXPERIMENTAL_BITS_BULK_EXECUTION_H
#define STD_EXPERIMENTAL_BITS_BULK_EXECUTION_H

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {
namespace bulk_execution_impl {

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

} // namespace bulk_execution_impl

struct bulk_sequenced_execution_t : bulk_execution_impl::property_base<bulk_sequenced_execution_t> {};
struct bulk_parallel_execution_t : bulk_execution_impl::property_base<bulk_parallel_execution_t> {};
struct bulk_unsequenced_execution_t : bulk_execution_impl::property_base<bulk_unsequenced_execution_t> {};

constexpr bulk_sequenced_execution_t bulk_sequenced_execution;
constexpr bulk_parallel_execution_t bulk_parallel_execution;
constexpr bulk_unsequenced_execution_t bulk_unsequenced_execution;

} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_BULK_EXECUTION_H
