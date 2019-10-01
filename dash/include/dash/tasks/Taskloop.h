#ifndef DASH__TASKS__PARALLELFOR_H__
#define DASH__TASKS__PARALLELFOR_H__

#include <dash/tasks/Tasks.h>
#include <dash/tasks/internal/LambdaTraits.h>
#include <dash/Exception.h>
#include <dash/dart/if/dart_tasking.h>

#include <atomic>


namespace dash{
namespace tasks{

  class num_chunks {
  public:
    explicit num_chunks(size_t nc) : n{nc}
    { }

    template<typename IteratorT>
    size_t
    get_chunk_size(IteratorT begin, IteratorT end) {
      size_t nelem = dash::distance(begin, end);
      // round-up if necessary
      size_t chunk_size = (nelem + n - 1) / n;

      return chunk_size ? chunk_size : 1;
    }

    template<typename IteratorT>
    size_t
    get_num_chunks(IteratorT, IteratorT) {
      return n;
    }


  private:
    size_t n;
  };


  class chunk_size {
  public:
    explicit chunk_size(size_t cs) : n{cs?cs:1}
    { }

    template<typename IteratorT>
    size_t
    get_chunk_size(IteratorT, IteratorT) {
      return n;
    }

    template<typename IteratorT>
    size_t
    get_num_chunks(IteratorT begin, IteratorT end) {
      size_t nelem = dash::distance(begin, end);
      // round-up if necessary
      size_t num_chunks = (nelem + n - 1) / n;
      return num_chunks;
    }

  private:
    size_t n;
  };

namespace internal {
  template<typename T>
  struct is_chunk_definition : std::false_type
  { };

  template<>
  struct is_chunk_definition<dash::tasks::chunk_size> : std::true_type
  { };

  template<>
  struct is_chunk_definition<dash::tasks::num_chunks> : std::true_type
  { };

  /**
   * Wrapper around a function object that contains a simple reference count.
   * The object is allocated using \c CountedFunction<T>::create()
   * and released as soon as all references have been dropped.
   */
  template<typename FunctionT>
  struct CountedFunction {
  private:
    CountedFunction(const FunctionT& fn, int32_t cnt) : m_fn(fn), m_cnt(cnt)
    { }
    ~CountedFunction() { }

  public:

    static
    CountedFunction<FunctionT>* create(const FunctionT& fn, int32_t cnt) {
      return new CountedFunction<FunctionT>(fn, cnt);
    }

    void
    drop(void)
    {
      int32_t cnt = --m_cnt;
      DASH_ASSERT(cnt >= 0);
      if (cnt == 0) {
        // once all references are gone we delete ourselves
        delete this;
      }
    }

    FunctionT& operator*()
    {
      return m_fn;
    }


    FunctionT& get()
    {
      return m_fn;
    }


  private:
    FunctionT m_fn;
    std::atomic<int32_t> m_cnt;
  };

  template<
    class InputIter,
    class ChunkType,
    class RangeFunc,
    typename = typename std::enable_if<
                  internal::is_chunk_definition<ChunkType>::value>::type>
  void
  taskloop(
    InputIter   begin,
    InputIter   end,
    ChunkType   chunking,
    RangeFunc   f,
    const char *name = nullptr)
  {
    // TODO: extend this to handle GlobIter!

    // skip empty ranges
    if (dash::distance(begin, end) == 0) {
      return;
    }

    size_t chunk_size = chunking.get_chunk_size(begin, end);
    InputIter from = begin;
    constexpr const bool is_mutable =
                            !internal::is_const_callable<RangeFunc, InputIter, InputIter>::value;

    using CountedFunctionT = CountedFunction<RangeFunc>;
    CountedFunctionT *f_ptr;
    if (!is_mutable) {
      int32_t num_chunks = chunking.get_num_chunks(begin, end);
      f_ptr = CountedFunctionT::create(std::move(f), num_chunks);
    }

    while (from < end) {
      InputIter to = from + chunk_size;
      if (to > end) to = end;
#if DASH_TASKS_INVOKE_DIRECT
      f(from, to);
#else // DASH_TASKS_INVOKE_DIRECT
      if (!is_mutable) {
        dash::tasks::async(
          name,
          // capture the counted ptr and the range
          [f_ptr, from, to](){
            // the d'tor will finally delete the object once all
            // references are gone
            auto _ = internal::finally([f_ptr](){ f_ptr->drop(); });

            f_ptr->get()(from, to);
          }
        );
      } else {
        dash::tasks::async(
          name,
          // capture the function object and the range
          [f, from, to](){
            f(from, to);
          }
        );
      }
#endif // DASH_TASKS_INVOKE_DIRECT
      from = to;
    }
  }

