#ifndef DASH__CONCURRENCY_H_
#define DASH__CONCURRENCY_H_

#include <future>

// forward declarations needed because we cannot include libgomp headers atm
//extern "C" {
//void
//GOMP_task (void (*fn) (void *), void *data, void (*cpyfn) (void *, void *),
//           long arg_size, long arg_align, bool if_clause, unsigned flags,
//           void **depend, int priority);
//}
//
//namespace dash {
//
//namespace concurrency {
//
//template<typename Function>
//void invoke(void *ptr)
//{
//  Function *tf = static_cast<Function*>(ptr);
//  // invoke the function object
//  (*tf)();
//}
//
//template<typename Function>
//void task(Function f, bool enqueu = true) {
//
//  using FunctionPtr = void (*)(void*);
//  int depend[2] = {0, 0};
//
//  // works
//  GOMP_task(static_cast<FunctionPtr>(invoke<Function>), &(f), NULL, sizeof(&f), 8, enqueu, 0, (void**)depend, 0);
//  std::launch::async();
//  // works as well.
////#pragma omp task
////  invoke<Function>(tf);
//
//}

#include <dash/iterator/GlobIter.h>

#include <vector>

namespace dash {
namespace concurrency {

typedef enum DependencyType {
  DASH_IN,
  DASH_OUT,
  DASH_INOUT
} DependencyType ;

template<class GlobInputIt>
typedef struct DependencyRange {
  GlobInputIt    start;
  GlobInputIt    end;
  DependencyType type;
} DependencyRange;

template<typename T>
struct function_traits;

template<typename R, typename... ARGS>
struct function_traits<std::function<R(ARGS...)>>
{
    static const size_t nargs = sizeof...(ARGS);

    typedef R result_type;

    template <size_t i>
    struct arg
    {
        typedef typename std::tuple_element<i, std::tuple<ARGS...>>::type type;
    };
};


template<typename Function, class GlobInputIt>
void create_task(Function f, std::vector<DependencyRange<class GlobInputIt>>& deps)
{
  /**
   *  Check whether this task has remote dependencies (the range between start and end points to elements outside the local scope)
   *    1) For local elements: handle the dependecy locally
   *    2) For remote dependencies:
   *      a) On the remote unit (unit R), enqueue a task T_R1 to unit(elem) that has elem as local IN dependency (D_R1)
   *      b) Enqueue this "real" task (T_L1) with pseudo IN dependency (D_L1)
   *      c) Once D_R1 is satisfied, T_R1 again enqueues a remote task (on this task's unit) (T_L2) with
   *         pseudo OUT dependency D_L2 that immediately satisfies D_L1.
   *
   *  ===========                          |             ===========
   *  | Unit L  |                                        |Unit R   |
   *  ===========                          |             ===========
   *     |    |
   *     |    |                            |             ***********
   *     |    \----------------------------------------->*Task T_R1*
   *     |                                 |             ***********
   *     |                                                    |
   *     | T_L1(D_L1)                      |                  | (IN dependency D_R1 satisfied)
   *     |                                                    |
   *     |                      -----------|------------------/
   *     |                      |
   *     v                      v          |
   *  ***********   D_L1 ***********
   *  *Task T_L1*<-------*Task T_L2*       |
   *  ***********        ***********
   *
   *  TODO:
   *  - What to do with remote OUT dependencies? Does it make sense to have remote OUT dependencies?
   *  - How to handle cases where the producing task on the remote side finishes before the remote task is enqueued?
   *    -> Do we still need synchronization between iterations?
   *  - Do we want to handle satisfaction of sub-range dependencies? Overhead for handling all individual elements might be too large!
   *  - What will pseudo dependencies look like?
   *  - Where is the tasking handled? Maybe DART should handle all the tasking, including local and remote task creation...
   */
}

void wait()
{
  /**
   * Wait for all local tasks to complete
   *
   * TODO: Do we need a task-specific wait?
   */
}

} // namespace concurrency
} // namespace dash



#endif /* DASH__CONCURRENCY_H_ */
