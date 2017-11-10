#ifndef DASH__TASKS_TASKS_H__
#define DASH__TASKS_TASKS_H__

#include <dash/internal/Macro.h>
#include <dash/Exception.h>

#include <dash/dart/if/dart_tasking.h>

namespace dash {
namespace tasks {
namespace internal {
  template<typename T=void>
  using task_action_t = std::function<T()>;

  // dummy base class to catch them all
  class BaseCancellationSignal {
  public:
    BaseCancellationSignal() { }
    virtual
    ~BaseCancellationSignal() { }
  };

  template<class CancellationMethod>
  class CancellationSignal : public BaseCancellationSignal {
  public:
    CancellationSignal() { }
    // do not allow copy construction and assignment
    CancellationSignal(const CancellationSignal&) = delete;
    CancellationSignal& operator=(const CancellationSignal&) = delete;

    CancellationSignal& operator=(CancellationSignal&& other) =  delete;
    CancellationSignal(CancellationSignal&& other)
    : will_cancel(other.will_cancel)
    {
      // transfer ownership of the cancellation
      other.will_cancel = false;
    }

    virtual
    ~CancellationSignal() {
      if (will_cancel) {
        CancellationMethod::cancel();
      }
    }

  private:
    bool will_cancel = true;
  };


  struct BcastCancellation {
    static void cancel() DASH__NORETURN {
      dart_task_cancel_bcast();
    }
  };

  struct BarrierCancellation{
    static void cancel() DASH__NORETURN {
      dart_task_cancel_barrier();
    }
  };

  struct AbortCancellation{
    static void cancel() DASH__NORETURN {
      dart_task_abort();
    }
  };

  using BcastCancellationSignal  = CancellationSignal<BcastCancellation>;
  using BarrierCancellationSignal = CancellationSignal<BarrierCancellation>;
  using AbortCancellationSignal  = CancellationSignal<AbortCancellation>;

  template<typename ReturnT = void>
  struct TaskData {

    TaskData(task_action_t<ReturnT> func) : func(func) { }
    TaskData(
      task_action_t<ReturnT> func,
      std::shared_ptr<ReturnT> result) : func(func), result(result) { }

    task_action_t<ReturnT>   func;
    std::shared_ptr<ReturnT> result = NULL;
  };


  template<typename T>
  typename std::enable_if<!std::is_same<T, void>::value, void>::type
  invoke_task_action_impl(TaskData<T>& data)
  {
    task_action_t<T>& func = data.func;
    assert(data.result != NULL);
    if (data.result != NULL) {
      *data.result = func();
    } else {
      func();
    }
  }

  template<typename T>
  typename std::enable_if<std::is_same<T, void>::value, void>::type
  invoke_task_action_impl(TaskData<T>& data)
  {
    task_action_t<T>& func = data.func;
    func();
  }


  template<typename T=void>
  void
  invoke_task_action(void *data)
  {
    try {
      TaskData<T> *td = static_cast<TaskData<T>*>(data);
      invoke_task_action_impl(*td);
      delete td;
    } catch (const BaseCancellationSignal& cs) {
      // nothing to be done, the cancellation is triggered by the d'tor
    } catch (...) {
      DASH_LOG_ERROR(
        "An unhandled exception is escaping one of your tasks, "
        "your application will thus abort.");
      throw;
    }
  }

} // namespace internal

  template<typename T>
  class TaskHandle {
  public:
    using self_t = TaskHandle<T>;

    // create an empty task handle
    TaskHandle() { }

    TaskHandle(dart_taskref_t ref, std::shared_ptr<T> retval)
    : _ref(ref), _ret(retval) { }

    // Do not allow copying the task handle to avoid double references
    TaskHandle(const self_t&) = delete;
    self_t& operator=(const self_t&) = delete;

    TaskHandle(self_t&& other) {
      _ref = other._ref;
      other._ref = DART_TASK_NULL;
      std::swap(_ret, other._ret);
    }

    self_t& operator=(self_t&& other) {
      if (&other == this) return;
      _ref = other._ref;
      other._ref = DART_TASK_NULL;
      std::swap(_ret, other._ret);
    }

