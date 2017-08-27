
#include <dash/dart/if/dart_tasking.h>

namespace dash {
namespace tasks {
namespace internal {

  using task_function = std::function<void()>;

  //template<typename FuncT>
  static void invoke_task_action(void *data)
  {
    task_function *func = static_cast<task_function*>(data);
    func->operator()();
    delete func;
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

  dart_task_dep_t
  direct(dart_taskref_t taskref) {
    dart_task_dep_t res;
    res.task  = taskref;
    res.type  = DART_DEP_DIRECT;
    return res;
  }


  template<class TaskFunc, typename DepContainer>
  void
  create(TaskFunc f, dart_task_prio_t prio, const DepContainer& deps){
    dart_task_create(
      &dash::tasks::internal::invoke_task_action,
      new dash::internal::task_function(f), 0,
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
    std::array<dart_task_dep_t, sizeof...(args)> deps({{
      static_cast<dart_task_dep_t>(args)...
    }});
    dart_task_create(
      &dash::tasks::internal::invoke_task_action,
      new dash::internal::task_function(f), 0,
                     deps.data(), deps.size(), prio);
  }

  template<class TaskFunc, typename ... Args>
  dart_taskref_t
  create_handle(TaskFunc f, const Args&... args){
    return create_handle(f, DART_PRIO_LOW, args...);
  }

} // namespace tasks

} // namespace dash