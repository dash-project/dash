#ifndef STD_EXPERIMENTAL_BITS_FUTURE_H
#define STD_EXPERIMENTAL_BITS_FUTURE_H

#include <atomic>
#include <future>
#include <functional>
#include <memory>

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace future_impl {

template<class R, class... Args>
struct function_base
{
  virtual ~function_base() {}
  virtual R call(Args...) = 0;
};

template<class Function, class R, class...Args>
struct function : function_base<R, Args...>
{
  Function function_;
  function(Function f) : function_(std::move(f)) {}
  virtual R call(Args... args) { return function_(std::forward<Args>(args)...); }
};

struct continuation
{
  std::unique_ptr<function_base<void, bool>> function_;
  std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
};

using continuation_ptr = std::shared_ptr<continuation>;

template<class Function>
void attach(continuation_ptr& p, Function f)
{
  p->function_.reset(new function<Function, void, bool>(std::move(f)));
  if (p->flag_.test_and_set())
    p->function_->call(true);
}

inline void make_ready(continuation_ptr& p)
{
  future_impl::continuation_ptr p1{std::move(p)};
  if (p1->flag_.test_and_set())
    p1->function_->call(false);
}

template<class R> inline future<R> unwrap(future<R> f) { return f; }
template<class R> inline future<R> unwrap(future<future<R>> f) { return {std::move(f)}; }

struct default_executor
{
public:
  friend bool operator==(const default_executor&, const default_executor&) noexcept { return true; }
  friend bool operator!=(const default_executor&, const default_executor&) noexcept { return false; }
  template<class Function> void execute(Function f) const noexcept { f(); }
  constexpr bool query(execution::oneway_t) { return true; }
  constexpr bool query(execution::single_t) { return true; }
};

} // namespace future_impl

template<class R>
class promise
{
  template<class> friend class future;

  std::promise<R> impl_;
  future_impl::continuation_ptr continuation_{std::make_shared<future_impl::continuation>()};

public:
  promise() = default;
  template<class Alloc>
    promise(std::allocator_arg_t, const Alloc& alloc)
      : impl_(std::allocator_arg, alloc) {}
  promise(promise&& other) noexcept = default;
  promise(const promise& other) = delete;
  ~promise()
  {
    if (continuation_)
      set_exception(std::make_exception_ptr(std::future_error(std::future_errc::broken_promise)));
  }

  promise& operator=(promise&& other) noexcept = default;
  promise& operator=(const promise& other) = delete;
  void swap(promise& other) noexcept
  {
    std::swap(impl_, other.impl_);
    std::swap(continuation_, other.continuation_);
  }

  future<R> get_future() { return {*this}; }

  template <class... Args> auto set_value(Args&&... args)
    -> decltype(impl_.set_value(std::forward<Args>(args)...))
      { impl_.set_value(std::forward<Args>(args)...); make_ready(continuation_); }
  template <class... Args> auto set_exception(Args&&... args)
    -> decltype(impl_.set_exception(std::forward<Args>(args)...))
      { impl_.set_exception(std::forward<Args>(args)...); make_ready(continuation_); }
};

template<class R> inline void swap(promise<R>& a, promise<R>& b) noexcept { a.swap(b); }

template<class R>
class future
{
  template<class> friend class future;
  template<class> friend class promise;

  std::future<R> impl_;
  future_impl::continuation_ptr continuation_;
  future(promise<R>& prom) : impl_(prom.impl_.get_future()), continuation_(prom.continuation_) {}

public:
  future() noexcept = default;
  future(future&& other) noexcept = default;
  future(const future& other) = delete;
  future& operator=(future&& other) noexcept = default;
  future& operator=(const future& other) = delete;
  future(future<future<R>>&& fut);
  ~future() = default;

  std::shared_future<R> share() { return impl_.share(); }

  auto get() -> decltype(impl_.get()) { return impl_.get(); }

  bool valid() const { return impl_.valid(); }

  bool is_ready() const { return impl_.wait_for(chrono::seconds(0)) == future_status::ready; }

  void wait() { return impl_.wait(); }
  template<class Rep, class Period>
    future_status wait_for(const chrono::duration<Rep, Period>& rel_time) const
      { return impl_.wait_for(rel_time); }
  template<class Clock, class Duration>
    future_status wait_until(const chrono::time_point<Clock, Duration>& abs_time) const
      { return impl_.wait_until(abs_time); }

