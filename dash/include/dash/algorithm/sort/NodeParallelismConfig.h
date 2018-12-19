#ifndef DASH__ALGORITHM__SORT__NODE_PARALLELISM_CONFIG_H
#define DASH__ALGORITHM__SORT__NODE_PARALLELISM_CONFIG_H

#include <dash/Exception.h>
#include <dash/internal/Macro.h>
#include <cstdint>
#include <thread>

#ifdef DASH_ENABLE_PSTL
#include <tbb/task_scheduler_init.h>
#endif
#ifdef DASH_ENABLE_OPENMP
#include <omp.h>
#endif

namespace dash {
namespace impl {
class NodeParallelismConfig {
  uint32_t m_nthreads{};
#ifdef DASH_ENABLE_PSTL
  // We use the default number of threads
  tbb::task_scheduler_init m_init{};
#endif
public:
  NodeParallelismConfig(uint32_t nthreads = 0)
#ifdef DASH_ENABLE_PSTL
    : m_nthreads(

          nthreads == 0 ? tbb::task_scheduler_init::default_num_threads()
                        : nthreads)
    , m_init(m_nthreads)
#endif
  {
#ifndef DASH_ENABLE_PSTL
    // If we use TBB we cannot do that
    setNumThreads(nthreads);
#endif
  }

  void setNumThreads(uint32_t nthreadsRequested) DASH_NOEXCEPT
  {
    m_nthreads = getNThreads(nthreadsRequested);

#if defined(DASH_ENABLE_OPENMP)
    omp_set_num_threads(m_nthreads);
#endif
  }

  auto parallelism() const noexcept
  {
    if (NodeParallelismConfig::hasNodeLevelParallelism()) {
      return m_nthreads;
    }
    else {
      return 1u;
    }
  }

private:
  constexpr static bool hasNodeLevelParallelism() noexcept
  {
#if defined(DASH_ENABLE_THREADSUPPORT) && \
    (defined(DASH_ENABLE_PSTL) || defined(DASH_ENABLE_OPENMP))
    return true;
#endif
    return false;
  }

  static uint32_t getNThreads(uint32_t nthreads) noexcept
  {
    if (!NodeParallelismConfig::hasNodeLevelParallelism()) {
      return 1u;
    }

    if (nthreads > 0) {
      return nthreads;
    }

#if defined(DASH_ENABLE_OPENMP)
    return omp_get_max_threads();
#else
    // always create at least one thread...
    return std::max(std::thread::hardware_concurrency(), 2u) - 1u;
#endif
  }
};
}  // namespace impl
}  // namespace dash

#endif  // DASH__ALGORITHM__SORT__NODE_PARALLELISM_CONFIG_H
