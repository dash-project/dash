#ifndef STD_EXPERIMENTAL_BITS_BLOCKING_H
#define STD_EXPERIMENTAL_BITS_BLOCKING_H

#include <experimental/bits/require_member_result.h>
#include <experimental/bits/query_member_result.h>
#include <future>
#include <type_traits>
#include <tuple>
#include <utility>

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {
namespace blocking_impl {

template<class InnerExecutor>
class adaptable_blocking_adapter
{
  InnerExecutor inner_ex_;
  template <class T> static auto inner_declval() -> decltype(std::declval<InnerExecutor>());

public:
  adaptable_blocking_adapter(InnerExecutor ex) : inner_ex_(std::move(ex)) {}

  adaptable_blocking_adapter require(const adaptable_blocking_t&) const & { return *this; }
  adaptable_blocking_adapter require(const adaptable_blocking_t&) && { return std::move(*this); }
  InnerExecutor require(const not_adaptable_blocking_t&) const & { return inner_ex_; }
  InnerExecutor require(const not_adaptable_blocking_t&) && { return std::move(inner_ex_); }

  template<class... T> auto require(T&&... t) const &
    -> adaptable_blocking_adapter<typename require_member_result_impl::eval<InnerExecutor, T...>::type>
      { return { inner_ex_.require(std::forward<T>(t)...) }; }
  template<class... T> auto require(T&&... t) &&
    -> adaptable_blocking_adapter<typename require_member_result_impl::eval<InnerExecutor&&, T...>::type>
      { return { std::move(inner_ex_).require(std::forward<T>(t)...) }; }

  constexpr bool query(const adaptable_blocking_t&) const { return true; }

  template<class... T> auto query(T&&... t) const
    -> typename query_member_result_impl::eval<InnerExecutor, T...>::type
      { return inner_ex_.query(std::forward<T>(t)...); }

  friend bool operator==(const adaptable_blocking_adapter& a, const adaptable_blocking_adapter& b) noexcept
  {
    return a.inner_ex_ == b.inner_ex_;
  }

  friend bool operator!=(const adaptable_blocking_adapter& a, const adaptable_blocking_adapter& b) noexcept
  {
    return !(a == b);
  }

  template<class Function> auto execute(Function f) const
    -> decltype(inner_declval<Function>().execute(std::move(f)))
  {
    return inner_ex_.execute(std::move(f));
  }

  template<class Function>
  auto twoway_execute(Function f) const
    -> decltype(inner_declval<Function>().twoway_execute(std::move(f)))
  {
    return inner_ex_.twoway_execute(std::move(f));
  }

  template<class Function, class SharedFactory>
  auto bulk_execute(Function f, std::size_t n, SharedFactory sf) const
    -> decltype(inner_declval<Function>().bulk_execute(std::move(f), n, std::move(sf)))
  {
    return inner_ex_.bulk_execute(std::move(f), n, std::move(sf));
  }

  template<class Function, class ResultFactory, class SharedFactory>
  auto bulk_twoway_execute(Function f, std::size_t n, ResultFactory rf, SharedFactory sf) const
    -> decltype(inner_declval<Function>().bulk_twoway_execute(std::move(f), n, std::move(rf), std::move(sf)))
  {
    return inner_ex_.bulk_twoway_execute(std::move(f), n, std::move(rf), std::move(sf));
  }
};

template<class InnerExecutor>
class always_blocking_adapter
{
  InnerExecutor inner_ex_;
  template <class T> static auto inner_declval() -> decltype(std::declval<InnerExecutor>());

public:
  always_blocking_adapter(InnerExecutor ex) : inner_ex_(std::move(ex)) {}

  always_blocking_adapter require(const always_blocking_t&) const & { return *this; }
  always_blocking_adapter require(const always_blocking_t&) && { return std::move(*this); }
  always_blocking_adapter require(const possibly_blocking_t&) const & { return *this; }
  always_blocking_adapter require(const possibly_blocking_t&) && { return std::move(*this); }

  template<class... T> auto require(T&&... t) const &
    -> always_blocking_adapter<typename require_member_result_impl::eval<InnerExecutor, T...>::type>
      { return { inner_ex_.require(std::forward<T>(t)...) }; }
  template<class... T> auto require(T&&... t) &&
    -> always_blocking_adapter<typename require_member_result_impl::eval<InnerExecutor&&, T...>::type>
      { return { std::move(inner_ex_).require(std::forward<T>(t)...) }; }

  constexpr bool query(const always_blocking_t&) const { return true; }

  template<class... T> auto query(T&&... t) const
    -> typename query_member_result_impl::eval<InnerExecutor, T...>::type
      { return inner_ex_.query(std::forward<T>(t)...); }

  friend bool operator==(const always_blocking_adapter& a, const always_blocking_adapter& b) noexcept
  {
    return a.inner_ex_ == b.inner_ex_;
  }

  friend bool operator!=(const always_blocking_adapter& a, const always_blocking_adapter& b) noexcept
  {
    return !(a == b);
  }

  template<class Function> auto execute(Function f) const
    -> decltype(inner_declval<Function>().execute(std::move(f)))
  {
    promise<void> promise;
    future<void> future = promise.get_future();
    inner_ex_.execute([f = std::move(f), p = std::move(promise)]() mutable { f(); });
    future.wait();
  }

  template<class Function>
  auto twoway_execute(Function f) const
    -> decltype(inner_declval<Function>().twoway_execute(std::move(f)))
  {
    auto future = inner_ex_.twoway_execute(std::move(f));
    future.wait();
    return future;
  }

  template<class Function, class SharedFactory>
  auto bulk_execute(Function f, std::size_t n, SharedFactory sf) const
    -> decltype(inner_declval<Function>().bulk_execute(std::move(f), n, std::move(sf)))
  {
    promise<void> promise;
    future<void> future = promise.get_future();
    inner_ex_.bulk_execute([f = std::move(f), p = std::move(promise)](auto i, auto& s) mutable { f(i, s); }, n, std::move(sf));
    future.wait();
  }

  template<class Function, class ResultFactory, class SharedFactory>
  auto bulk_twoway_execute(Function f, std::size_t n, ResultFactory rf, SharedFactory sf) const
    -> decltype(inner_declval<Function>().bulk_twoway_execute(std::move(f), n, std::move(rf), std::move(sf)))
  {
    auto future = inner_ex_.bulk_twoway_execute(std::move(f), n, std::move(rf), std::move(sf));
    future.wait();
    return future;
  }
};

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

template<class Executor, class Property>
constexpr auto possibly_blocking_static_query()
  -> typename std::enable_if<!can_query_v<Executor, never_blocking_t>
    && !can_query_v<Executor, always_blocking_t>, bool>::type
      { return true; }

template<class Executor, class Property>
constexpr auto possibly_blocking_static_query()
  -> typename std::enable_if<can_query_v<Executor, never_blocking_t>
    || can_query_v<Executor, always_blocking_t>,
      decltype(Executor::query(Property()))>::type
{
  return Executor::query(Property());
}

template<class Derived>
struct possibly_blocking_base
{
  static constexpr bool is_requirable = true;
  static constexpr bool is_preferable = true;

  using polymorphic_query_result_type = bool;

  template<class Executor, class = decltype(possibly_blocking_static_query<Executor, Derived>())>
    static constexpr bool static_query_v = possibly_blocking_static_query<Executor, Derived>();

  static constexpr bool value() { return true; }
};

} // namespace blocking_impl

struct never_blocking_t : blocking_impl::property_base<never_blocking_t> {};

struct possibly_blocking_t : blocking_impl::possibly_blocking_base<possibly_blocking_t> {};

struct always_blocking_t : blocking_impl::property_base<always_blocking_t>
{
  // Default require for always blocking adapts all executors. Only enabled if adaptable blocking is supported.

  template<class Executor>
    friend typename enable_if<can_query<Executor, adaptable_blocking_t>::value,
      blocking_impl::always_blocking_adapter<Executor>>::type
        require(Executor ex, const always_blocking_t&)
          { return blocking_impl::always_blocking_adapter<Executor>(std::move(ex)); }
};

struct adaptable_blocking_t : blocking_impl::property_base<adaptable_blocking_t>
{
  // Default adapter for adaptable blocking adds the adaptable blocking property.

  template<class Executor>
    friend blocking_impl::adaptable_blocking_adapter<Executor>
      require(Executor ex, const adaptable_blocking_t&)
        { return blocking_impl::adaptable_blocking_adapter<Executor>(std::move(ex)); }
};

struct not_adaptable_blocking_t : blocking_impl::property_base<not_adaptable_blocking_t> {};

constexpr never_blocking_t never_blocking;
constexpr possibly_blocking_t possibly_blocking;
constexpr always_blocking_t always_blocking;
constexpr adaptable_blocking_t adaptable_blocking;
constexpr not_adaptable_blocking_t not_adaptable_blocking;

} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_BLOCKING_H
