#ifndef STD_EXPERIMENTAL_BITS_EXECUTOR_H
#define STD_EXPERIMENTAL_BITS_EXECUTOR_H

#include <atomic>
#include <cassert>
#include <experimental/bits/bad_executor.h>
#include <experimental/future>
#include <memory>
#include <utility>

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {
namespace executor_impl {

template<class... SupportableProperties>
struct property_list;

template<>
struct property_list<> {};

template<class Head, class... Tail>
struct property_list<Head, Tail...> {};

template<class Property, class... SupportableProperties>
struct contains_exact_property;

template<class Property, class Head, class... Tail>
struct contains_exact_property<Property, Head, Tail...>
  : std::conditional<std::is_same<Property, Head>::value, std::true_type, contains_exact_property<Property, Tail...>>::type {};

template<class Property>
struct contains_exact_property<Property> : std::false_type {};

template<class Property, class... SupportableProperties>
static constexpr bool contains_exact_property_v = contains_exact_property<Property, SupportableProperties...>::value;

template<class Property, class... SupportableProperties>
struct contains_convertible_property;

template<class Property, class Head, class... Tail>
struct contains_convertible_property<Property, Head, Tail...>
  : std::conditional<std::is_convertible<Property, Head>::value, std::true_type, contains_convertible_property<Property, Tail...>>::type {};

template<class Property>
struct contains_convertible_property<Property> : std::false_type {};

template<class Property, class... SupportableProperties>
static constexpr bool contains_convertible_property_v = contains_convertible_property<Property, SupportableProperties...>::value;

template<class Property, class... SupportableProperties>
struct find_convertible_property;

template<class Property, class Head, class... Tail>
struct find_convertible_property<Property, Head, Tail...>
  : std::conditional<std::is_convertible<Property, Head>::value, std::decay<Head>, find_convertible_property<Property, Tail...>>::type {};

template<class Property>
struct find_convertible_property<Property> {};

template<class Property, class... SupportableProperties>
using find_convertible_property_t = typename find_convertible_property<Property, SupportableProperties...>::type;

template<class PropertyList, class... SupportableProperties>
struct contains_exact_property_list;

template<class Head, class... Tail, class... SupportableProperties>
struct contains_exact_property_list<property_list<Head, Tail...>, SupportableProperties...>
  : std::integral_constant<bool, contains_exact_property_v<Head, SupportableProperties...>
      && contains_exact_property_list<property_list<Tail...>, SupportableProperties...>::value> {};

template<class... SupportableProperties>
struct contains_exact_property_list<property_list<>, SupportableProperties...> : std::true_type {};

template<class PropertyList, class... SupportableProperties>
static constexpr bool contains_exact_property_list_v = contains_exact_property_list<PropertyList, SupportableProperties...>::value;

struct identity_property
{
  static constexpr bool is_requirable = true;
  static constexpr bool is_preferable = true;
  template<class Executor> static constexpr bool static_query_v = true;
  static constexpr bool value() { return true; }
};

template<class Property, class... SupportableProperties>
using conditional_property_t = typename conditional<
  contains_exact_property_v<Property, SupportableProperties...>,
    Property, identity_property>::type;

template<class R, class... Args>
struct single_use_func_base
{
  virtual ~single_use_func_base() {}
  virtual R call(Args...) = 0;
};

template<class Function, class R, class... Args>
struct single_use_func : single_use_func_base<R, Args...>
{
  Function function_;

  explicit single_use_func(Function f) : function_(std::move(f)) {}

  virtual R call(Args... args)
  {
    std::unique_ptr<single_use_func> fp(this);
    Function f(std::move(function_));
    fp.reset();
    return f(std::forward<Args>(args)...);
  }
};

template<class R, class... Args>
struct multi_use_func_base
{
  virtual ~multi_use_func_base() {}
  virtual R call(Args...) = 0;
};

template<class Function, class R, class... Args>
struct multi_use_func : multi_use_func_base<R, Args...>
{
  Function function_;

  explicit multi_use_func(Function f) : function_(std::move(f)) {}

  virtual R call(Args... args)
  {
    return function_(std::forward<Args>(args)...);
  }
};

using oneway_func_base = single_use_func_base<void>;
template<class Function> using oneway_func = single_use_func<Function, void>;

using shared_factory_base = multi_use_func_base<std::shared_ptr<void>>;
template<class SharedFactory> using shared_factory = multi_use_func<SharedFactory, std::shared_ptr<void>>;

using bulk_func_base = multi_use_func_base<void, std::size_t, std::shared_ptr<void>&>;
template<class Function> using bulk_func = multi_use_func<Function, void, std::size_t, std::shared_ptr<void>&>;

using twoway_func_base = single_use_func_base<std::shared_ptr<void>>;
template<class Function> using twoway_func = single_use_func<Function, std::shared_ptr<void>>;

using twoway_then_func_base = single_use_func_base<void, std::shared_ptr<void>, std::exception_ptr>;
template<class Function> using twoway_then_func = single_use_func<Function, void, std::shared_ptr<void>, std::exception_ptr>;

struct impl_base
{
  virtual ~impl_base() {}
  virtual impl_base* clone() const noexcept = 0;
  virtual void destroy() noexcept = 0;
  virtual void execute(std::unique_ptr<oneway_func_base> f) = 0;
  virtual void twoway_execute(std::unique_ptr<twoway_func_base> f, std::unique_ptr<twoway_then_func_base> then) = 0;
  virtual void bulk_execute(std::unique_ptr<bulk_func_base> f, std::size_t n, std::shared_ptr<shared_factory_base> sf) = 0;
  virtual const type_info& target_type() const = 0;
  virtual void* target() = 0;
  virtual const void* target() const = 0;
  virtual bool equals(const impl_base* e) const noexcept = 0;
  virtual impl_base* require(const type_info&, const void* p) const = 0;
  virtual impl_base* prefer(const type_info&, const void* p) const = 0;
  virtual void* query(const type_info&, const void* p) const = 0;
};

template<class Executor, class... SupportableProperties>
struct impl : impl_base
{
  Executor executor_;
  std::atomic<std::size_t> ref_count_{1};

  explicit impl(Executor ex) : executor_(std::move(ex)) {}

  virtual impl_base* clone() const noexcept
  {
    impl* e = const_cast<impl*>(this);
    ++e->ref_count_;
    return e;
  }

  virtual void destroy() noexcept
  {
    if (--ref_count_ == 0)
      delete this;
  }

  template<class T> auto execute_helper(T&& f)
    -> typename std::enable_if<std::is_same<T, T>::value
      && contains_exact_property_v<oneway_t, SupportableProperties...>
        && contains_exact_property_v<single_t, SupportableProperties...>>::type
  {
    executor_.execute([f = std::move(f)]() mutable { f.release()->call(); });
  }

  template<class T> auto execute_helper(T&&)
    -> typename std::enable_if<!std::is_same<T, T>::value
      || !contains_exact_property_v<oneway_t, SupportableProperties...>
        || !contains_exact_property_v<single_t, SupportableProperties...>>::type
  {
    assert(0);
  }

  virtual void execute(std::unique_ptr<oneway_func_base> f)
  {
    this->execute_helper<>(f);
  }

  template<class T, class U> auto twoway_execute_helper(T&& f, U&& then)
    -> typename std::enable_if<std::is_same<T, T>::value
      && contains_exact_property_v<twoway_t, SupportableProperties...>
        && contains_exact_property_v<single_t, SupportableProperties...>>::type
  {
    executor_.twoway_execute(
        [f = std::move(f)]() mutable
        {
          return f.release()->call();
        })
    .then(
        [then = std::move(then)](auto fut) mutable
        {
          std::shared_ptr<void> result;
          std::exception_ptr excep;
          try { result = fut.get(); } catch (...) { excep = std::current_exception(); }
          then.release()->call(result, excep);
        });
  }

  template<class T, class U> auto twoway_execute_helper(T&&, U&&)
    -> typename std::enable_if<!std::is_same<T, T>::value
      || !contains_exact_property_v<twoway_t, SupportableProperties...>
        || !contains_exact_property_v<single_t, SupportableProperties...>>::type
  {
    assert(0);
  }

  virtual void twoway_execute(std::unique_ptr<twoway_func_base> f, std::unique_ptr<twoway_then_func_base> then)
  {
    this->twoway_execute_helper<>(f, then);
  }

  template<class T, class U, class V>
  auto bulk_execute_helper(T&& f, U&& n, V&& sf)
    -> typename std::enable_if<std::is_same<T, T>::value
      && contains_exact_property_v<oneway_t, SupportableProperties...>
        && contains_exact_property_v<bulk_t, SupportableProperties...>>::type
  {
    executor_.bulk_execute(
        [f = std::move(f)](std::size_t i, auto s) mutable { f->call(i, s); }, n,
        [sf = std::move(sf)]() mutable { return sf->call(); });
  }

  template<class T, class U, class V>
  auto bulk_execute_helper(T&&, U&&, V&&)
    -> typename std::enable_if<!std::is_same<T, T>::value
      || !contains_exact_property_v<oneway_t, SupportableProperties...>
        || !contains_exact_property_v<bulk_t, SupportableProperties...>>::type
  {
    assert(0);
  }

  virtual void bulk_execute(std::unique_ptr<bulk_func_base> f, std::size_t n, std::shared_ptr<shared_factory_base> sf)
  {
    this->bulk_execute_helper<>(f, n, sf);
  }

  virtual const type_info& target_type() const
  {
    return typeid(executor_);
  }

  virtual void* target()
  {
    return &executor_;
  }

  virtual const void* target() const
  {
    return &executor_;
  }

  virtual bool equals(const impl_base* e) const noexcept
  {
    if (this == e)
      return true;
    if (target_type() != e->target_type())
      return false;
    return executor_ == *static_cast<const Executor*>(e->target());
  }

  impl_base* require_helper(property_list<>, const type_info&, const void*) const
  {
    assert(0);
    return nullptr;
  }

  template<class Head, class... Tail>
  impl_base* require_helper(property_list<Head, Tail...>, const type_info& t, const void* p, typename std::enable_if<Head::is_requirable>::type* = 0) const
  {
    if (t == typeid(Head))
    {
      using executor_type = decltype(execution::require(executor_, *static_cast<const Head*>(p)));
      return new impl<executor_type, SupportableProperties...>(execution::require(executor_, *static_cast<const Head*>(p)));
    }
    return require_helper(property_list<Tail...>{}, t, p);
  }

  template<class Head, class... Tail>
  impl_base* require_helper(property_list<Head, Tail...>, const type_info& t, const void* p, typename std::enable_if<!Head::is_requirable>::type* = 0) const
  {
    return require_helper(property_list<Tail...>{}, t, p);
  }

  virtual impl_base* require(const type_info& t, const void* p) const
  {
    return this->require_helper(property_list<SupportableProperties...>{}, t, p);
  }

  impl_base* prefer_helper(property_list<>, const type_info&, const void*) const
  {
    return clone();
  }

  template<class Head, class... Tail>
  impl_base* prefer_helper(property_list<Head, Tail...>, const type_info& t, const void* p, typename std::enable_if<Head::is_preferable>::type* = 0) const
  {
    if (t == typeid(Head))
    {
      using executor_type = decltype(execution::prefer(executor_, *static_cast<const Head*>(p)));
      return new impl<executor_type, SupportableProperties...>(execution::prefer(executor_, *static_cast<const Head*>(p)));
    }
    return prefer_helper(property_list<Tail...>{}, t, p);
  }

  template<class Head, class... Tail>
  impl_base* prefer_helper(property_list<Head, Tail...>, const type_info& t, const void* p, typename std::enable_if<!Head::is_preferable>::type* = 0) const
  {
    return prefer_helper(property_list<Tail...>{}, t, p);
  }

  virtual impl_base* prefer(const type_info& t, const void* p) const
  {
    return this->prefer_helper(property_list<SupportableProperties...>{}, t, p);
  }

  void* query_helper(property_list<>, const type_info&, const void*) const
  {
    return nullptr;
  }

  template<class Head, class... Tail>
  void* query_helper(property_list<Head, Tail...>, const type_info& t, const void* p, typename std::enable_if<can_query_v<Executor, Head>>::type* = 0) const
  {
    if (t == typeid(Head))
      return new typename Head::polymorphic_query_result_type(execution::query(executor_, *static_cast<const Head*>(p)));
    return query_helper(property_list<Tail...>{}, t, p);
  }

  template<class Head, class... Tail>
  void* query_helper(property_list<Head, Tail...>, const type_info& t, const void* p, typename std::enable_if<!can_query_v<Executor, Head>>::type* = 0) const
  {
    return query_helper(property_list<Tail...>{}, t, p);
  }

  virtual void* query(const type_info& t, const void* p) const
  {
    return this->query_helper(property_list<SupportableProperties...>{}, t, p);
  }
};

} // namespace executor_impl

template<class... SupportableProperties>
class executor
{
public:
  // construct / copy / destroy:

  executor() noexcept
    : impl_(nullptr)
  {
  }

  executor(std::nullptr_t) noexcept
    : impl_(nullptr)
  {
  }

  executor(const executor& e) noexcept
    : impl_(e.impl_ ? e.impl_->clone() : nullptr)
  {
  }

  executor(executor&& e) noexcept
    : impl_(e.impl_)
  {
    e.impl_ = nullptr;
  }

  template<class Executor> executor(Executor e)
  {
    auto e2 = execution::require(std::move(e),
        executor_impl::conditional_property_t<single_t, SupportableProperties...>{},
        executor_impl::conditional_property_t<bulk_t, SupportableProperties...>{},
        executor_impl::conditional_property_t<oneway_t, SupportableProperties...>{},
        executor_impl::conditional_property_t<twoway_t, SupportableProperties...>{});
    impl_ = new executor_impl::impl<decltype(e2), SupportableProperties...>(std::move(e2));
  }

  template<class... OtherSupportableProperties>
  executor(executor<OtherSupportableProperties...> e,
      typename std::enable_if<executor_impl::contains_exact_property_list_v<
        executor_impl::property_list<SupportableProperties...>,
          OtherSupportableProperties...>>::type* = 0)
    : impl_(e.impl_ ? e.impl_->clone() : nullptr)
  {
  }

  template<class... OtherSupportableProperties>
  executor(executor<OtherSupportableProperties...> e,
      typename std::enable_if<!executor_impl::contains_exact_property_list_v<
        executor_impl::property_list<SupportableProperties...>,
          OtherSupportableProperties...>>::type* = 0) = delete;

  executor& operator=(const executor& e) noexcept
  {
    if (impl_) impl_->destroy();
    impl_ = e.impl_ ? e.impl_->clone() : nullptr;
    return *this;
  }

  executor& operator=(executor&& e) noexcept
  {
    if (this != &e)
    {
      if (impl_) impl_->destroy();
      impl_ = e.impl_;
      e.impl_ = nullptr;
    }
    return *this;
  }

  executor& operator=(nullptr_t) noexcept
  {
    if (impl_) impl_->destroy();
    impl_ = nullptr;
    return *this;
  }

  template<class Executor> executor& operator=(Executor e)
  {
    return operator=(executor(std::move(e)));
  }

  ~executor()
  {
    if (impl_) impl_->destroy();
  }

  // polymorphic executor modifiers:

  void swap(executor& other) noexcept
  {
    std::swap(impl_, other.impl_);
  }

  template<class Executor> void assign(Executor e)
  {
    operator=(executor(std::move(e)));
  }

  // executor operations:

  template<class Property,
    class = typename std::enable_if<
      executor_impl::find_convertible_property_t<Property, SupportableProperties...>::is_requirable>::type>
  executor require(const Property& p) const
  {
    executor_impl::find_convertible_property_t<Property, SupportableProperties...> p1(p);
    return impl_ ? impl_->require(typeid(p1), &p1) : throw bad_executor();
  }

  template<class Property,
    class = typename std::enable_if<
      executor_impl::find_convertible_property_t<Property, SupportableProperties...>::is_preferable>::type>
  friend executor prefer(const executor& e, const Property& p)
  {
    executor_impl::find_convertible_property_t<Property, SupportableProperties...> p1(p);
    return e.get_impl() ? e.get_impl()->prefer(typeid(p1), &p1) : throw bad_executor();
  }

  template<class Property>
  auto query(const Property& p) const
    -> typename executor_impl::find_convertible_property_t<Property, SupportableProperties...>::polymorphic_query_result_type
  {
    executor_impl::find_convertible_property_t<Property, SupportableProperties...> p1(p);
    using result_type = typename decltype(p1)::polymorphic_query_result_type;
    if (!impl_) throw bad_executor();
    std::unique_ptr<result_type> result(static_cast<result_type*>(impl_->query(typeid(p1), &p1)));
    return result ? *result : result_type();
  }

  template<class Function,
    class = typename std::enable_if<std::is_same<Function, Function>::value
      && executor_impl::contains_exact_property_v<oneway_t, SupportableProperties...>
        && executor_impl::contains_exact_property_v<single_t, SupportableProperties...>>::type>
  void execute(Function f) const
  {
    std::unique_ptr<executor_impl::oneway_func_base> fp(new executor_impl::oneway_func<Function>(std::move(f)));
    impl_ ? impl_->execute(std::move(fp)) : throw bad_executor();
  }

  template<class Function,
    class = typename std::enable_if<std::is_same<Function, Function>::value &&
      executor_impl::contains_exact_property_v<twoway_t, SupportableProperties...>
        && executor_impl::contains_exact_property_v<single_t, SupportableProperties...>>::type>
  auto twoway_execute(Function f) const
    -> typename std::enable_if<std::is_same<decltype(f()), void>::value, future<void>>::type
  {
    promise<void> prom;
    future<void> fut(prom.get_future());

    auto f_wrap = [f = std::move(f)]() mutable
    {
      f();
      return std::shared_ptr<void>();
    };

    auto then = [prom = std::move(prom)](std::shared_ptr<void>, std::exception_ptr excep) mutable
    {
      if (excep)
        prom.set_exception(excep);
      else
        prom.set_value();
    };

    std::unique_ptr<executor_impl::twoway_func_base> fp(new executor_impl::twoway_func<decltype(f_wrap)>(std::move(f_wrap)));
    std::unique_ptr<executor_impl::twoway_then_func_base> tp(new executor_impl::twoway_then_func<decltype(then)>(std::move(then)));
    impl_ ? impl_->twoway_execute(std::move(fp), std::move(tp)) : throw bad_executor();

    return fut;
  }

  template<class Function,
    class = typename std::enable_if<std::is_same<Function, Function>::value &&
      executor_impl::contains_exact_property_v<twoway_t, SupportableProperties...>
        && executor_impl::contains_exact_property_v<single_t, SupportableProperties...>>::type>
  auto twoway_execute(Function f) const
    -> typename std::enable_if<!std::is_same<decltype(f()), void>::value, future<decltype(f())>>::type
  {
    promise<decltype(f())> prom;
    future<decltype(f())> fut(prom.get_future());

    auto f_wrap = [f = std::move(f)]() mutable
    {
      return std::make_shared<decltype(f())>(f());
    };

    auto then = [prom = std::move(prom)](std::shared_ptr<void> result, std::exception_ptr excep) mutable
    {
      if (excep)
        prom.set_exception(excep);
      else
        prom.set_value(std::move(*std::static_pointer_cast<decltype(f())>(result)));
    };

    std::unique_ptr<executor_impl::twoway_func_base> fp(new executor_impl::twoway_func<decltype(f_wrap)>(std::move(f_wrap)));
    std::unique_ptr<executor_impl::twoway_then_func_base> tp(new executor_impl::twoway_then_func<decltype(then)>(std::move(then)));
    impl_ ? impl_->twoway_execute(std::move(fp), std::move(tp)) : throw bad_executor();

    return fut;
  }

  template<class Function, class SharedFactory,
    class = typename std::enable_if<std::is_same<Function, Function>::value &&
      executor_impl::contains_exact_property_v<oneway_t, SupportableProperties...>
        && executor_impl::contains_exact_property_v<bulk_t, SupportableProperties...>>::type>
  void bulk_execute(Function f, std::size_t n, SharedFactory sf) const
  {
    auto f_wrap = [f = std::move(f)](std::size_t i, std::shared_ptr<void>& ss) mutable
    {
      f(i, *std::static_pointer_cast<decltype(sf())>(ss));
    };

    auto sf_wrap = [sf = std::move(sf)]() mutable
    {
      return std::make_shared<decltype(sf())>(sf());
    };

    std::unique_ptr<executor_impl::bulk_func_base> fp(new executor_impl::bulk_func<decltype(f_wrap)>(std::move(f_wrap)));
    std::shared_ptr<executor_impl::shared_factory_base> sfp(new executor_impl::shared_factory<decltype(sf_wrap)>(std::move(sf_wrap)));
    impl_ ? impl_->bulk_execute(std::move(fp), n, std::move(sfp)) : throw bad_executor();
  }

#if 0 // TODO implement bulk two-way support.
  template<class Function, class ResultFactory, class SharedFactory>
    auto bulk_twoway_execute(Function f, std::size_t n, ResultFactory rf, SharedFactory sf) const -> future<decltype(rf())>;
#endif

  // polymorphic executor capacity:

  explicit operator bool() const noexcept
  {
    return !!impl_;
  }

  // polymorphic executor target access:

  const type_info& target_type() const noexcept
  {
    return impl_ ? impl_->target_type() : typeid(void);
  }

  template<class Executor> Executor* target() noexcept
  {
    return impl_ ? static_cast<Executor*>(impl_->target()) : nullptr;
  }

  template<class Executor> const Executor* target() const noexcept
  {
    return impl_ ? static_cast<Executor*>(impl_->target()) : nullptr;
  }

  // polymorphic executor comparisons:

  friend bool operator==(const executor& a, const executor& b) noexcept
  {
    if (!a.get_impl() && !b.get_impl())
      return true;
    if (a.get_impl() && b.get_impl())
      return a.get_impl()->equals(b.get_impl());
    return false;
  }

  friend bool operator==(const executor& e, nullptr_t) noexcept
  {
    return !e;
  }

  friend bool operator==(nullptr_t, const executor& e) noexcept
  {
    return !e;
  }

  friend bool operator!=(const executor& a, const executor& b) noexcept
  {
    return !(a == b);
  }

  friend bool operator!=(const executor& e, nullptr_t) noexcept
  {
    return !!e;
  }

  friend bool operator!=(nullptr_t, const executor& e) noexcept
  {
    return !!e;
  }

private:
  template<class...> friend class executor;
  executor(executor_impl::impl_base* i) noexcept : impl_(i) {}
  executor_impl::impl_base* impl_;
  const executor_impl::impl_base* get_impl() const { return impl_; }
};

// executor specialized algorithms:

template<class... SupportableProperties>
inline void swap(executor<SupportableProperties...>& a, executor<SupportableProperties...>& b) noexcept
{
  a.swap(b);
}

} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_EXECUTOR_H