    ~TaskHandle() {
      if (_ref != DART_TASK_NULL) {
        dart_task_freeref(&_ref);
      }
    }

    bool test() {
      if (_ref != DART_TASK_NULL) {
        int flag;
        dart_task_test(&_ref, &flag);
        if (flag != 0) _ready = true;
        return (flag != 0);
      }
    }

    void wait() {
      if (_ref != DART_TASK_NULL) {
        dart_task_wait(&_ref);
      }
    }

    T get() {
      DASH_ASSERT(_ready || _ref != DART_TASK_NULL);
      if (!_ready) wait();
      return *_ret;
    }

    dart_taskref_t dart_handle() const {
      return _ref;
    }

  private:
    dart_taskref_t     _ref   = DART_TASK_NULL;
    std::shared_ptr<T> _ret   = NULL;
    bool               _ready = false;
  };


  template<>
  class TaskHandle<void> {
  public:
    using self_t = TaskHandle<void>;

    // create an empty task handle
    TaskHandle() { }

    TaskHandle(dart_taskref_t ref) : _ref(ref) { }

    // Do not allow copying the task handle to avoid double references
    TaskHandle(const self_t&) = delete;
    self_t& operator=(const self_t&) = delete;

    TaskHandle(self_t&& other) {
      _ref = other._ref;
      other._ref = DART_TASK_NULL;
    }

    self_t& operator=(self_t&& other) {
      if (&other == this) return *this;
      _ref = other._ref;
      other._ref = DART_TASK_NULL;
    }

    ~TaskHandle() {
      if (_ref != DART_TASK_NULL) {
        dart_task_freeref(&_ref);
      }
    }

    bool test() {
      int flag;
      dart_task_test(&_ref, &flag);
      return (flag != 0);
    }

    void wait() {
      dart_task_wait(&_ref);
    }

    dart_taskref_t dart_handle() const {
      return _ref;
    }

  private:
    dart_taskref_t _ref = DART_TASK_NULL;
  };

  class TaskDependency {
  public:
    template<typename ElementT>
    TaskDependency(
      const dash::GlobRef<ElementT>& globref,
      dart_task_deptype_t       type,
      dart_taskphase_t          phase = DART_PHASE_TASK) {
      _dep.gptr  = globref.dart_gptr();
      _dep.type  = type;
      _dep.phase = phase;
    }

    template<typename T>
    TaskDependency(
      const T             * lptr,
      dart_task_deptype_t   type,
      dart_taskphase_t      phase = DART_PHASE_TASK) {
      _dep.gptr  = DART_GPTR_NULL;
      _dep.gptr.addr_or_offs.addr = const_cast<T*>(lptr);
      _dep.gptr.segid  = -1; // segment ID for attached memory, not translated
      _dep.gptr.unitid = dash::myid();
      _dep.gptr.teamid = dash::Team::All().dart_id();
      _dep.type  = type;
      _dep.phase = phase;
    }

    template<typename T>
    TaskDependency(const TaskHandle<T>& handle) {
      _dep.task  = handle.dart_handle();
      _dep.type  = DART_DEP_DIRECT;
    }

    operator dart_task_dep_t() {
      return _dep;
    }

  protected:
    dart_task_dep_t _dep;
  };

  template<typename ElementT>
  TaskDependency
  in(const dash::GlobRef<ElementT>& globref, int32_t phase = DART_PHASE_TASK) {
    return TaskDependency(globref, DART_DEP_IN, phase);
  }

  template<typename T>
  TaskDependency
  in(const T* lptr, int32_t phase = DART_PHASE_TASK) {
    return TaskDependency(lptr, DART_DEP_IN, phase);
  }

  template<typename ElementT>
  TaskDependency
  out(const dash::GlobRef<ElementT>& globref, int32_t phase = DART_PHASE_TASK) {
    return TaskDependency(globref, DART_DEP_OUT, phase);
  }

  template<typename T>
  TaskDependency
  out(const T* lptr, int32_t phase = DART_PHASE_TASK) {
    return TaskDependency(lptr, DART_DEP_OUT, phase);
  }

  template<typename T>
  TaskDependency
  direct(const TaskHandle<T>& taskref) {
    return TaskDependency(taskref);
  }

