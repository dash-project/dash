#ifndef DASH__TASKS_TASKS_H__
#define DASH__TASKS_TASKS_H__

#include <dash/internal/Macro.h>
#include <dash/Exception.h>

#include <dash/dart/if/dart_tasking.h>

#include <dash/tasks/internal/DependencyContainer.h>
#include <dash/tasks/internal/TaskHandle.h>
#include <dash/tasks/internal/LambdaTraits.h>

#include <utility>

// Enable for debugging to directly invoke the tasks, ignoring any dependencies
// NOTE: this won't enqueue any tasks so all but the main thread stay idle
#define DASH_TASKS_INVOKE_DIRECT 0

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



  /**
   * Emulate a final statement,
   * taken from https://github.com/Microsoft/GSL/blob/master/include/gsl/gsl_util
   */
  template <class F>
  class final_act
  {
  public:
      explicit final_act(F f) noexcept
        : f_(std::move(f)), invoke_(true) {}

      final_act(final_act&& other) noexcept
      : f_(std::move(other.f_)),
        invoke_(other.invoke_)
      {
          other.invoke_ = false;
      }

      final_act(const final_act&) = delete;
      final_act& operator=(const final_act&) = delete;

      ~final_act() noexcept
      {
          if (invoke_) f_();
      }

  private:
      F f_;
      bool invoke_;
  };

  template <class F>
  inline final_act<F> finally(const F& f) noexcept
  {
      return final_act<F>(f);
  }

  template <class F>
  inline final_act<F> finally(F&& f) noexcept
  {
      return final_act<F>(std::forward<F>(f));
  }



  template<typename ReturnT = void>
  struct TaskData {

    TaskData(task_action_t<ReturnT> func) : func(func) { }
    TaskData(
      task_action_t<ReturnT> func,
      std::shared_ptr<ReturnT> result) : func(func), result(result) { }

    task_action_t<ReturnT>   func;
    std::shared_ptr<ReturnT> result = nullptr;
  };

  template<typename FuncT, typename TupleT, std::size_t... I>
  void
  invoke_function_with_params(FuncT& func, TupleT, std::index_sequence<I...>)
  {
    int i = 0;
    auto task = dart_task_current_task();
    func( (typename std::tuple_element<I, TupleT>::type::value_type*)
                                          dart_task_copyin_info(task, i++)...);
  }

  template <bool, typename>
  struct pick;

  template <typename T>
  struct pick<true, T> { using type = std::tuple<T>; };

  template <typename T>
  struct pick<false, T> { using type = std::tuple<>; };

  template<typename ...DepTypes>
  struct copyin_tuple_filter
  {
    using tuple_type = decltype(
                        std::tuple_cat(
                          typename pick<DepTypes::is_copyin, DepTypes>::type{}...));
  };

  template<typename ReturnT, typename ... DepTypes>
  typename std::enable_if<!std::is_same<ReturnT, void>::value, void>::type
  invoke_task_action_impl(TaskData<ReturnT>& data)
  {
    task_action_t<ReturnT>& func = data.func;
    assert(data.result != NULL);
    if (data.result != NULL) {
      *data.result = func();
    } else {
      func();
    }
  }

  template<typename T, typename ... DepTypes>
  typename std::enable_if<std::is_same<T, void>::value, void>::type
  invoke_task_action_impl(TaskData<T>& data)
  {
    task_action_t<T>& func = data.func;
    using tuple_type = typename copyin_tuple_filter<DepTypes...>::tuple_type;
    constexpr const int tuple_size = std::tuple_size<tuple_type>::value;
    constexpr const int num_args = std::min(tuple_size,
                                            lambda_traits<decltype(func)>::num_args);
    //tuple_type deps;
    //std::tuple<typename DepTypes::value_type*...> deps;
    //extract_depinfo<0, decltype(deps), DepTypes...>(dart_task_current_task(), deps);
    invoke_function_with_params(func, tuple_type{}, std::make_index_sequence<num_args>{});
  }

  template<typename T=void, typename ... DepTypes>
  void
  invoke_task_action(void *data)
  {
    try {
      TaskData<T> *td = static_cast<TaskData<T>*>(data);
      invoke_task_action_impl<T, DepTypes...>(*td);
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

  template<typename FuncT, typename ... DepTypes>
  void
  invoke_task_action_void(void *data)
  {
    try{
      FuncT& func = *static_cast<FuncT*>(data);
      // at the end we have to call the destructor on the function object,
      // the memory will be free'd by the runtime
      auto _ = internal::finally([&](){func.~FuncT();});
      using tuple_type = typename copyin_tuple_filter<DepTypes...>::tuple_type;
      constexpr const int tuple_size = std::tuple_size<tuple_type>::value;
      constexpr const int num_args = std::min(tuple_size, lambda_traits<FuncT>::num_args);
      //tuple_type deps;
      //extract_depinfo<0, decltype(deps), DepTypes...>(dart_task_current_task(), deps);
      //invoke_function_with_params(f, deps, typename gens<std::tuple_size<tuple_type>::value>::type());

      invoke_function_with_params(func, tuple_type{}, std::make_index_sequence<num_args>{});
    } catch (const BaseCancellationSignal& cs) {
      // nothing to be done, the cancellation is triggered by the d'tor
    } catch (...) {
      DASH_LOG_ERROR(
        "An unhandled exception is escaping one of your tasks, "
        "your application will thus abort.");
      throw;
    }
  }

  template<typename FuncT, typename ... DepTypes>
  void
  invoke_task_action_void_delete(void *data)
  {
    try{
      FuncT& func = *static_cast<FuncT*>(data);
      // at the end we have to call the destructor on the function object and
      // free its memory explicitely
      auto _ = internal::finally([&](){delete &func;});
      using tuple_type = typename copyin_tuple_filter<DepTypes...>::tuple_type;
      constexpr const int tuple_size = std::tuple_size<tuple_type>::value;
      constexpr const int num_args = std::min(tuple_size, lambda_traits<FuncT>::num_args);
      //extract_depinfo<0, decltype(deps), DepTypes...>(dart_task_current_task(), deps);
      invoke_function_with_params(func, tuple_type{}, std::make_index_sequence<num_args>{});
    } catch (const BaseCancellationSignal& cs) {
      // nothing to be done, the cancellation is triggered by the d'tor
    } catch (...) {
      DASH_LOG_ERROR(
        "An unhandled exception is escaping one of your tasks, "
        "your application will thus abort.");
      throw;
    }
  }

  template<typename T>
  struct is_range
  {
  private:
    // fall-back
    template<typename>
    static constexpr std::false_type test(...);

    // test for T::begin
    template<typename U=T>
    static decltype(std::begin(std::declval<U>()), std::true_type{}) test(int);

  public:
    static constexpr bool value = decltype(test<T>(0))::value;
  };

  template<typename T>
  struct has_gptr
  {
  private:
    // fall-back
    template<typename>
    static constexpr std::false_type test(...);

    // test for T::begin
    template<typename U=T>
    static decltype((std::declval<U>()).dart_gptr(), std::true_type{}) test(int);

  public:
    static constexpr bool value = decltype(test<T>(0))::value;
  };

} // namespace internal


  using DependencyContainerInserter
    = std::insert_iterator<typename dash::tasks::internal::DependencyContainer>;
  using DependencyGenerator = std::function<void(DependencyContainerInserter)>;

  /**
   * A class representing a task dependency.
   */
  template<typename ValueType, bool IsCopyin = false>
  class TaskDependency {
  public:

    using value_type = ValueType;
    static constexpr const bool is_copyin  = IsCopyin;

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
    TaskDependency(
      const ValueType     * lptr,
      dart_task_deptype_t   type,
      dart_taskphase_t      phase = DART_PHASE_TASK) {
      _dep.gptr  = DART_GPTR_NULL;
      _dep.gptr.addr_or_offs.addr = const_cast<ValueType*>(lptr);
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
      dart_task_deptype_t type,
      dart_taskphase_t    phase = DART_PHASE_TASK)
    {
      _dep.copyin.gptr = gptr;
      _dep.copyin.dest = ptr;
      _dep.copyin.size = num_bytes;
      _dep.type  = type;
      _dep.phase = phase;
    }

    /**
     * Create a direct task dependency using a \ref TaskHandle created
     * previously.
     */
    TaskDependency(const TaskHandle<ValueType>& handle) {
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
  in(const GlobRefT& globref, int32_t phase = DART_PHASE_TASK)
    -> decltype((void)(globref.dart_gptr()), TaskDependency<typename GlobRefT::value_type>()) {
    return TaskDependency<typename GlobRefT::value_type, false>(globref.dart_gptr(), DART_DEP_IN, phase);
  }

#if 0
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
#endif

  /**
   * Create an input dependency using the local memory reference \c lptr.
   *
   * \note The value referenced by \c lref is never accessed but it is non-const
   *       to prohibit temporary values.
   *
   * \sa TaskDependency
   */
  template<typename T>
  constexpr
  auto
  in(T& lref, int32_t phase = DART_PHASE_TASK)
    // exclude range types covered above
    -> typename std::enable_if<!(internal::has_gptr<T>::value),
                               TaskDependency<T, false>>::type
  {
    return TaskDependency<T, false>(&lref, DART_DEP_IN, phase);
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
    ValueT   * target = nullptr,
    int32_t    phase = DART_PHASE_TASK)
    -> decltype((void)(globref.dart_gptr()), TaskDependency<ValueT, true>()) {
    return TaskDependency<ValueT, true>(globref.dart_gptr(), nelem*sizeof(ValueT),
                          target, DART_DEP_COPYIN, phase);
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
    ValueT  * target = nullptr,
    int32_t   phase = DART_PHASE_TASK)
    -> decltype((void)(begin.dart_gptr()), TaskDependency<typename IterT::value_type, true>()) {
#if defined(DASH_DEBUG)
    auto pattern = begin.pattern();
    auto g_begin = begin.global();
    auto u_begin = pattern.unit_at(g_begin.pos());
    auto g_end   = end.global();
    auto u_end   = pattern.unit_at(g_end.pos() - 1);
    if (u_begin != u_end) {
      DASH_LOG_ERROR("dash::tasks::copyin",
                     "Cannot copy-in across unit boundaries: begin ",
                     u_begin, ", end ", u_end);
    }
#endif // DASH_DEBUG
    return TaskDependency<typename IterT::value_type, true>(
                          begin.dart_gptr(),
                          dash::distance(begin, end)*sizeof(ValueT),
                          target, DART_DEP_COPYIN, phase);
  }


  /**
   * Create a copyin dependency using the global memory \c range.
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
  template<typename RangeT, typename ValueT = typename RangeT::value_type>
  auto
  copyin(
    RangeT&&  range,
    ValueT  * target = nullptr,
    int32_t   phase = DART_PHASE_TASK)
    -> decltype((void)(range.begin()), (void)(range.end()),
                TaskDependency<typename RangeT::value_type, true>()) {
      return copyin(range.begin(), range.end(), target, phase);
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
    ValueT   * target = nullptr,
    int32_t    phase = DART_PHASE_TASK)
    -> decltype((void)(globref.dart_gptr()),
                  TaskDependency<typename GlobRefT::value_type, true>()) {
    return TaskDependency<typename GlobRefT::value_type, true>(
                          globref.dart_gptr(), nelem*sizeof(ValueT),
                          target, DART_DEP_COPYIN_R, phase);
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
    ValueT  * target = nullptr,
    int32_t   phase = DART_PHASE_TASK)
    -> decltype((void)(begin.dart_gptr()), TaskDependency<typename IterT::value_type, true>()) {
#if defined(DASH_DEBUG)
    auto pattern = begin.pattern();
    auto g_begin = begin.global();
    auto u_begin = pattern.unit_at(g_begin.pos());
    auto g_end   = end.global();
    auto u_end   = pattern.unit_at(g_end.pos() - 1);
    if (u_begin != u_end) {
      DASH_LOG_ERROR("dash::tasks::copyin",
                     "Cannot copy-in across unit boundaries: begin ",
                     u_begin, ", end ", u_end);
    }
#endif // DASH_DEBUG
    return TaskDependency<typename IterT::value_type, true>(
                          begin.dart_gptr(),
                          dash::distance(begin, end)*sizeof(ValueT),
                          target, DART_DEP_COPYIN_R, phase);
  }

  /**
   * Create a copyin dependency using the global memory \c range.
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
  template<typename RangeT, typename ValueT = typename RangeT::value_type>
  auto
  copyin_r(
    RangeT&&  range,
    ValueT  * target = nullptr,
    int32_t   phase = DART_PHASE_TASK)
    -> decltype((void)(range.begin()), (void)(range.end()),
                TaskDependency<typename RangeT::value_type, true>()) {
      return copyin_r(range.begin(), range.end(), target, phase);
  }

  /**
   * Create an output dependency using the global memory reference \c globref.
   *
   * \sa TaskDependency
   */
  template<typename GlobRefT>
  auto
  out(const GlobRefT& globref, int32_t phase = DART_PHASE_TASK)
    -> decltype((void)(globref.dart_gptr()), TaskDependency<typename GlobRefT::value_type>())  {
    return TaskDependency<typename GlobRefT::value_type>(globref.dart_gptr(), DART_DEP_OUT, phase);
  }

#if 0
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
#endif

  /**
   * Create an output dependency using the local memory pointer \c lptr.
   *
   * \note The value referenced by \c lref is never accessed but it is non-const
   *       to prohibit temporary values.
   *
   * \sa TaskDependency
   */
  template<typename T>
  constexpr
  auto
  out(T& lref, int32_t phase = DART_PHASE_TASK)
    // exclude range types covered above
    -> typename std::enable_if<!(internal::has_gptr<T>::value),
                               TaskDependency<T>>::type {
    return TaskDependency<T>(&lref, DART_DEP_OUT, phase);
  }


  /**
   * Create a direct dependency to the task referenced by \c TaskHandle.
   *
   * \sa TaskDependency
   */
  template<typename T>
  TaskDependency<T>
  direct(const TaskHandle<T>& taskref) {
    return TaskDependency<T>(taskref);
  }

  /**
   * Create an empty dependency that will be ignored.
   *
   * \sa TaskDependency
   */
  template<typename T=int>
  constexpr
  TaskDependency<T>
  none() {
    return TaskDependency<T>();
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
  template<class TaskFunc, typename DepContainer, typename ... DepTypes>
  void
  async(
    TaskFunc            f,
    dart_task_prio_t    prio,
    DepContainer&       deps,
    int                 flags = 0,
    const char         *name = nullptr) {
#if DASH_TASKS_INVOKE_DIRECT
    f();
#else // DASH_TASKS_INVOKE_DIRECT
    if (dart_task_should_abort()) abort_task();

    if (std::is_trivially_copyable<TaskFunc>::value &&
        sizeof(f) <= DART_TASKING_INLINE_DATA_SIZE) {
      // the function is small enough to fit into the task inline so don't
      // allocate extra memory
      dart_task_create(
        &dash::tasks::internal::invoke_task_action_void<TaskFunc, DepTypes...>,
        &f, sizeof(f), deps.data(), deps.size(), prio, flags, name);
    } else {
      dart_task_create(
        &dash::tasks::internal::invoke_task_action_void_delete<TaskFunc, DepTypes...>,
        new TaskFunc(std::move(f)), 0,
        deps.data(), deps.size(), prio, flags, name);
    }
#endif // DASH_TASKS_INVOKE_DIRECT
  }

  template<class TaskFunc, typename DepContainer>
  void
  async(
    TaskFunc            f,
    DepContainer&&      deps,
    int                 flags = 0,
    const char         *name = nullptr) {
    internal::async(f, DART_PRIO_PARENT, std::forward<DepContainer>(deps), flags, name);
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
  template<class TaskFunc, typename T, bool FirstIsCopyin, typename ... Args>
  void
  async(
    TaskFunc                f,
    dart_task_prio_t        prio,
    TaskDependency<T, FirstIsCopyin> dep,
    Args&&...               args) {
    std::array<dart_task_dep_t, sizeof...(args)+1> deps(
    {{
      static_cast<dart_task_dep_t>(dep),
      static_cast<dart_task_dep_t>(args)...
    }});
    internal::async<TaskFunc, decltype(deps), decltype(dep), Args...>(f, prio, deps);
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
    typename dash::tasks::internal::DependencyContainer deps;
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
    async(f, DART_PRIO_PARENT, std::forward<Args>(args)...);
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
    internal::async(f, prio, deps, 0, name);
  }

  /**
   * Create an asynchronous task that will execute \c f with priority \c prio
   * after all specified dependencies have been satisfied.
   *
   * \note This function is a cancellation point.
   */
  template<class TaskFunc, typename T, bool FirstIsCopyin, typename ... Args>
  void
  async(
    const char*             name,
    TaskFunc                f,
    dart_task_prio_t        prio,
    TaskDependency<T, FirstIsCopyin> dep,
    Args&&...               args){
    std::array<dart_task_dep_t, sizeof...(args)+1> deps(
    {{
      static_cast<dart_task_dep_t>(dep),
      static_cast<dart_task_dep_t>(args)...
    }});
    internal::async<TaskFunc, decltype(deps), decltype(dep), Args...>(f, prio, deps, 0, name);
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
    dash::tasks::internal::DependencyContainer deps;
    dependency_generator(std::inserter(deps, deps.begin()));
    internal::async(f, prio, deps, 0, name);
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
    async(name, f, DART_PRIO_PARENT, std::forward<Args>(args)...);
  }

#define SLOC_(__file, __delim, __line) __file # __delim # __line
#define SLOC(__file, __line)  SLOC_(__file, :, __line)
#define ASYNC(...) async(SLOC(__FILE__, __LINE__), __VA_ARGS__)

  /**
   * Create an asynchronous task that will execute \c f with priority \c prio
   * without any dependencies.
   *
   * \note This function is a cancellation point.
   */
  template<class TaskFunc>
  void
  tasklet(
    TaskFunc         f,
    dart_task_prio_t prio){
    std::array<dart_task_dep_t, 0> deps;
    internal::async(f, prio, deps, DART_TASK_NOYIELD);
  }

  /**
   * Create an asynchronous tasklet that will execute \c f with priority \c prio
   * after all specified dependencies have been satisfied.
   *
   * Tasklets are similar to tasks created using \sa async with the exception
   * that yield has no effect.
   *
   * \note This function is a cancellation point.
   */
  template<class TaskFunc, typename ... Args>
  void
  tasklet(
    TaskFunc                f,
    dart_task_prio_t        prio,
    TaskDependency          dep,
    Args&&...               args){
    std::array<dart_task_dep_t, sizeof...(args)+1> deps(
    {{
      static_cast<dart_task_dep_t>(dep),
      static_cast<dart_task_dep_t>(args)...
    }});
    internal::async(f, prio, deps, DART_TASK_NOYIELD);
  }


  /**
   * Create an asynchronous tasklet that will execute \c f with priority \c prio
   * after all specified dependencies have been satisfied.
   *
   * Tasklets are similar to tasks created using \sa async with the exception
   * that yield has no effect.
   *
   * \note This function is a cancellation point.
   */
  template<class TaskFunc, typename DependencyGeneratorFunc>
  void
  tasklet(
    TaskFunc                  f,
    dart_task_prio_t          prio,
    DependencyGeneratorFunc   dependency_generator)
  {
    typename dash::tasks::internal::DependencyContainer deps;
    dependency_generator(std::inserter(deps, deps.begin()));
    internal::async(f, prio, deps, DART_TASK_NOYIELD);
  }

  /**
   * Create an asynchronous tasklet that will execute \c f with normal priority
   * after all dependencies specified in \c deps have been satisfied.
   *
   * Tasklets are similar to tasks created using \sa async with the exception
   * that yield has no effect.
   *
   * \note This function is a cancellation point.
   */
  template<class TaskFunc, typename ... Args>
  void
  tasklet(TaskFunc f, Args&&... args){
    tasklet(f, DART_PRIO_PARENT, std::forward<Args>(args)...);
  }


  /**
   * Create an asynchronous tasklet that will execute \c f with priority \c prio
   * without any dependencies.
   *
   * Tasklets are similar to tasks created using \sa async with the exception
   * that yield has no effect.
   *
   * \note This function is a cancellation point.
   */
  template<class TaskFunc>
  void
  tasklet(
    const char*      name,
    TaskFunc         f,
    dart_task_prio_t prio){
    std::array<dart_task_dep_t, 0> deps;
    internal::async(f, prio, deps, DART_TASK_NOYIELD, name);
  }

  /**
   * Create an asynchronous tasklet that will execute \c f with priority \c prio
   * after all specified dependencies have been satisfied.
   *
   * Tasklets are similar to tasks created using \sa async with the exception
   * that yield has no effect.
   *
   * \note This function is a cancellation point.
   */
  template<class TaskFunc, typename ... Args>
  void
  tasklet(
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
    internal::async(f, prio, deps, DART_TASK_NOYIELD, name);
  }


  /**
   * Create an asynchronous tasklet that will execute \c f with priority \c prio
   * after all specified dependencies have been satisfied.
   *
   * Tasklets are similar to tasks created using \sa async with the exception
   * that yield has no effect.
   *
   * \note This function is a cancellation point.
   */
  template<class TaskFunc, typename DependencyGeneratorFunc>
  void
  tasklet(
    const char*               name,
    TaskFunc                  f,
    dart_task_prio_t          prio,
    DependencyGeneratorFunc   dependency_generator)
  {
    dash::tasks::internal::DependencyContainer deps;
    dependency_generator(std::inserter(deps, deps.begin()));
    internal::async(f, prio, deps, DART_TASK_NOYIELD, name);
  }

  /**
   * Create an asynchronous tasklet that will execute \c f with normal priority
   * after all dependencies specified in \c deps have been satisfied.
   *
   * Tasklets are similar to tasks created using \sa async with the exception
   * that yield has no effect.
   *
   * \note This function is a cancellation point.
   */
  template<class TaskFunc, typename ... Args>
  void
  tasklet(
    const char* name,
    TaskFunc    f,
    Args&&...   args){
    tasklet(name, f, DART_PRIO_PARENT, std::forward<Args>(args)...);
  }

#define TASKLET(...) tasklet(SLOC(__FILE__, __LINE__), __VA_ARGS__)


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
    DepContainer& deps) -> TaskHandle<decltype(f())>
  {
    using return_t = decltype(f());
    if (dart_task_should_abort()) abort_task();
    dart_taskref_t handle;
    std::shared_ptr<return_t> retval = std::make_shared<return_t>();
    dart_task_create_handle(
      &dash::tasks::internal::invoke_task_action<return_t>,
      new dash::tasks::internal::TaskData<return_t>(
            dash::tasks::internal::task_action_t<return_t>(std::move(f)), retval),
      0, deps.data(), deps.size(), prio, 0, &handle);
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
  template<class TaskFunc, typename T, typename ... Args>
  auto
  async_handle(
    TaskFunc f,
    dart_task_prio_t  prio,
    TaskDependency<T> dep,
    Args&&...         args) -> TaskHandle<decltype(f())>
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
    return async_handle(f, DART_PRIO_PARENT, std::forward<Args>(args)...);
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
    dart_task_prio_t     prio = DART_PRIO_PARENT) -> TaskHandle<decltype(f())>
  {
    dash::tasks::internal::DependencyContainer deps;
    dependency_generator(std::inserter(deps, deps.begin()));
    return async_handle(f, prio, deps);
  }

  /**
   * Perform an asynchronous barrier to signal the completion of the current
   * task execution phase.
   *
   * This operation does not block but should be called on all units
   * synchronously, i.e., all units should perform the same number of
   * \c async_fence calls to phases synchronized.
   * This function should only be called on the main task and will have no
   * effect in any other task.
   */
  template<typename T=int>
  void async_fence() {
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
   * If the optional parameter \c local_only is set to \c true then the unit
   * does attempt to handle remote dependencies and the call is not collective.
   * Otherwise, if the call is made from the \c root_task the call is collective
   * across all units.
   *
   * \note This function is a cancellation point.
   */
  template<typename T=int>
  void
  complete(bool local_only = false) {
    if (dart_task_should_abort()) abort_task();
    dart_task_complete(local_only);
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
