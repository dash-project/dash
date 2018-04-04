#ifndef STD_EXPERIMENTAL_BITS_DIRECTIONALITY_H
#define STD_EXPERIMENTAL_BITS_DIRECTIONALITY_H

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
namespace directionality_impl {

template<class InnerExecutor>
class twoway_adapter
{
  InnerExecutor inner_ex_;
  template <class T> static auto inner_declval() -> decltype(std::declval<InnerExecutor>());
  template <class, class T> struct dependent_type { using type = T; };

public:
  twoway_adapter(InnerExecutor ex) : inner_ex_(std::move(ex)) {}

  twoway_adapter require(const oneway_t&) const & { return *this; }
  twoway_adapter require(const oneway_t&) && { return std::move(*this); }
  twoway_adapter require(const twoway_t&) const & { return *this; }
  twoway_adapter require(const twoway_t&) && { return std::move(*this); }

  template<class... T> auto require(T&&... t) const &
    -> twoway_adapter<typename require_member_result_impl::eval<InnerExecutor, T...>::type>
      { return { inner_ex_.require(std::forward<T>(t)...) }; }
  template<class... T> auto require(T&&... t) &&
    -> twoway_adapter<typename require_member_result_impl::eval<InnerExecutor&&, T...>::type>
      { return { std::move(inner_ex_).require(std::forward<T>(t)...) }; }

  template<class... T> auto query(T&&... t) const
    -> typename query_member_result_impl::eval<InnerExecutor, T...>::type
      { return inner_ex_.query(std::forward<T>(t)...); }

  friend bool operator==(const twoway_adapter& a, const twoway_adapter& b) noexcept
  {
    return a.inner_ex_ == b.inner_ex_;
  }

  friend bool operator!=(const twoway_adapter& a, const twoway_adapter& b) noexcept
  {
    return !(a == b);
  }

  template<class Function> auto execute(Function f) const
    -> decltype(inner_declval<Function>().execute(std::move(f)))
  {
    inner_ex_.execute(std::move(f));
  }

  template<class Function>
  auto twoway_execute(Function f) const -> typename dependent_type<
      decltype(inner_declval<Function>().execute(std::move(f))),
      future<decltype(f())>>::type
  {
    packaged_task<decltype(f())()> task(std::move(f));
    future<decltype(f())> future = task.get_future();
    inner_ex_.execute(std::move(task));
    return future;
  }

  template<class Function, class SharedFactory>
  auto bulk_execute(Function f, std::size_t n, SharedFactory sf) const
    -> decltype(inner_declval<Function>().bulk_execute(std::move(f), n, std::move(sf)))
  {
    inner_ex_.bulk_execute(std::move(f), n, std::move(sf));
  }

  template<class Function, class ResultFactory, class SharedFactory>
  auto bulk_twoway_execute(Function f, std::size_t n, ResultFactory rf, SharedFactory sf) const -> typename dependent_type<
      decltype(inner_declval<Function>().bulk_execute(std::move(f), n, std::move(sf))),
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
    inner_ex_.bulk_execute(
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
      decltype(inner_declval<Function>().bulk_execute(std::move(f), n, std::move(sf))),
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
    inner_ex_.bulk_execute(
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

} // namespace directionality_impl

struct oneway_t
{
  static constexpr bool is_requirable = true;
  static constexpr bool is_preferable = false;

  using polymorphic_query_result_type = bool;

  template<class Executor>
    static constexpr bool static_query_v
      = is_oneway_executor<Executor>::value
        || is_bulk_oneway_executor<Executor>::value;

  static constexpr bool value() { return true; }
};

constexpr oneway_t oneway;

struct twoway_t
{
  static constexpr bool is_requirable = true;
  static constexpr bool is_preferable = false;

  using polymorphic_query_result_type = bool;

  template<class Executor>
    static constexpr bool static_query_v
      = is_twoway_executor<Executor>::value
        || is_bulk_twoway_executor<Executor>::value;

  static constexpr bool value() { return true; }

  // Default require for twoway adapts oneway executors.

  template<class Executor>
    friend typename std::enable_if<
      (is_oneway_executor<Executor>::value || is_bulk_oneway_executor<Executor>::value)
        && !(is_twoway_executor<Executor>::value || is_bulk_twoway_executor<Executor>::value)
          && can_query<Executor, adaptable_blocking_t>::value,
            directionality_impl::twoway_adapter<Executor>>::type
              require(Executor ex, twoway_t) { return directionality_impl::twoway_adapter<Executor>(std::move(ex)); }
};

constexpr twoway_t twoway;

} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_DIRECTIONALITY_H
