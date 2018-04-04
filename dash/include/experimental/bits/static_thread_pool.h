#ifndef STD_EXPERIMENTAL_BITS_STATIC_THREAD_POOL_H
#define STD_EXPERIMENTAL_BITS_STATIC_THREAD_POOL_H

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <experimental/future>
#include <list>
#include <memory>
#include <mutex>
#include <new>
#include <thread>
#include <tuple>

namespace std {
namespace experimental {
inline namespace executors_v1 {

class static_thread_pool
{
  template<class, class T, class U> struct dependent_is_same : std::is_same<T, U> {};

  template<class Blocking, class Continuation, class Work, class ProtoAllocator>
  class executor_impl
  {
    friend class static_thread_pool;
    static_thread_pool* pool_;
    ProtoAllocator allocator_;

    executor_impl(static_thread_pool* p, const ProtoAllocator& a) noexcept
      : pool_(p), allocator_(a) { pool_->work_up(Work{}); }

  public:
    using shape_type = std::size_t;

    executor_impl(const executor_impl& other) noexcept : pool_(other.pool_) { pool_->work_up(Work{}); }
    ~executor_impl() { pool_->work_down(Work{}); }

    // Associated execution context.
    static_thread_pool& query(execution::context_t) const noexcept { return *pool_; }

    // Blocking modes.
    executor_impl<execution::never_blocking_t, Continuation, Work, ProtoAllocator>
      require(execution::never_blocking_t) const { return {pool_, allocator_}; };
    executor_impl<execution::possibly_blocking_t, Continuation, Work, ProtoAllocator>
      require(execution::possibly_blocking_t) const { return {pool_, allocator_}; };
    executor_impl<execution::always_blocking_t, Continuation, Work, ProtoAllocator>
      require(execution::always_blocking_t) const { return {pool_, allocator_}; };
    bool query(execution::never_blocking_t) const { return is_same<Blocking, execution::never_blocking_t>::value; }
    bool query(execution::possibly_blocking_t) const { return is_same<Blocking, execution::possibly_blocking_t>::value; }
    bool query(execution::always_blocking_t) const { return is_same<Blocking, execution::always_blocking_t>::value; }

    // Continuation hint.
    executor_impl<Blocking, execution::continuation_t, Work, ProtoAllocator>
      require(execution::continuation_t) const { return {pool_, allocator_}; };
    executor_impl<Blocking, execution::not_continuation_t, Work, ProtoAllocator>
      require(execution::not_continuation_t) const { return {pool_, allocator_}; };
    bool query(execution::continuation_t) const { return is_same<Continuation, execution::continuation_t>::value; }
    bool query(execution::not_continuation_t) const { return is_same<Continuation, execution::not_continuation_t>::value; }

    // Work tracking.
    executor_impl<Blocking, Continuation, execution::outstanding_work_t, ProtoAllocator>
      require(execution::outstanding_work_t) const { return {pool_, allocator_}; };
    executor_impl<Blocking, Continuation, execution::not_outstanding_work_t, ProtoAllocator>
      require(execution::not_outstanding_work_t) const { return {pool_, allocator_}; };
    bool query(execution::outstanding_work_t) const { return is_same<Work, execution::outstanding_work_t>::value; }
    bool query(execution::not_outstanding_work_t) const { return is_same<Work, execution::not_outstanding_work_t>::value; }

    // Bulk forward progress.
    executor_impl require(execution::bulk_parallel_execution_t) const { return *this; }
    bool query(execution::bulk_parallel_execution_t) const { return true; }

    // Mapping of execution on to threads.
    executor_impl require(execution::thread_execution_mapping_t) const { return *this; }
    bool query(execution::thread_execution_mapping_t) const { return true; }

    // Allocator.
    executor_impl<Blocking, Continuation, Work, std::allocator<void>>
      require(const execution::allocator_t<void>&) const { return {pool_, std::allocator<void>{}}; };
    template<class NewProtoAllocator>
      executor_impl<Blocking, Continuation, Work, NewProtoAllocator>
        require(const execution::allocator_t<NewProtoAllocator>& a) const { return {pool_, a.value()}; }
    ProtoAllocator query(const execution::allocator_t<ProtoAllocator>&) const noexcept { return allocator_; }
    ProtoAllocator query(const execution::allocator_t<void>&) const noexcept { return allocator_; }

    bool running_in_this_thread() const noexcept { return pool_->running_in_this_thread(); }

    friend bool operator==(const executor_impl& a, const executor_impl& b) noexcept
    {
      return a.pool_ == b.pool_;
    }

    friend bool operator!=(const executor_impl& a, const executor_impl& b) noexcept
    {
      return a.pool_ != b.pool_;
    }

    template<class Function> void execute(Function f) const
    {
      pool_->execute(Blocking{}, Continuation{}, allocator_, std::move(f));
    }

    template<class Function> auto twoway_execute(Function f) const -> future<decltype(f())>
    {
      return pool_->twoway_execute(Blocking{}, Continuation{}, allocator_, std::move(f));
    }

    template<class Function, class SharedFactory> void bulk_execute(Function f, std::size_t n, SharedFactory sf) const
    {
      pool_->bulk_execute(Blocking{}, Continuation{}, allocator_, std::move(f), n, std::move(sf));
    }

    template<class Function, class ResultFactory, class SharedFactory>
    auto bulk_twoway_execute(Function f, std::size_t n, ResultFactory rf, SharedFactory sf) const -> future<decltype(rf())>
    {
      return pool_->bulk_twoway_execute(Blocking{}, Continuation{}, allocator_, std::move(f), n, std::move(rf), std::move(sf));
    }
  };

public:
  using executor_type = executor_impl<execution::possibly_blocking_t, execution::not_continuation_t, execution::not_outstanding_work_t, std::allocator<void>>;

  explicit static_thread_pool(std::size_t threads)
  {
    for (std::size_t i = 0; i < threads; ++i)
      threads_.emplace_back([this]{ attach(); });
  }

  static_thread_pool(const static_thread_pool&) = delete;
  static_thread_pool& operator=(const static_thread_pool&) = delete;

  ~static_thread_pool()
  {
    stop();
    wait();
  }

  executor_type executor() noexcept
  {
    return executor_type{this, std::allocator<void>{}};
  }

  void attach()
  {
    thread_private_state private_state{this};
    for (std::unique_lock<std::mutex> lock(mutex_);;)
    {
      condition_.wait(lock, [this]{ return stopped_ || work_ == 0 || head_; });
      if (stopped_ || (work_ == 0 && !head_)) return;
      func_base* func = head_.release();
      head_ = std::move(func->next_);
      tail_ = head_ ? tail_ : &head_;
      lock.unlock();
      func->call();
      lock.lock();
      if (private_state.head_)
      {
        *tail_ = std::move(private_state.head_);
        tail_ = private_state.tail_;
        private_state.tail_ = &private_state.head_;
        // TODO notify other threads if more than one in private queue
      }
    }
  }

  void stop()
  {
    std::unique_lock<std::mutex> lock(mutex_);
    stopped_ = true;
    condition_.notify_all();
  }

  void wait()
  {
    std::unique_lock<std::mutex> lock(mutex_);
    std::list<std::thread> threads(std::move(threads_));
    if (!threads.empty())
    {
      --work_;
      condition_.notify_all();
      lock.unlock();
      for (auto& t : threads)
        t.join();
    }
  }

private:
  template<class Function>
  static void invoke(Function& f) noexcept // Exceptions mean std::terminate.
  {
    f();
  }

  struct func_base
  {
    struct deleter { void operator()(func_base* p) { p->destroy(); } };
    using pointer = std::unique_ptr<func_base, deleter>;

    virtual ~func_base() {}
    virtual void call() = 0;
    virtual void destroy() = 0;

    pointer next_;
  };

  template<class Function, class ProtoAllocator>
  struct func : func_base
  {
    explicit func(Function f, const ProtoAllocator& a) : function_(std::move(f)), allocator_(a) {}

    static func_base::pointer create(Function f, const ProtoAllocator& a)
		{
			typename std::allocator_traits<ProtoAllocator>::template rebind_alloc<func> allocator(a);
			func* raw_p = allocator.allocate(1);
			try
			{
        func* p = new (raw_p) func(std::move(f), a);
				return func_base::pointer(p);
			}
			catch (...)
			{
				allocator.deallocate(raw_p, 1);
				throw;
			}
		}

    virtual void call()
    {
      func_base::pointer fp(this);
      Function f(std::move(function_));
      fp.reset();
      static_thread_pool::invoke(f);
    }

    virtual void destroy()
    {
      func* p = this;
      p->~func();
      allocator_.deallocate(p, 1);
    }

    Function function_;
    typename std::allocator_traits<ProtoAllocator>::template rebind_alloc<func> allocator_;
  };

  struct thread_private_state
  {
    static_thread_pool* pool_;
    func_base::pointer head_;
    func_base::pointer* tail_{&head_};
    thread_private_state* prev_state_{instance()};

    explicit thread_private_state(static_thread_pool* p) : pool_(p) { instance() = this; }
    ~thread_private_state() { instance() = prev_state_; }

    static thread_private_state*& instance()
    {
      static thread_local thread_private_state* p;
      return p;
    }
  };

  bool running_in_this_thread() const noexcept
  {
    if (thread_private_state* private_state = thread_private_state::instance())
      if (private_state->pool_ == this)
        return true;
    return false;
  }

  template<class Blocking, class Continuation, class ProtoAllocator, class Function>
  void execute(Blocking, Continuation, const ProtoAllocator& alloc, Function f)
  {
    if (std::is_same<Blocking, execution::possibly_blocking_t>::value)
    {
      // Run immediately if already in the pool.
      if (thread_private_state* private_state = thread_private_state::instance())
      {
        if (private_state->pool_ == this)
        {
          static_thread_pool::invoke(f);
          return;
        }
      }
    }

    func_base::pointer fp(func<Function, ProtoAllocator>::create(std::move(f), alloc));

    if (std::is_same<Continuation, execution::continuation_t>::value)
    {
      // Push to thread-private queue if available.
      if (thread_private_state* private_state = thread_private_state::instance())
      {
        if (private_state->pool_ == this)
        {
          *private_state->tail_ = std::move(fp);
          private_state->tail_ = &(*private_state->tail_)->next_;
          return;
        }
      }
    }

    // Otherwise push to main queue.
    std::unique_lock<std::mutex> lock(mutex_);
    *tail_ = std::move(fp);
    tail_ = &(*tail_)->next_;
    condition_.notify_one();
  }

  template<class Continuation, class ProtoAllocator, class Function>
  void execute(execution::always_blocking_t, Continuation, const ProtoAllocator& alloc, Function f)
  {
    // Run immediately if already in the pool.
    if (thread_private_state* private_state = thread_private_state::instance())
    {
      if (private_state->pool_ == this)
      {
        static_thread_pool::invoke(f);
        return;
      }
    }

    // Otherwise, wrap the function with a promise that, when broken, will signal that the function is complete.
    promise<void> promise;
    future<void> future = promise.get_future();
    this->execute(execution::never_blocking, Continuation{}, alloc, [f = std::move(f), p = std::move(promise)]() mutable { f(); });
    future.wait();
  }

  template<class Blocking, class Continuation, class ProtoAllocator, class Function>
  auto twoway_execute(Blocking, Continuation, const ProtoAllocator& alloc, Function f) -> future<decltype(f())>
  {
    packaged_task<decltype(f())()> task(std::move(f));
    future<decltype(f())> future = task.get_future();
    this->execute(Blocking{}, Continuation{}, alloc, std::move(task));
    return future;
  }

  template<class Function, class SharedFactory>
  struct bulk_state
  {
    Function f_;
    decltype(std::declval<SharedFactory>()()) ss_;
    bulk_state(Function f, SharedFactory sf) : f_(std::move(f)), ss_(sf()) {}
    void operator()(std::size_t i) { f_(i, ss_); }
  };

  template<class Blocking, class Continuation, class ProtoAllocator, class Function, class SharedFactory>
  void bulk_execute(Blocking, Continuation, const ProtoAllocator& alloc, Function f, std::size_t n, SharedFactory sf)
  {
    typename std::allocator_traits<ProtoAllocator>::template rebind_alloc<char> alloc2(alloc);
    auto shared_state = std::allocate_shared<bulk_state<Function, SharedFactory>>(alloc2, std::move(f), std::move(sf));

    func_base::pointer head;
    func_base::pointer* tail{&head};
    for (std::size_t i = 0; i < n; ++i)
    {
      auto indexed_f = [shared_state, i]() mutable { (*shared_state)(i); };
      *tail = func_base::pointer(func<decltype(indexed_f), ProtoAllocator>::create(std::move(indexed_f), alloc));
      tail = &(*tail)->next_;
    }

    if (std::is_same<Continuation, execution::continuation_t>::value)
    {
      // Push to thread-private queue if available.
      if (thread_private_state* private_state = thread_private_state::instance())
      {
        if (private_state->pool_ == this)
        {
          *private_state->tail_ = std::move(head);
          private_state->tail_ = tail;
          return;
        }
      }
    }

    // Otherwise push to main queue.
    std::unique_lock<std::mutex> lock(mutex_);
    *tail_ = std::move(head);
    tail_ = tail;
    condition_.notify_all();
  }

  template<class Continuation, class ProtoAllocator, class Function, class SharedFactory>
  void bulk_execute(execution::always_blocking_t, Continuation, const ProtoAllocator& alloc, Function f, std::size_t n, SharedFactory sf)
  {
    // Wrap the function with a promise that, when broken, will signal that the function is complete.
    promise<void> promise;
    future<void> future = promise.get_future();
    auto wrapped_f = [f = std::move(f), p = std::move(promise)](std::size_t n, auto& s) mutable { f(n, s); };
    this->bulk_execute(execution::never_blocking, Continuation{}, alloc, std::move(wrapped_f), n, std::move(sf));
    future.wait();
  }

  template<class Blocking, class Continuation, class ProtoAllocator, class Function, class ResultFactory, class SharedFactory>
  auto bulk_twoway_execute(Blocking, Continuation, const ProtoAllocator& alloc, Function f, std::size_t n, ResultFactory rf, SharedFactory sf)
    -> typename std::enable_if<is_same<decltype(rf()), void>::value, future<void>>::type
  {
    // Wrap the shared state so that we can capture and return the result.
    typename std::allocator_traits<ProtoAllocator>::template rebind_alloc<char> alloc2(alloc);
    auto shared_state = std::allocate_shared<
        std::tuple<
          std::atomic<std::size_t>, // Number of incomplete functions.
          decltype(sf()), // Underlying shared state.
          std::atomic<std::size_t>, // Number of exceptions raised.
          std::exception_ptr, // First exception raised.
          promise<void> // Promise to receive result
        >>(alloc2, n, sf(), 0, nullptr, promise<void>());
    future<void> future = std::get<4>(*shared_state).get_future();

    // Convert to a one way bulk operation.
    this->bulk_execute(Blocking{}, Continuation{}, alloc,
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

  template<class Blocking, class Continuation, class ProtoAllocator, class Function, class ResultFactory, class SharedFactory>
  auto bulk_twoway_execute(Blocking, Continuation, const ProtoAllocator& alloc, Function f, std::size_t n, ResultFactory rf, SharedFactory sf)
    -> typename std::enable_if<!is_same<decltype(rf()), void>::value, future<decltype(rf())>>::type
  {
    // Wrap the shared state so that we can capture and return the result.
    typename std::allocator_traits<ProtoAllocator>::template rebind_alloc<char> alloc2(alloc);
    auto shared_state = std::allocate_shared<
        std::tuple<
          std::atomic<std::size_t>, // Number of incomplete functions.
          decltype(rf()), // Result.
          decltype(sf()), // Underlying shared state.
          std::atomic<std::size_t>, // Number of exceptions raised.
          std::exception_ptr, // First exception raised.
          promise<decltype(rf())> // Promise to receive result
        >>(alloc2, n, rf(), sf(), 0, nullptr, promise<decltype(rf())>());
    future<decltype(rf())> future = std::get<5>(*shared_state).get_future();

    // Convert to a one way bulk operation.
    this->bulk_execute(Blocking{}, Continuation{}, alloc,
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

  template<class Blocking, class Continuation, class ProtoAllocator, class Function, class ResultFactory, class SharedFactory>
  auto bulk_twoway_execute(execution::always_blocking_t, Continuation, const ProtoAllocator& alloc, Function f, std::size_t n, ResultFactory rf, SharedFactory sf)
  {
    auto future = this->bulk_twoway_execute(execution::never_blocking, Continuation{}, alloc, std::move(f), n, std::move(rf), std::move(sf));
    future.wait();
    return future;
  }

  void work_up(execution::outstanding_work_t) noexcept
  {
    std::unique_lock<std::mutex> lock(mutex_);
    ++work_;
  }

  void work_down(execution::outstanding_work_t) noexcept
  {
    std::unique_lock<std::mutex> lock(mutex_);
    if (--work_ == 0)
      condition_.notify_all();
  }

  void work_up(execution::not_outstanding_work_t) noexcept {}
  void work_down(execution::not_outstanding_work_t) noexcept {}

  std::mutex mutex_;
  std::condition_variable condition_;
  std::list<std::thread> threads_;
  func_base::pointer head_;
  func_base::pointer* tail_{&head_};
  bool stopped_{false};
  std::size_t work_{1};
};

} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_STATIC_THREAD_POOL_H