  template<typename T=int>
  void
  abort_task() DASH__NORETURN;

  template<typename T>
  void
  abort_task() {
    throw(dash::tasks::internal::AbortCancellationSignal());
  }

  template<typename T=int>
  void
  cancel_barrier() DASH__NORETURN;

  template<typename T>
  void
  cancel_barrier() {
    if (dart_task_should_abort()) abort_task();
    throw(dash::tasks::internal::BarrierCancellationSignal());
  }

  template<typename T=int>
  void
  cancel_bcast() DASH__NORETURN;

  template<typename T>
  void
  cancel_bcast() {
    if (dart_task_should_abort()) abort_task();
    throw(dash::tasks::internal::BcastCancellationSignal());
  }

  template<class TaskFunc, typename DepContainer>
  void
  async(
    TaskFunc            f,
    dart_task_prio_t    prio,
    const DepContainer& deps){
    if (dart_task_should_abort()) abort_task();
    dart_task_create(
      &dash::tasks::internal::invoke_task_action<void>,
      new dash::tasks::internal::TaskData<void>(f), 0,
                     deps.data(), deps.size(), prio);
  }

  template<class TaskFunc, typename ... Args>
  void
  async(
    TaskFunc         f,
    dart_task_prio_t prio){
    std::array<dart_task_dep_t, 0> deps;
    async(f, prio, deps);
  }

  template<class TaskFunc, typename ... Args>
  void
  async(
    TaskFunc         f,
    dart_task_prio_t prio,
    TaskDependency   dep,
    const Args&...   args){
    std::array<dart_task_dep_t, sizeof...(args)+1> deps({{dep,
      static_cast<dart_task_dep_t>(args)...
    }});
    async(f, prio, deps);
  }

  template<class TaskFunc, typename ... Args>
  void
  async(TaskFunc f, const Args&... args){
    async(f, DART_PRIO_LOW, args...);
  }

  template<class TaskFunc, typename DepContainer>
  auto
  async_handle(
    TaskFunc f,
    dart_task_prio_t prio,
    const DepContainer& deps) -> TaskHandle<decltype(f())>
  {
    using return_t = decltype(f());
    if (dart_task_should_abort()) abort_task();
    dart_taskref_t handle;
    std::shared_ptr<return_t> retval = std::make_shared<return_t>();
    dart_task_create_handle(
      &dash::tasks::internal::invoke_task_action<return_t>,
      new dash::tasks::internal::TaskData<return_t>(
            dash::tasks::internal::task_action_t<return_t>(f), retval),
      0, deps.data(), deps.size(), prio, &handle);
    return TaskHandle<return_t>(handle, retval);
  }

  template<class TaskFunc>
  auto
  async_handle(
    TaskFunc f,
    dart_task_prio_t prio) -> TaskHandle<decltype(f())>
  {
    std::array<dart_task_dep_t, 0> deps;
    return async_handle(f, prio, deps);
  }

  template<class TaskFunc, typename ... Args>
  auto
  async_handle(
    TaskFunc f,
    dart_task_prio_t prio,
    TaskDependency dep,
    const Args&... args) -> TaskHandle<decltype(f())>
  {
    std::array<dart_task_dep_t, sizeof...(args)+1> deps({{dep,
      static_cast<dart_task_dep_t>(args)...
    }});
    return async_handle(f, prio, deps);
  }

  template<class TaskFunc, typename ... Args>
  auto
  async_handle(
    TaskFunc f,
    const Args&... args) -> TaskHandle<decltype(f())>
  {
    return async_handle(f, DART_PRIO_LOW, args...);
  }

  template<typename T=int>
  void async_barrier() {
    dart_task_phase_advance();
  }

  template<typename T=int>
  void resync_barrier(dash::Team& team = dash::Team::All()) {
    dart_task_phase_resync(team.dart_id());
  }

  template<typename T=int>
  void
  yield(int delay = -1) {
    if (dart_task_should_abort()) abort_task();
    dart_task_yield(delay);
  }

  template<typename T=int>
  void
  complete() {
    if (dart_task_should_abort()) abort_task();
    dart_task_complete();
  }

} // namespace tasks

} // namespace dash

#endif // DASH__TASKS_TASKS_H__
