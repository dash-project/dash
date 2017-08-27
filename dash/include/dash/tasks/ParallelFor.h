#ifndef DASH__PARALLEL__PARALLELFOR_H__
#define DASH__PARALLEL__PARALLELFOR_H__



namespace dash{

  namespace internal {

    using FuncT = std::function<void()>;

    //template<typename FuncT>
    static void invoke_task_action(void *data)
    {
      FuncT *func = static_cast<FuncT*>(data);
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
  create_task(TaskFunc f, dart_task_prio_t prio, const DepContainer& deps){
    dart_task_create(
      &dash::internal::invoke_task_action,
      new dash::internal::FuncT(f), 0,
                     deps.data(), deps.size(), prio);
  }


  template<class TaskFunc, typename ... Args>
  void
  create_task(TaskFunc f, dart_task_prio_t prio, const Args&... args){
    std::array<dart_task_dep_t, sizeof...(args)> deps({{
      static_cast<dart_task_dep_t>(args)...
    }});
    dash::create_task(f, prio, deps);
  }

  template<class TaskFunc, typename ... Args>
  void
  create_task(TaskFunc f, const Args&... args){
    create_task(f, DART_PRIO_LOW, args...);
  }


  template<class TaskFunc, typename ... Args>
  dart_taskref_t
  create_task_handle(TaskFunc f, dart_task_prio_t prio, const Args&... args){
    std::array<dart_task_dep_t, sizeof...(args)> deps({{
      static_cast<dart_task_dep_t>(args)...
    }});
    dart_task_create(
      &dash::internal::invoke_task_action,
      new dash::internal::FuncT(f), 0,
                     deps.data(), deps.size(), prio);
  }

  template<class TaskFunc, typename ... Args>
  dart_taskref_t
  create_task_handle(TaskFunc f, const Args&... args){
    return create_task_handle(f, DART_PRIO_LOW, args...);
  }

  /**
   * Create a bunch of tasks operating on the input range but do not wait
   * for their completion.
   * TODO: Add launch policies here!
   */
  template<class InputIter, typename RangeFunc>
  void
  parallel_for(
    InputIter begin,
    InputIter end,
    size_t chunk_size,
    RangeFunc f)
  {
    if (chunk_size == 0) chunk_size = 1;
    InputIter from = begin;
    while (from < end) {
      InputIter to = from + chunk_size;
      if (to > end) to = end;
      dash::create_task(
        [=](){
          f(from, to);
        }
      );
      from = to;
    }
  }

  using DependencyVector = std::vector<dart_task_dep_t>;
  using DependencyVectorInserter = std::insert_iterator<DependencyVector>;

  template<class InputIter, typename RangeFunc, typename DepGeneratorFunc>
  void
  parallel_for(
    InputIter begin,
    InputIter end,
    size_t chunk_size,
    RangeFunc f,
    DepGeneratorFunc df)
  {
    if (chunk_size == 0) chunk_size = 1;
    InputIter from = begin;
    while (from < end) {
      InputIter to = from + chunk_size;
      if (to > end) to = end;
      DependencyVector deps;
      deps.reserve(10);
      df(from, to, std::inserter(deps, deps.begin()));
      dash::create_task(
        [=](){
          f(from, to);
        },
        deps
      );
      from = to;
    }
  }

  template<class InputIter, typename RangeFunc>
  void
  parallel_for(
    InputIter begin,
    InputIter end,
    RangeFunc f)
  {
    parallel_for(begin, end, 1, f);
  }

  void
  yield(int delay) {
    dart_task_yield(delay);
  }

} // namespace dash

#endif // DASH__PARALLEL__PARALLELFOR_H__
