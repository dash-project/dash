#ifndef DASH__ALGORITHM__SORT__MERGE_H
#define DASH__ALGORITHM__SORT__MERGE_H

#include <dash/algorithm/Copy.h>
#include <dash/algorithm/sort/ThreadPool.h>
#include <dash/algorithm/sort/Types.h>

#include <dash/dart/if/dart_communication.h>

#include <dash/internal/Logging.h>

#include <map>

namespace dash {
namespace impl {

template <typename GlobIterT, typename LocalIt, typename SendInfoT>
inline auto psort__exchange_data(
    GlobIterT                             from_global_begin,
    LocalIt                               to_local_begin,
    std::vector<dash::team_unit_t> const& valid_partitions,
    SendInfoT&&                           get_send_info)
{
  using iter_type = GlobIterT;

  auto&      pattern       = from_global_begin.pattern();
  auto&      team          = from_global_begin.team();
  auto const unit_at_begin = pattern.unit_at(from_global_begin.pos());

  auto                       nchunks = team.size();
  std::vector<dart_handle_t> handles(nchunks, DART_HANDLE_NULL);

  if (nullptr == to_local_begin) {
    // this is the case if we have an empty unit
    return handles;
  }

  std::size_t target_count, src_disp, target_disp;

  for (auto unit : valid_partitions) {
    std::tie(target_count, src_disp, target_disp) = get_send_info(unit);

    if (team.myid() == unit || 0 == target_count) {
      continue;
    }

    DASH_LOG_TRACE(
        "async copy",
        "source unit",
        unit,
        "target_count",
        target_count,
        "src_disp",
        src_disp,
        "target_disp",
        target_disp);

    // Get a global iterator to the first local element of a unit within the
    // range to be sorted [begin, end)
    //
    iter_type it_src =
        (unit == unit_at_begin)
            ?
            /* If we are the unit at the beginning of the global range simply
               return begin */
            from_global_begin
            :
            /* Otherwise construct an global iterator pointing the first local
               element from the correspoding unit */
            iter_type{std::addressof(from_global_begin.globmem()),
                      pattern,
                      pattern.global_index(
                          static_cast<dash::team_unit_t>(unit), {})};

    dash::internal::get_handle(
        (it_src + src_disp).dart_gptr(),
        std::addressof(*(to_local_begin + target_disp)),
        target_count,
        std::addressof(handles[unit]));
  }

  return handles;
}

template <class ThreadPoolT, class LocalCopy>
inline auto psort__schedule_copy_tasks(
    std::vector<dash::team_unit_t> const& remote_partitions,
    std::vector<dart_handle_t>            copy_handles,
    ThreadPoolT&                          thread_pool,
    dash::team_unit_t                     whoami,
    LocalCopy&&                           local_copy)
{
  // Futures for the merges - only used to signal readiness.
  // Use a std::map because emplace will not invalidate any
  // references or iterators.
  impl::ChunkDependencies chunk_dependencies;

  std::transform(
      std::begin(remote_partitions),
      std::end(remote_partitions),
      std::inserter(chunk_dependencies, chunk_dependencies.begin()),
      [&thread_pool,
       handles = std::move(copy_handles)](auto partition) mutable {
        // our copy handle
        dart_handle_t& handle = handles[partition];
        return std::make_pair(
            // the partition range
            std::make_pair(partition, partition + 1),
            // the future of our asynchronous communication task
            thread_pool.submit([hdl = std::move(handle)]() mutable {
              if (hdl != DART_HANDLE_NULL) {
                dart_wait(&hdl);
              }
            }));
      });

  // Create an entry for the local part
  ChunkRange local_range{whoami, whoami + 1};
  chunk_dependencies.emplace(local_range, thread_pool.submit(local_copy));
  DASH_ASSERT_EQ(
      remote_partitions.size() + 1,
      chunk_dependencies.size(),
      "invalid chunk dependencies");

  return chunk_dependencies;
}

template <class Iter, class OutputIt, class Cmp, class Barrier>
inline void merge_inplace(
    Iter      first,
    Iter      mid,
    Iter      last,
    OutputIt  out,
    Cmp&&     cmp,
    Barrier&& barrier,
    bool      is_final_merge)
{
  // The final merge can be done non-inplace, because we need to
  // copy the result to the final buffer anyways.
  if (is_final_merge) {
    // Make sure everyone merged their parts (necessary for the copy
    // into the final buffer)
    barrier();
    std::merge(first, mid, mid, last, out, cmp);
  }
  else {
    std::inplace_merge(first, mid, last, cmp);
  }
}

template <class Iter, class OutputIt, class Cmp>
inline void merge(
    Iter     first,
    Iter     mid,
    Iter     last,
    OutputIt out,
    Cmp&&    cmp,
    bool     is_final_merge)
{
  // The final merge can be done non-inplace, because we need to
  // copy the result to the final buffer anyways.
  if (is_final_merge) {
    // Make sure everyone merged their parts (necessary for the copy
    // into the final buffer)
    barrier();
    std::merge(first, mid, mid, last, out, cmp);
  }
  else {
    std::inplace_merge(first, mid, last, cmp);
  }
}

template <typename ThreadPoolT, typename MergeOp>
inline auto psort__merge_tree(
    ChunkDependencies chunk_dependencies,
    size_t            nchunks,
    ThreadPoolT&      thread_pool,
    MergeOp&&         mergeOp)
{
  // number of merge steps in the tree
  auto const depth = static_cast<size_t>(std::ceil(std::log2(nchunks)));

  auto const npartitions = nchunks;

  // calculate the prefix sum among all receive counts to find the offsets for
  // merging

  for (std::size_t d = 0; d < depth; ++d) {
    // distance between first and mid iterator while merging
    auto const step = std::size_t(0x1) << d;
    // distance between first and last iterator while merging
    auto const dist = step << 1;
    // number of merges
    auto const nmerges = nchunks >> 1;

    auto const is_final_merge = nchunks == 2;

    // Start threaded merges. When d == 0 they depend on dash::copy to finish,
    // later on other merges.
    for (std::size_t m = 0; m < nmerges; ++m) {
      auto f  = m * dist;
      auto mi = m * dist + step;
      // sometimes we have a lonely merge in the end, so we have to guarantee
      // that we do not access out of bounds
      auto l = std::min(m * dist + dist, npartitions);

      // tuple of chunk displacements. Be cautious with the indexes and the
      // order in make_tuple
      static constexpr int left   = 0;
      static constexpr int right  = 1;
      static constexpr int middle = 2;

      // Start a thread that blocks until the two previous merges are ready.
      auto fut = thread_pool.submit(
          [f, mi, l, &chunk_dependencies, is_final_merge, npartitions, merge = mergeOp]() {
            // Wait for the left and right chunks to be copied/merged
            // This guarantees that for
            //
            // [____________________________]
            // ^f           ^mi             ^l
            //
            // [f, mi) and [mi, f) are both merged sequences when the task
            // continues.

            // pair of merge dependencies
            ChunkRange dep_l{f, mi};
            ChunkRange dep_r{mi, l};

            if (chunk_dependencies[dep_l].valid()) {
              chunk_dependencies[dep_l].wait();
            }
            if (chunk_dependencies[dep_r].valid()) {
              chunk_dependencies[dep_r].wait();
            }

            merge(f, mi, l, is_final_merge);
            DASH_LOG_TRACE("merged chunks", dep_l.first, dep_r.second);
          });

      ChunkRange to_merge(f, l);
      chunk_dependencies.emplace(to_merge, std::move(fut));
    }

    nchunks -= nmerges;
  }

  // Wait for the final merge step
  impl::ChunkRange final_range(0, npartitions);
  chunk_dependencies.at(final_range).get();
}

}  // namespace impl
}  // namespace dash

#endif
