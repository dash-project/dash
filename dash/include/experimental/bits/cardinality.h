#ifndef STD_EXPERIMENTAL_BITS_CARDINALITY_H
#define STD_EXPERIMENTAL_BITS_CARDINALITY_H

#include <future>
#include <experimental/bits/is_oneway_executor.h>
#include <experimental/bits/is_twoway_executor.h>
#include <experimental/bits/is_bulk_oneway_executor.h>
#include <experimental/bits/is_bulk_twoway_executor.h>
#include <experimental/bits/require_member_result.h>
#include <experimental/bits/query_member_result.h>

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {
namespace cardinality_impl {

template<class InnerExecutor>
class bulk_adapter
{
  InnerExecutor inner_ex_;
  template <class T> static auto inner_declval() -> decltype(std::declval<InnerExecutor>());
  template <class, class T> struct dependent_type { using type = T; };

public:
  bulk_adapter(InnerExecutor ex) : inner_ex_(std::move(ex)) {}

  bulk_adapter require(const single_t&) const & { return *this; }
  bulk_adapter require(const single_t&) && { return std::move(*this); }
  bulk_adapter require(const bulk_t&) const & { return *this; }
  bulk_adapter require(const bulk_t&) && { return std::move(*this); }

  template<class... T> auto require(T&&... t) const &
    -> bulk_adapter<typename require_member_result_impl::eval<InnerExecutor, T...>::type>
      { return { inner_ex_.require(std::forward<T>(t)...) }; }
  template<class... T> auto require(T&&... t) &&
    -> bulk_adapter<typename require_member_result_impl::eval<InnerExecutor&&, T...>::type>
      { return { std::move(inner_ex_).require(std::forward<T>(t)...) }; }

  template<class... T> auto query(T&&... t) const
    -> typename query_member_result_impl::eval<InnerExecutor, T...>::type
      { return inner_ex_.query(std::forward<T>(t)...); }

  friend bool operator==(const bulk_adapter& a, const bulk_adapter& b) noexcept
  {
    return a.inner_ex_ == b.inner_ex_;
  }

  friend bool operator!=(const bulk_adapter& a, const bulk_adapter& b) noexcept
  {
    return !(a == b);
  }

  template<class Function> void execute(Function f) const
  {
    inner_ex_.execute(std::move(f));
  }

  template<class Function>
  auto twoway_execute(Function f) const
    -> decltype(inner_declval<Function>().twoway_execute(std::move(f)))
  {
    return inner_ex_.twoway_execute(std::move(f));
  }

  template<class Function, class SharedFactory>
  void bulk_execute(Function f, std::size_t n, SharedFactory sf) const
  {
    auto shared_state = std::make_shared<decltype(sf())>(sf());
    for (std::size_t i = 0; i < n; ++i)
    {
      inner_ex_.execute(
          [f = std::move(f), i, shared_state]() mutable
          {
            f(i, *shared_state);
          });
    }
  }

  template<class Function, class ResultFactory, class SharedFactory>
  auto bulk_twoway_execute(Function f, std::size_t n, ResultFactory rf, SharedFactory sf) const -> typename dependent_type<
      decltype(inner_declval<Function>().twoway_execute(std::move(rf))),
      typename std::enable_if<is_same<decltype(rf()), void>::value, future<void>>::type>::type
  {
    auto shared_state = std::make_shared<
        std::tuple<
          std::atomic<std::size_t>, // Number of incomplete functions.
          decltype(sf()), // Underlying shared state.
          std::atomic<std::size_t>, // Number of exceptions raised.
          std::exception_ptr, // First exception raised.
          promise<void> // Promise to receive result
        >>(n, sf(), 0, nullptr, promise<void>());
    future<void> future = std::get<4>(*shared_state).get_future();
    this->bulk_execute(
        [f = std::move(f)](auto i, auto& s) mutable
        {
          try
          {
            f(i, std::get<1>(*s));
          }
          catch (...)
          {
            if (std::get<2>(*s)++ == 0)
              std::get<3>(*s) = std::current_exception();
          }
          if (--std::get<0>(*s) == 0)
          {
            if (std::get<3>(*s))
              std::get<4>(*s).set_exception(std::get<3>(*s));
            else
              std::get<4>(*s).set_value();
          }
        }, n, [shared_state]{ return shared_state; });
    return future;
  }

  template<class Function, class ResultFactory, class SharedFactory>
  auto bulk_twoway_execute(Function f, std::size_t n, ResultFactory rf, SharedFactory sf) const -> typename dependent_type<
      decltype(inner_declval<Function>().twoway_execute(std::move(rf))),
      typename std::enable_if<!is_same<decltype(rf()), void>::value, future<decltype(rf())>>::type>::type
  {
    auto shared_state = std::make_shared<
        std::tuple<
          std::atomic<std::size_t>, // Number of incomplete functions.
          decltype(rf()), // Result.
          decltype(sf()), // Underlying shared state.
          std::atomic<std::size_t>, // Number of exceptions raised.
          std::exception_ptr, // First exception raised.
          promise<decltype(rf())> // Promise to receive result
        >>(n, rf(), sf(), 0, nullptr, promise<decltype(rf())>());
    future<decltype(rf())> future = std::get<5>(*shared_state).get_future();
    this->bulk_execute(
        [f = std::move(f)](auto i, auto& s) mutable
        {
          try
          {
            f(i, std::get<1>(*s), std::get<2>(*s));
          }
          catch (...)
          {
            if (std::get<3>(*s)++ == 0)
              std::get<4>(*s) = std::current_exception();
          }
          if (--std::get<0>(*s) == 0)
          {
            if (std::get<4>(*s))
              std::get<5>(*s).set_exception(std::get<4>(*s));
            else
              std::get<5>(*s).set_value(std::move(std::get<1>(*s)));
          }
        }, n, [shared_state]{ return shared_state; });
    return future;
  }
};

} // namespace cardinality_impl

struct single_t
{
  static constexpr bool is_requirable = true;
  static constexpr bool is_preferable = false;

  using polymorphic_query_result_type = bool;

  template<class Executor>
    static constexpr bool static_query_v
      = is_oneway_executor<Executor>::value
        || is_twoway_executor<Executor>::value;

  static constexpr bool value() { return true; }
};

constexpr single_t single;

struct bulk_t
{
  static constexpr bool is_requirable = true;
  static constexpr bool is_preferable = false;

  using polymorphic_query_result_type = bool;

  template<class Executor>
    static constexpr bool static_query_v
      = is_bulk_oneway_executor<Executor>::value
       || is_bulk_twoway_executor<Executor>::value;

  static constexpr bool value() { return true; }

  // Default require for bulk adapts single executors.

  template<class Executor>
    friend typename std::enable_if<is_oneway_executor<Executor>::value
      && !(is_bulk_oneway_executor<Executor>::value || is_bulk_twoway_executor<Executor>::value),
        cardinality_impl::bulk_adapter<Executor>>::type
          require(Executor ex, bulk_t) { return cardinality_impl::bulk_adapter<Executor>(std::move(ex)); }
};

constexpr bulk_t bulk;

} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_CARDINALITY_H
