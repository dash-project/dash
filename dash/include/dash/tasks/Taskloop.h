#ifndef DASH__TASKS__PARALLELFOR_H__
#define DASH__TASKS__PARALLELFOR_H__

#include <dash/tasks/Tasks.h>
#include <dash/dart/if/dart_tasking.h>


namespace dash{
namespace tasks{

  /**
   * Create a bunch of tasks operating on the input range but do not wait
   * for their completion.
   * TODO: Add launch policies here!
   */
  template<class InputIter, typename RangeFunc>
  void
  taskloop(
    InputIter begin,
    InputIter end,
    size_t chunk_size,
    RangeFunc f)
  {
    // TODO: extend this to handle GlobIter!
    if (chunk_size == 0) chunk_size = 1;
    InputIter from = begin;
    while (from < end) {
      InputIter to = from + chunk_size;
      if (to > end) to = end;
      dash::tasks::async(
        [=](){
          f(from, to);
        }
      );
      from = to;
    }
  }

  template<class InputIter, typename RangeFunc, typename DepGeneratorFunc>
  void
  taskloop(
    InputIter begin,
    InputIter end,
    size_t chunk_size,
    RangeFunc f,
    DepGeneratorFunc depedency_generator)
  {
    // TODO: extend this to handle GlobIter!
    if (chunk_size == 0) chunk_size = 1;
    InputIter from = begin;
    DependencyVector deps;
    deps.reserve(10);
    while (from < end) {
      InputIter to = from + chunk_size;
      if (to > end) to = end;
      auto dep_inserter = std::inserter(deps, deps.begin());
      depedency_generator(from, to, dep_inserter);
      dash::tasks::internal::async(
        [=](){
          f(from, to);
        },
        deps
      );
      from = to;
      deps.clear();
    }
  }

  template<class InputIter, typename RangeFunc>
  void
  taskloop(
    InputIter begin,
    InputIter end,
    RangeFunc f)
  {
    taskloop(begin, end,
                 dash::distance(begin, end) / dart_task_num_threads(), f);
  }

  template<class InputIter, typename RangeFunc, typename DepGeneratorFunc>
  void
  taskloop(
    InputIter        begin,
    InputIter        end,
    RangeFunc        f,
    DepGeneratorFunc depedency_generator)
  {
    taskloop(begin, end, dash::distance(begin, end) / dart_task_num_threads(),
             f, depedency_generator);
  }
} // namespace tasks
} // namespace dash

#endif // DASH__TASKS__PARALLELFOR_H__
