#ifndef STD_EXPERIMENTAL_BITS_OUTSTANDING_WORK_H
#define STD_EXPERIMENTAL_BITS_OUTSTANDING_WORK_H

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {
namespace outstanding_work_impl {

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

} // namespace outstanding_work_impl

struct outstanding_work_t : outstanding_work_impl::property_base<outstanding_work_t> {};
struct not_outstanding_work_t : outstanding_work_impl::property_base<not_outstanding_work_t> {};

constexpr outstanding_work_t outstanding_work;
constexpr not_outstanding_work_t not_outstanding_work;

} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_OUTSTANDING_WORK_H