  template<class Executor, class Function>
    auto then(Executor ex, Function f);

  template<class Function> auto then(Function f)
    { return this->then(future_impl::default_executor(), std::move(f)); }
};

namespace future_impl {

template<class R, class F>
inline void set_value(promise<R>& p, F f)
{
  p.set_value(f());
}

template<class F>
inline void set_value(promise<void>& p, F f)
{
  f();
  p.set_value();
}

} // namespace future_impl

template<class R>
future<R>::future(future<future<R>>&& fut)
{
  promise<R> prom;
  *this = prom.get_future();
  future_impl::continuation_ptr cont(fut.continuation_);
  attach(cont, [prom = std::move(prom), pred = std::move(fut)](bool) mutable
      {
        try
        {
          future<R> next = pred.get();
          future_impl::continuation_ptr cont(next.continuation_);
          attach(cont, [prom = std::move(prom), pred = std::move(next)](bool) mutable
              {
                try
                {
                  future_impl::set_value(prom, [&]{ return pred.get(); });
                }
                catch (...)
                {
                  prom.set_exception(std::current_exception());
                }
              });
        }
        catch (...)
        {
          prom.set_exception(std::current_exception());
        }
      });
}

template<class R> template<class Executor, class Function>
auto future<R>::then(Executor ex, Function f)
{
  promise<typename std::result_of<Function(future)>::type> prom;
  future<typename std::result_of<Function(future)>::type> fut(prom.get_future());

  future_impl::continuation_ptr continuation(continuation_);
  future_impl::attach(continuation,
      [ex = execution::require(std::move(ex), execution::oneway), prom = std::move(prom),
        pred = std::move(*this), f = std::move(f)](bool nested_inside_then) mutable
      {
        auto func = [prom = std::move(prom), pred = std::move(pred), f = std::move(f)]() mutable
        {
          try
          {
            future_impl::set_value(prom, [&]{ return f(std::move(pred)); });
          }
          catch (...)
          {
            prom.set_exception(std::current_exception());
          }
        };

        if (nested_inside_then)
        {
          ex.execute(std::move(func));                                                    // #1: Least safe, most flexible
          // or execution::prefer(ex, execution::non_blocking).execute(std::move(func));  // #2: Safe when supported, flexible with effort
          // or execution::require(ex, execution::non_blocking).execute(std::move(func)); // #3: Safest, least flexible
        }
        else
          execution::prefer(ex, execution::possibly_blocking).execute(std::move(func));
      });

  return future_impl::unwrap(std::move(fut));
}

template<class R, class... Args>
class packaged_task<R(Args...)>
{
  std::unique_ptr<promise<R>> promise_;
  std::unique_ptr<future_impl::function_base<R, Args...>> task_;

  void call_helper(std::true_type, Args... args)
  {
    task_->call(std::forward<Args>(args)...);
    promise_->set_value();
  }

  void call_helper(std::false_type, Args... args)
  {
    promise_->set_value(task_->call(std::forward<Args>(args)...));
  }

public:
  packaged_task() noexcept = default;
  template<class F> explicit packaged_task(F&& f)
    : promise_(new promise<R>),
      task_(new future_impl::function<typename std::decay<F>::type, R, Args...>(std::forward<F>(f))) {}
  template<class Allocator, class F>
    explicit packaged_task(std::allocator_arg_t, const Allocator& a, F&& f)
      : promise_(new promise<R>(std::allocator_arg, a)),
        task_(new future_impl::function<typename std::decay<F>::type, R, Args...>(std::forward<F>(f))) {}
  packaged_task(packaged_task&& other) = default;
  packaged_task(const packaged_task&) = delete;
  packaged_task& operator=(packaged_task&& other) = default;
  packaged_task& operator=(const packaged_task& other) = delete;
  ~packaged_task() = default;

  bool valid() const noexcept { return !!task_; }

  void swap(packaged_task& other) noexcept
  {
    std::swap(promise_, other.promise_);
    std::swap(task_, other.task_);
  }

  future<R> get_future() { return promise_->get_future(); }

  void operator()(Args... args)
  { 
    try
    {
      this->call_helper(typename std::is_same<R, void>::type{}, std::forward<Args>(args)...);
    }
    catch (...)
    {
      promise_->set_exception(std::current_exception());
    }
  }
};

} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_FUTURE_H
