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

  template<typename FuncT>
  void
  invoke_task_action_void(void *data)
  {
    try{
      FuncT& f = *static_cast<FuncT*>(data);
      f();
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


  using DependencyVector = std::vector<dart_task_dep_t>;
  using DependencyVectorInserter = std::insert_iterator<DependencyVector>;
  using DependencyGenerator = std::function<void(DependencyVectorInserter)>;

  /**
   * Class representing a task created through \ref dash::tasks::async_handle.
   *
   * The handle can be used to test whether the task has finished execution,
   * to wait for its completion, and to retrieve it's return value.
   */
  template<typename T>
  class TaskHandle {
  public:
    using self_t = TaskHandle<T>;

    /**
     * create an empty task handle
     */
    constexpr
    TaskHandle() { }

    /**
     * Create a TaskHandle from a DART task handle and a pointer to the return
     * value that is shared with the task instance.
     */
    TaskHandle(dart_taskref_t ref, std::shared_ptr<T> retval)
    : _ref(ref), _ret(std::move(retval)) { }

    // Do not allow copying the task handle to avoid double references
    TaskHandle(const self_t&) = delete;
    self_t& operator=(const self_t&) = delete;

    /**
     * Move constructor.
     */
    TaskHandle(self_t&& other) {
      _ref = other._ref;
      other._ref = DART_TASK_NULL;
      _ready = other._ready;
      std::swap(_ret, other._ret);
    }

    /**
     * Move operator.
     */
    self_t& operator=(self_t&& other) {
      if (&other == this) return *this;
      _ref = other._ref;
      other._ref = DART_TASK_NULL;
      _ready = other._ready;
      std::swap(_ret, other._ret);
      return *this;
    }

    ~TaskHandle() {
      if (_ref != DART_TASK_NULL) {
        dart_task_freeref(&_ref);
      }
    }

    /**
     * Test for completion of the task.
     */
    bool test() {
      if (_ready) return true;
      if (_ref != DART_TASK_NULL) {
        int flag;
        dart_task_test(&_ref, &flag);
        if (flag != 0) _ready = true;
        return (flag != 0);
      }
      DASH_ASSERT(_ready || _ref != DART_TASK_NULL); // should not happen
    }

    /**
     * Wait for completion of the task.
     */
    void wait() {
      if (_ref != DART_TASK_NULL) {
        dart_task_wait(&_ref);
        _ready = true;
      }
    }

    /**
     * Get the result of the task and wait for it if the task has not completed.
     */
    T get() {
      DASH_ASSERT(_ready || _ref != DART_TASK_NULL);
      if (!_ready) wait();
      return *_ret;
    }

    /**
     * Return the underlying DART task handle.
     */
    dart_taskref_t dart_handle() const {
      return _ref;
    }

  private:
    dart_taskref_t     _ref   = DART_TASK_NULL;
    std::shared_ptr<T> _ret   = NULL;
    bool               _ready = false;
  };


  /**
   * Class representing a handle to a task created through
   * \ref dash::tasks::async_handle.
   *
   * The handle can be used to test whether the task has finished execution
   * and to wait its completion.
   *
   * This is the specialization for \c TaskHandle<void>, which does not return
   * a value.
   */
  template<>
  class TaskHandle<void> {
  public:
    using self_t = TaskHandle<void>;

    /**
     * Create an empty task handle
     */
    constexpr
    TaskHandle() { }

    /**
     * Create a TaskHandle from a DART task handle.
     */
    TaskHandle(dart_taskref_t ref) : _ref(ref) { }

    // Do not allow copying the task handle to avoid double references
    TaskHandle(const self_t&) = delete;
    self_t& operator=(const self_t&) = delete;

    /**
     * Move constructor.
     */
    TaskHandle(self_t&& other) {
      _ref = other._ref;
      other._ref = DART_TASK_NULL;
      _ready = other._ready;
    }

    self_t& operator=(self_t&& other) {
      if (&other == this) return *this;
      _ref = other._ref;
      other._ref = DART_TASK_NULL;
      _ready = other._ready;
      return *this;
    }

    ~TaskHandle() {
      if (_ref != DART_TASK_NULL) {
        dart_task_freeref(&_ref);
      }
    }

    /**
     * Test for completion of the task.
     */
    bool test() {
      if (_ready) return true;
      if (_ref != DART_TASK_NULL) {
        int flag;
        dart_task_test(&_ref, &flag);
        if (flag != 0) _ready = true;
        return (flag != 0);
      }
      DASH_ASSERT(_ready || _ref != DART_TASK_NULL); // should not happen
      return false;
    }

    /**
     * Wait for completion of the task.
     */
    void wait() {
      if (_ref != DART_TASK_NULL) {
        dart_task_wait(&_ref);
      }
    }

    /**
     * Return the underlying DART task handle.
     */
    dart_taskref_t dart_handle() const {
      return _ref;
    }

  private:
    dart_taskref_t _ref   = DART_TASK_NULL;
    bool           _ready = false;
  };

  /**
   * A class representing a task dependency.
   */
  class TaskDependency {
  public:

    /**
     * Create an empty dependency that will be ignored.
     */
    constexpr
    TaskDependency() { }

    /**
     * Create a data dependency using a DART global pointer.
     */
    TaskDependency(
      dart_gptr_t         gptr,
      dart_task_deptype_t type,
      dart_taskphase_t    phase = DART_PHASE_TASK)
      : _dep({{gptr}, type, phase})
    { }

    /**
     * Create a data dependency using a local pointer.
     */
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

    /**
     * Create a copy-in dependency using a DART global pointer.
     */
    TaskDependency(
      dart_gptr_t         gptr,
      size_t              num_bytes,
      void               *ptr,
      dart_taskphase_t    phase = DART_PHASE_TASK)
    {
      _dep.copyin.gptr = gptr;
      _dep.copyin.dest = ptr;
      _dep.copyin.size = num_bytes;
      _dep.type  = DART_DEP_COPYIN;
      _dep.phase = phase;
    }

    /**
     * Create a direct task dependency using a \ref TaskHandle created
     * previously.
     */
    template<typename T>
    TaskDependency(const TaskHandle<T>& handle) {
      _dep.task  = handle.dart_handle();
      _dep.type  = DART_DEP_DIRECT;
    }

    /**
     * Return the underlying DART dependency descriptor.
     */
    constexpr
    operator dart_task_dep_t() const {
      return _dep;
    }

  protected:
    dart_task_dep_t _dep = {{DART_GPTR_NULL}, DART_DEP_IGNORE, DART_PHASE_ANY};
  };

  /**
   * Create an input dependency using the global memory reference \c globref.
   *
   * \sa TaskDependency
   */
  template<typename GlobRefT>
  auto
  in(GlobRefT&& globref, int32_t phase = DART_PHASE_TASK)
    -> decltype((void)(globref.dart_gptr()), TaskDependency()) {
    return TaskDependency(globref.dart_gptr(), DART_DEP_IN, phase);
  }

  /**
   * Create an input dependency using the global memory range \c globref.
   *
   * \sa TaskDependency
   */
  template<typename RangeT>
  auto
  in(RangeT&& range, int32_t phase = DART_PHASE_TASK)
    -> decltype((void)(range.begin().dart_gptr()), TaskDependency()) {
    return TaskDependency(range.begin().dart_gptr(), DART_DEP_IN, phase);
  }


  /**
   * Create an input dependency using the local memory pointer \c lptr.
   *
   * \sa TaskDependency
   */
  template<typename T>
  TaskDependency
  in(const T* lptr, int32_t phase = DART_PHASE_TASK) {
    return TaskDependency(lptr, DART_DEP_IN, phase);
  }

  /**
   * Create an input dependency using the local memory pointer \c lptr.
   *
   * \sa TaskDependency
   */
  template<typename T>
  constexpr
  auto
  in(T& lref, int32_t phase = DART_PHASE_TASK)
    // exclude range types covered above
    -> typename std::enable_if<!std::is_function<decltype(T::begin)>::value,
                               TaskDependency>::type
  {
    return dash::tasks::in(const_cast<const T*>(&lref), phase);
  }

  /**
   * Create a copyin dependency using the global memory range starting at
   * \c globref.
   *
   * The data in \c range will be copied into \c target. Multiple tasks with
   * similar copyin dependencies in the same phase will use the same copy.
   * Only consecutive global memory ranges on a single unit can be used.
   *
   * \note currently, copyin dependencies are identified using their target. It
   *       is thus erroneous to mix output and copyin dependencies on \c target.
   * \note only  copyin dependencies in the same phase share a copy. If a
   *       copy created in one phase is also valid in subsequent phases you
   *       may explicitely specify the phase a copyin dependency refers to.
   * \sa TaskDependency
   */
  template<typename GlobRefT, typename ValueT = typename GlobRefT::value_type>
  auto
  copyin(
    GlobRefT&& globref,
    size_t     nelem,
    ValueT   * target,
    int32_t    phase = DART_PHASE_TASK)
    -> decltype((void)(globref.dart_gptr()), TaskDependency()) {
    return TaskDependency(globref.dart_gptr(), nelem*sizeof(ValueT),
                          target, phase);
  }

  /**
   * Create a copyin dependency using the global memory range [begin, end).
   *
   * The data in \c range will be copied into \c target. Multiple tasks with
   * similar copyin dependencies in the same phase will use the same copy.
   * Only consecutive global memory ranges on a single unit can be used.
   *
   * \note currently, copyin dependencies are identified using their target. It
   *       is thus erroneous to mix output and copyin dependencies on \c target.
   * \note only  copyin dependencies in the same phase share a copy. If a
   *       copy created in one phase is also valid in subsequent phases you
   *       may explicitely specify the phase a copyin dependency refers to.
   *
   * \sa TaskDependency
   */
  template<typename IterT, typename ValueT = typename IterT::value_type>
  auto
  copyin(
    IterT&&   begin,
    IterT&&   end,
    ValueT  * target,
    int32_t   phase = DART_PHASE_TASK)
    -> decltype((void)(begin.dart_gptr()), TaskDependency()) {
#if defined(DASH_DEBUG)
    auto pattern = begin.pattern();
    auto g_begin = begin.global();
    auto u_begin = pattern.unit_at(g_begin.pos());
    auto g_end   = end.global();
    auto u_end   = pattern.unit_at(g_end.pos());
    if (u_begin != u_end) {
      DASH_LOG_ERROR("Cannot copy-in across unit boundaries: begin %d, end %d",
                     u_begin, u_end);
    }
#endif // DASH_DEBUG
    return TaskDependency(begin.dart_gptr(),
                          dash::distance(begin, end)*sizeof(ValueT),
                          target, phase);
  }


  /**
   * Create a copyin dependency using the global memory range starting at
   * \c globref.
   *
   * The data in \c range will be copied into \c target if the range is remote.
   * Multiple tasks with similar copyin dependencies in the same phase will use
   * the same copy.
   * Only consecutive global memory ranges on a single unit can be used.
   *
   * If the range is local, the dependency will be in input dependency. It is
   * left to the user to handle the buffer correctly.
   *
   * \note currently, copyin dependencies are identified using their target. It
   *       is thus erroneous to mix output and copyin dependencies on \c target.
   * \note only  copyin dependencies in the same phase share a copy. If a
   *       copy created in one phase is also valid in subsequent phases you
   *       may explicitely specify the phase a copyin dependency refers to.
   * \sa TaskDependency
   */
  template<typename GlobRefT, typename ValueT = typename GlobRefT::value_type>
  auto
  copyin_r(
    GlobRefT&& globref,
    size_t     nelem,
    ValueT   * target,
    int32_t    phase = DART_PHASE_TASK)
    -> decltype((void)(globref.dart_gptr()), TaskDependency()) {
    if (globref.is_local()) {
      return TaskDependency(globref.dart_gptr(), DART_DEP_IN, phase);
    }
    return TaskDependency(globref.dart_gptr(), nelem*sizeof(ValueT),
                          target, phase);
  }

  /**
   * Create a copyin dependency using the global memory range [begin, end).
   *
   * The data in \c range will be copied into \c target if the range is remote.
   * Multiple tasks with similar copyin dependencies in the same phase will use
   * the same copy.
   * Only consecutive global memory ranges on a single unit can be used.
   *
   * If the range is local, the dependency will be in input dependency. It is
   * left to the user to handle the buffer correctly.
   *
   * \note currently, copyin dependencies are identified using their target. It
   *       is thus erroneous to mix output and copyin dependencies on \c target.
   * \note only  copyin dependencies in the same phase share a copy. If a
   *       copy created in one phase is also valid in subsequent phases you
   *       may explicitely specify the phase a copyin dependency refers to.
   *
   * \sa TaskDependency
   */
  template<typename IterT, typename ValueT = typename IterT::value_type>
  auto
  copyin_r(
    IterT&&   begin,
    IterT&&   end,
    ValueT  * target,
    int32_t   phase = DART_PHASE_TASK)
    -> decltype((void)(begin.dart_gptr()), TaskDependency()) {
#if defined(DASH_DEBUG)
    auto pattern = begin.pattern();
    auto g_begin = begin.global();
    auto u_begin = pattern.unit_at(g_begin.pos());
    auto g_end   = end.global();
    auto u_end   = pattern.unit_at(g_end.pos());
    if (u_begin != u_end) {
      DASH_LOG_ERROR("Cannot copy-in across unit boundaries: begin %d, end %d",
                     u_begin, u_end);
    }
#endif // DASH_DEBUG
    if (begin.is_local()) {
      return TaskDependency(begin.dart_gptr(), DART_DEP_IN, phase);
    }
    return TaskDependency(begin.dart_gptr(),
                          dash::distance(begin, end)*sizeof(ValueT),
                          target, phase);
  }

  /**
   * Create an output dependency using the global memory reference \c globref.
   *
   * \sa TaskDependency
   */
  template<typename GlobRefT>
  auto
  out(GlobRefT&& globref, int32_t phase = DART_PHASE_TASK)
    -> decltype((void)(globref.dart_gptr()), TaskDependency())  {
    return TaskDependency(globref.dart_gptr(), DART_DEP_OUT, phase);
  }

  /**
   * Create an output dependency using the global memory range \c globref.
   *
   * The first element in the range is used as a sentinel, i.e., no sub-range
   * matching is performed.
   *
   * \sa TaskDependency
   */
  template<typename RangeT>
  auto
  out(RangeT&& range, int32_t phase = DART_PHASE_TASK)
    -> decltype((void)(range.begin().dart_gptr()), TaskDependency()) {
    return TaskDependency(range.begin().dart_gptr(), DART_DEP_OUT, phase);
  }

  /**
   * Create an output dependency using the local memory pointer \c lptr.
   *
   * \sa TaskDependency
   */
  template<typename T>
  TaskDependency
  out(const T* lptr, int32_t phase = DART_PHASE_TASK) {
    return TaskDependency(lptr, DART_DEP_OUT, phase);
  }

  /**
   * Create an output dependency using the local memory pointer \c lptr.
   *
   * \sa TaskDependency
   */
  template<typename T>
  constexpr
  auto
  out(T& lref, int32_t phase = DART_PHASE_TASK)
    // exclude range types covered above
    -> typename std::enable_if<!std::is_function<decltype(T::begin)>::value,
                               TaskDependency>::type {
    return dash::tasks::out(const_cast<const T*>(&lref), phase);
  }


  /**
   * Create a direct dependency to the task referenced by \c TaskHandle.
   *
   * \sa TaskDependency
   */
  template<typename T>
  TaskDependency
  direct(const TaskHandle<T>& taskref) {
    return TaskDependency(taskref);
  }

  /**
   * Create an empty dependency that will be ignored.
   *
   * \sa TaskDependency
   */
  template<typename T=int>
  constexpr
  TaskDependency
  none() {
    return TaskDependency();
  }

  /**
   * Abort the execution of the current task.
   *
   * This function does not return.
   */
  template<typename T=int>
  [[noreturn]]
  void
  abort_task() {
    throw(dash::tasks::internal::AbortCancellationSignal());
  }

  /**
   * Abort the execution of the current and all remaining tasks and wait for
   * all other units to abort.
   *
   * This function does not return.
   */
  template<typename T=int>
  [[noreturn]]
  void
  cancel_barrier() {
    // check to avoid double abort
    if (dart_task_should_abort()) abort_task();
    throw(dash::tasks::internal::BarrierCancellationSignal());
  }

  /**
   * Abort the execution of the current all remaining tasks and broadcast a
   * cancellation request to all other units.
   *
   * This function does not return.
   */
  template<typename T=int>
  [[noreturn]]
  void
  cancel_bcast() {
    // check to avoid double abort
    if (dart_task_should_abort()) abort_task();
    throw(dash::tasks::internal::BcastCancellationSignal());
  }

namespace internal{
  template<class TaskFunc, typename DepContainer>
  void
  async(
    TaskFunc            f,
    dart_task_prio_t    prio,
    DepContainer&&      deps,
    const char         *name = nullptr) {
    if (dart_task_should_abort()) abort_task();
    if (std::is_trivially_copyable<TaskFunc>::value) {
      dart_task_create(
        &dash::tasks::internal::invoke_task_action_void<TaskFunc>,
        &f, sizeof(f), deps.data(), deps.size(), prio, name);
    } else {
      dart_task_create(
        &dash::tasks::internal::invoke_task_action<void>,
        new dash::tasks::internal::TaskData<void>(f), 0,
                      deps.data(), deps.size(), prio, name);
    }
  }

  template<class TaskFunc, typename DepContainer>
  void
  async(
    TaskFunc            f,
    DepContainer&&      deps,
    const char         *name = nullptr) {
    internal::async(f, DART_PRIO_LOW, std::forward<DepContainer>(deps), name);
  }
} // namespace internal

  /**
   * Create an asynchronous task that will execute \c f with priority \c prio
   * without any dependencies.
   *
   * \note This function is a cancellation point.
   */
  template<class TaskFunc>
  void
  async(
    TaskFunc         f,
    dart_task_prio_t prio){
    std::array<dart_task_dep_t, 0> deps;
    internal::async(f, prio, deps);
  }

  /**
   * Create an asynchronous task that will execute \c f with priority \c prio
   * after all specified dependencies have been satisfied.
   *
   * \note This function is a cancellation point.
   */
  template<class TaskFunc, typename ... Args>
  void
  async(
    TaskFunc                f,
    dart_task_prio_t        prio,
    TaskDependency          dep,
    Args&&...               args){
    std::array<dart_task_dep_t, sizeof...(args)+1> deps(
    {{
      static_cast<dart_task_dep_t>(dep),
      static_cast<dart_task_dep_t>(args)...
    }});
    internal::async(f, prio, deps);
  }


  /**
   * Create an asynchronous task that will execute \c f with priority \c prio
   * after all specified dependencies have been satisfied.
   *
   * \note This function is a cancellation point.
   */
  template<class TaskFunc, typename DependencyGeneratorFunc>
  void
  async(
    TaskFunc                  f,
    dart_task_prio_t          prio,
    DependencyGeneratorFunc   dependency_generator)
  {
    DependencyVector deps;
    dependency_generator(std::inserter(deps, deps.begin()));
    internal::async(f, prio, deps);
  }

  /**
   * Create an asynchronous task that will execute \c f with normal priority
   * after all dependencies specified in \c deps have been satisfied.
   *
   * \note This function is a cancellation point.
   */
  template<class TaskFunc, typename ... Args>
  void
  async(TaskFunc f, Args&&... args){
    async(f, DART_PRIO_LOW, std::forward<Args>(args)...);
  }


  /**
   * Create an asynchronous task that will execute \c f with priority \c prio
   * without any dependencies.
   *
   * \note This function is a cancellation point.
   */
  template<class TaskFunc>
  void
  async(
    const char*      name,
    TaskFunc         f,
    dart_task_prio_t prio){
    std::array<dart_task_dep_t, 0> deps;
    internal::async(f, prio, deps, name);
  }

  /**
   * Create an asynchronous task that will execute \c f with priority \c prio
   * after all specified dependencies have been satisfied.
   *
   * \note This function is a cancellation point.
   */
  template<class TaskFunc, typename ... Args>
  void
  async(
    const char*             name,
    TaskFunc                f,
    dart_task_prio_t        prio,
    TaskDependency          dep,
    Args&&...               args){
    std::array<dart_task_dep_t, sizeof...(args)+1> deps(
    {{
      static_cast<dart_task_dep_t>(dep),
      static_cast<dart_task_dep_t>(args)...
    }});
    internal::async(f, prio, deps, name);
  }


  /**
   * Create an asynchronous task that will execute \c f with priority \c prio
   * after all specified dependencies have been satisfied.
   *
   * \note This function is a cancellation point.
   */
  template<class TaskFunc, typename DependencyGeneratorFunc>
  void
  async(
    const char*               name,
    TaskFunc                  f,
    dart_task_prio_t          prio,
    DependencyGeneratorFunc   dependency_generator)
  {
    DependencyVector deps;
    dependency_generator(std::inserter(deps, deps.begin()));
    internal::async(f, prio, deps, name);
  }

  /**
   * Create an asynchronous task that will execute \c f with normal priority
   * after all dependencies specified in \c deps have been satisfied.
   *
   * \note This function is a cancellation point.
   */
  template<class TaskFunc, typename ... Args>
  void
  async(
    const char* name,
    TaskFunc    f,
    Args&&...   args){
    async(name, f, DART_PRIO_LOW, std::forward<Args>(args)...);
  }

#define SLOC_(__file, __delim, __line) __file # __delim # __line
#define SLOC(__file, __line)  SLOC_(__file, :, __line)
#define ASYNC(...) async(SLOC(__FILE__, __LINE__), __VA_ARGS__)

  /**
   * Return a handle to an asynchronous task that will execute \c f with
   * priority \c prio after all dependencies specified in \c deps have been
   * satisfied.
   *
   * \note This function is a cancellation point.
   *
   * \sa TaskHandle
   */
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

  /**
   * Return a handle to an asynchronous task that will execute \c f with
   * priority \c prio.
   *
   * \note This function is a cancellation point.
   *
   * \sa TaskHandle
   */
  template<class TaskFunc>
  auto
  async_handle(
    TaskFunc f,
    dart_task_prio_t prio) -> TaskHandle<decltype(f())>
  {
    std::array<dart_task_dep_t, 0> deps;
    return async_handle(f, prio, deps);
  }

  /**
   * Return a handle to an asynchronous task that will execute \c f with
   * priority \c prio after all specified dependencies have been
   * satisfied.
   *
   * \note This function is a cancellation point.
   *
   * \sa TaskHandle
   */
  template<class TaskFunc, typename ... Args>
  auto
  async_handle(
    TaskFunc f,
    dart_task_prio_t prio,
    TaskDependency   dep,
    Args&&...        args) -> TaskHandle<decltype(f())>
  {
    std::array<dart_task_dep_t, sizeof...(args)+1> deps(
    {{
      static_cast<dart_task_dep_t>(dep),
      static_cast<dart_task_dep_t>(args)...
    }});
    return async_handle(f, prio, deps);
  }

  /**
   * Return a handle to an asynchronous task that will execute \c f with normal
   * priority after all specified dependencies have been satisfied.
   *
   * \sa TaskHandle
   */
  template<class TaskFunc, typename ... Args>
  auto
  async_handle(
    TaskFunc  f,
    Args&&... args) -> TaskHandle<decltype(f())>
  {
    return async_handle(f, DART_PRIO_LOW, std::forward<Args>(args)...);
  }

  /**
   * Return a handle to an asynchronous task that will execute \c f with normal
   * priority after all specified dependencies have been satisfied.
   *
   * \sa TaskHandle
   */
  template<class TaskFunc, typename DepGeneratorFunc>
  auto
  async_handle(
    TaskFunc             f,
    DependencyGenerator  dependency_generator,
    dart_task_prio_t     prio = DART_PRIO_LOW) -> TaskHandle<decltype(f())>
  {
    DependencyVector deps;
    dependency_generator(std::inserter(deps, deps.begin()));
    return async_handle(f, prio, deps);
  }

  /**
   * Perform an asynchronous barrier to signal the completion of the current
   * task execution phase.
   *
   * This operation does not block but should be called on all units
   * synchronously, i.e., all units should perform the same number of
   * \c async_barrier calls to phases synchronized.
   * This function should only be called on the main task and will have no
   * effect in any other task.
   */
  template<typename T=int>
  void async_barrier() {
    dart_task_phase_advance();
  }

  template<typename T=int>
  void resync_barrier(dash::Team& team = dash::Team::All()) {
    dart_task_phase_resync(team.dart_id());
  }

  /**
   * Yield the current thread in order to execute another task.
   *
   * \note This function is a cancellation point.
   */
  template<typename T=int>
  void
  yield(int delay = -1) {
    if (dart_task_should_abort()) abort_task();
    dart_task_yield(delay);
  }

  /**
   * Wait for the execution of all previously created tasks to complete.
   *
   * \note This function is a cancellation point.
   */
  template<typename T=int>
  void
  complete() {
    if (dart_task_should_abort()) abort_task();
    dart_task_complete();
  }

  template<typename T=int>
  size_t
  numthreads() {
    return dart_task_num_threads ? dart_task_num_threads() : 1;
  }

  template<typename T=int>
  size_t
  threadnum() {
    return dart_task_thread_num ? dart_task_thread_num() : 0;
  }

} // namespace tasks

} // namespace dash

#endif // DASH__TASKS_TASKS_H__
