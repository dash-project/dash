#ifndef DASH__TASKS_TASKS_H__
#define DASH__TASKS_TASKS_H__

#include <dash/internal/Macro.h>

#include <dash/dart/if/dart_tasking.h>

namespace dash {
namespace tasks {
namespace internal {

  using task_action_t = std::function<void()>;

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

  //template<typename FuncT>
  static void invoke_task_action(void *data)
  {
    try {
      task_action_t *func = static_cast<task_action_t*>(data);
      func->operator()();
      delete func;
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

  template<typename ElementT>
  dart_task_dep_t
  in(dash::GlobRef<ElementT> globref, int32_t epoch = DART_EPOCH_ANY) {
    dart_task_dep_t res;
    res.gptr  = globref.dart_gptr();
    res.type  = DART_DEP_IN;
    res.epoch = epoch;
    return res;
  }

  template<typename ContainerT, typename ElementT>
  dart_task_dep_t
  in(ContainerT& container, ElementT* lptr, int32_t epoch = DART_EPOCH_ANY) {
    dart_task_dep_t res;
    res.gptr  = container.begin().dart_gptr();
    dart_gptr_incaddr(&res.gptr, lptr - container.lbegin());
    res.type  = DART_DEP_IN;
    res.epoch = epoch;
    return res;
  }

  template<typename ElementT>
  dart_task_dep_t
  out(dash::GlobRef<ElementT> globref, int32_t epoch = DART_EPOCH_ANY) {
    dart_task_dep_t res;
    res.gptr  = globref.dart_gptr();
    res.type  = DART_DEP_OUT;
    res.epoch = epoch;
    return res;
  }

  template<typename ContainerT, typename ElementT>
  dart_task_dep_t
  out(ContainerT& container, ElementT* lptr, int32_t epoch = DART_EPOCH_ANY) {
    dart_task_dep_t res;
    res.gptr  = container.begin().dart_gptr();
    dart_gptr_incaddr(&res.gptr, lptr - container.lbegin());
    res.type  = DART_DEP_OUT;
    res.epoch = epoch;
    return res;
  }

  static dart_task_dep_t
  direct(dart_taskref_t taskref) {
    dart_task_dep_t res;
    res.task  = taskref;
    res.type  = DART_DEP_DIRECT;
    return res;
  }

  DASH__NORETURN
  static void
  abort_task() {
    throw(dash::tasks::internal::AbortCancellationSignal());
  }

  DASH__NORETURN
  static void
  cancel_barrier() {
    if (dart_task_should_abort()) abort_task();
    throw(dash::tasks::internal::BarrierCancellationSignal());
  }

  DASH__NORETURN
  static void
  cancel_bcast() {
    if (dart_task_should_abort()) abort_task();
    throw(dash::tasks::internal::BcastCancellationSignal());
  }

  template<class TaskFunc, typename DepContainer>
  void
  create(TaskFunc f, dart_task_prio_t prio, const DepContainer& deps){
    if (dart_task_should_abort()) abort_task();
    dart_task_create(
      &dash::tasks::internal::invoke_task_action,
      new dash::tasks::internal::task_action_t(f), 0,
                     deps.data(), deps.size(), prio);
  }


  template<class TaskFunc, typename ... Args>
  void
  create(TaskFunc f, dart_task_prio_t prio, const Args&... args){
    std::array<dart_task_dep_t, sizeof...(args)> deps({{
      static_cast<dart_task_dep_t>(args)...
    }});
    create(f, prio, deps);
  }

  template<class TaskFunc, typename ... Args>
  void
  create(TaskFunc f, const Args&... args){
    create(f, DART_PRIO_LOW, args...);
  }


  template<class TaskFunc, typename ... Args>
  dart_taskref_t
  create_handle(TaskFunc f, dart_task_prio_t prio, const Args&... args){
    if (dart_task_should_abort()) abort_task();
    std::array<dart_task_dep_t, sizeof...(args)> deps({{
      static_cast<dart_task_dep_t>(args)...
    }});
    dart_task_create(
      &dash::tasks::internal::invoke_task_action,
      new dash::tasks::internal::task_action_t(f), 0,
                     deps.data(), deps.size(), prio);
  }

  template<class TaskFunc, typename ... Args>
  dart_taskref_t
  create_handle(TaskFunc f, const Args&... args){
    return create_handle(f, DART_PRIO_LOW, args...);
  }

  static void
  yield(int delay = -1) {
    if (dart_task_should_abort()) abort_task();
    dart_task_yield(delay);
  }

  static void
  complete() {
    if (dart_task_should_abort()) abort_task();
    dart_task_complete();
  }

} // namespace tasks

} // namespace dash

#endif // DASH__TASKS_TASKS_H__