  template<
    class InputIter,
    class ChunkType,
    class RangeFunc,
    class DepGeneratorFunc,
    typename = typename std::enable_if<
                  internal::is_chunk_definition<ChunkType>::value>::type>
  void
  taskloop(
    InputIter   begin,
    InputIter   end,
    ChunkType   chunking,
    RangeFunc   f,
    DepGeneratorFunc depedency_generator,
    const char *name = nullptr)
  {
    // TODO: extend this to handle GlobIter!

    // skip empty ranges
    if (dash::distance(begin, end) == 0) {
      return;
    }

    size_t chunk_size = chunking.get_chunk_size(begin, end);
    InputIter from = begin;
    constexpr const bool is_mutable =
                            !internal::is_const_callable<RangeFunc, InputIter, InputIter>::value;

#if DASH_TASKS_INVOKE_DIRECT
    while (from < end) {
      InputIter to = from + chunk_size;
      if (to > end) to = end;
      f(from, to);
      from = to;
    }
#else // DASH_TASKS_INVOKE_DIRECT
    using CountedFunctionT = CountedFunction<RangeFunc>;
    CountedFunctionT *f_ptr;
    if (!is_mutable) {
      int32_t num_chunks = chunking.get_num_chunks(begin, end);
      f_ptr = CountedFunctionT::create(std::move(f), num_chunks);
    }

    DependencyContainer deps;
    while (from < end) {
      InputIter to = from + chunk_size;
      if (to > end) to = end;
      auto dep_inserter = DependencyContainerInserter(deps, deps.begin());
      depedency_generator(from, to, dep_inserter);
      if (!is_mutable) {
        dash::tasks::internal::async(
          // capture the counted ptr and the range
          [f_ptr, from, to](){
            // the d'tor will finally delete the object once all
            // references are gone
            auto _ = internal::finally([f_ptr](){ f_ptr->drop(); });

            f_ptr->get()(from, to);
          },
          deps,
          name
        );
      } else {
        dash::tasks::internal::async(
          // capture the function object and the range
          [f, from, to](){
            f(from, to);
          },
          deps,
          name
        );
      }
      from = to;
      deps.clear();
    }
#endif // DASH_TASKS_INVOKE_DIRECT
  }


} // namespace internal


  /**
   * Create a bunch of tasks operating on the input range but do not wait
   * for their completion.
   * TODO: Add launch policies here!
   */
  template<
    class InputIter,
    class ChunkType,
    class RangeFunc,
    typename = typename std::enable_if<
                  internal::is_chunk_definition<ChunkType>::value>::type>
  void
  taskloop(
    InputIter begin,
    InputIter end,
    ChunkType chunking,
    RangeFunc f)
  {
    internal::taskloop(begin, end, chunking, f);
  }

  template<
    class InputIter,
    class ChunkType,
    class RangeFunc,
    class DepGeneratorFunc,
    typename = typename std::enable_if<
                  internal::is_chunk_definition<ChunkType>::value>::type>
  void
  taskloop(
    InputIter begin,
    InputIter end,
    ChunkType chunking,
    RangeFunc f,
    DepGeneratorFunc dependency_generator)
  {
    internal::taskloop(begin, end, chunking, f, dependency_generator);
  }

  template<class InputIter, typename RangeFunc>
  void
  taskloop(
    InputIter begin,
    InputIter end,
    RangeFunc f)
  {
    internal::taskloop(begin, end, dash::tasks::num_chunks{dash::tasks::numthreads()}, f);
  }

  template<
    class InputIter,
    class RangeFunc,
    class DepGeneratorFunc>
  auto
  taskloop(
    InputIter        begin,
    InputIter        end,
    RangeFunc        f,
    DepGeneratorFunc dependency_generator)
  ->  typename
      std::enable_if<!internal::is_chunk_definition<RangeFunc>::value, void>::type
  {
    internal::taskloop(begin, end, dash::tasks::num_chunks{dash::tasks::numthreads()},
                       f, dependency_generator);
  }

  template<
    class InputIter,
    class ChunkType,
    class RangeFunc,
    typename = typename std::enable_if<
                  internal::is_chunk_definition<ChunkType>::value>::type>
  void
  taskloop(
    const char *name,
    InputIter   begin,
    InputIter   end,
    ChunkType   chunking,
    RangeFunc   f)
  {
    internal::taskloop(begin, end, chunking, f, name);
  }

  template<
    class InputIter,
    class ChunkType,
    class RangeFunc,
    class DepGeneratorFunc,
    typename = typename std::enable_if<
                  internal::is_chunk_definition<ChunkType>::value>::type>
  void
  taskloop(
    const char *name,
    InputIter   begin,
    InputIter   end,
    ChunkType   chunking,
    RangeFunc   f,
    DepGeneratorFunc dependency_generator)
  {
    internal::taskloop(begin, end, chunking,
                       f, dependency_generator, name);
  }

  template<class InputIter, typename RangeFunc>
  void
  taskloop(
    const char *name,
    InputIter   begin,
    InputIter   end,
    RangeFunc   f)
  {
    internal::taskloop(begin, end,
                       dash::tasks::num_chunks{dash::tasks::numthreads()}, f, name);
  }

  template<
    class InputIter,
    class RangeFunc,
    class DepGeneratorFunc>
  auto
  taskloop(
    const char      *name,
    InputIter        begin,
    InputIter        end,
    RangeFunc        f,
    DepGeneratorFunc dependency_generator)
  ->  typename
      std::enable_if<!internal::is_chunk_definition<RangeFunc>::value, void>::type
  {
    internal::taskloop(begin, end,
                       dash::tasks::num_chunks{dash::tasks::numthreads()},
                       f, dependency_generator, name);
  }


#define SLOC_(__file, __delim, __line) __file # __delim # __line
#define SLOC(__file, __line)  SLOC_(__file, :, __line)
#define TASKLOOP(...) taskloop(SLOC(__FILE__, __LINE__), __VA_ARGS__)

} // namespace tasks
} // namespace dash

#endif // DASH__TASKS__PARALLELFOR_H__
