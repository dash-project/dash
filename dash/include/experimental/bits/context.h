#ifndef STD_EXPERIMENTAL_BITS_CONTEXT_H
#define STD_EXPERIMENTAL_BITS_CONTEXT_H

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {
namespace context_impl {

template<class Derived>
struct property_base
{
  static constexpr bool is_requirable = false;
  static constexpr bool is_preferable = false;

  //using polymorphic_query_result_type = ?;

  template<class Executor, class Type = decltype(Executor::query(*static_cast<Derived*>(0)))>
    static constexpr Type static_query_v = Executor::query(Derived());
};

} // namespace context_impl

struct context_t : context_impl::property_base<context_t> {};

constexpr context_t context;

} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_CONTEXT_H
