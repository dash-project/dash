#ifndef STD_EXPERIMENTAL_BITS_CONTINUATION_H
#define STD_EXPERIMENTAL_BITS_CONTINUATION_H

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {
namespace continuation_impl {

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

} // namespace continuation_impl

struct continuation_t : continuation_impl::property_base<continuation_t> {};
struct not_continuation_t : continuation_impl::property_base<not_continuation_t> {};

constexpr continuation_t continuation;
constexpr not_continuation_t not_continuation;

} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_CONTINUATION_H
