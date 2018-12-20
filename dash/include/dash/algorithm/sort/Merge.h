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

template <
    typename GlobIterT,
    typename SendInfoT,
    typename LocalIt,
    typename ThreadPoolT>
ChunkDependencies psort__exchange_data(
    GlobIterT       begin,
    GlobIterT       end,
    const LocalIt   lcopy_begin,
    const SendInfoT get_send_info,
    const UnitInfo& p_unit_info,
    ThreadPoolT&    thread_pool)
{
  using iter_type = GlobIterT;

  auto&      pattern       = begin.pattern();
  auto&      team          = begin.team();
  auto const unit_at_begin = pattern.unit_at(begin.pos());
  auto const myid          = team.myid();

  // local distance
  auto const l_range     = dash::local_index_range(begin, end);
  auto*      l_mem_begin = dash::local_begin(
      static_cast<typename iter_type::pointer>(begin), team.myid());

  auto* const lbegin = l_mem_begin + l_range.begin;
  auto* const lend   = l_mem_begin + l_range.end;

  std::size_t target_count, src_disp, target_disp;

  // Futures for the merges - only used to signal readiness.
  // Use a std::map because emplace will not invalidate any
  // references or iterators.
  ChunkDependencies chunk_dependencies;

  for (auto const& unit : p_unit_info.valid_remote_partitions) {
    std::tie(target_count, src_disp, target_disp) = get_send_info(unit);

    if (0 == target_count) {
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
            begin
            :
            /* Otherwise construct an global iterator pointing the first local
               element from the correspoding unit */
            iter_type{&(begin.globmem()),
                      pattern,
                      pattern.global_index(
                          static_cast<dash::team_unit_t>(unit), {})};

    dart_handle_t handle;
    dash::internal::get_handle(
        (it_src + src_disp).dart_gptr(),
        std::addressof(*(lcopy_begin + target_disp)),
        target_count,
        &handle);

    // A chunk range (unit, unit + 1) signals represents the copy. Unit + 1 is
    // a sentinel here.
    ChunkRange unit_range(unit, unit + 1);

    // Copy the handle into a task and wait
    chunk_dependencies.emplace(
        unit_range, thread_pool.submit([handle]() mutable {
          if (handle != DART_HANDLE_NULL) {
            dart_wait(&handle);
          }
        }));
  }

  std::tie(target_count, src_disp, target_disp) = get_send_info(myid);

  // Create an entry for the local part
  ChunkRange local_range(myid, myid + 1);
  chunk_dependencies.emplace(
      local_range,
      thread_pool.submit([target_count,
                          local_range,
                          src_disp,
                          target_disp,
                          lbegin,
                          lcopy_begin] {
        if (target_count) {
          std::copy(
              std::next(lbegin, src_disp),
              std::next(lbegin, src_disp + target_count),
              std::next(lcopy_begin, target_disp));
        }
      }));
  return std::move(chunk_dependencies);
}

template <
    typename LocalIt,
    typename MergeDeps,
    typename SortCompT,
    typename ThreadPoolT>
void psort__merge_local(
    LocalIt                         out,
    LocalIt                         buffer,
    const std::vector<std::size_t>& target_displs,
    MergeDeps&                      chunk_dependencies,
    SortCompT                       sort_comp,
    dash::Team const&               team,
    ThreadPoolT&                    thread_pool,
    bool                            in_place)
{
  auto const nunits  = team.size();
  auto       nchunks = nunits;
  // number of merge steps in the tree
  auto const depth = static_cast<size_t>(std::ceil(std::log2(nchunks)));

  // calculate the prefix sum among all receive counts to find the offsets for
  // merging

  for (std::size_t d = 0; d < depth; ++d) {
    // distance between first and mid iterator while merging
    auto const step = std::size_t(0x1) << d;
    // distance between first and last iterator while merging
    auto const dist = step << 1;
    // number of merges
    auto const nmerges = nchunks >> 1;

    // Start threaded merges. When d == 0 they depend on dash::copy to finish,
    // later on other merges.
    for (std::size_t m = 0; m < nmerges; ++m) {
      auto f  = m * dist;
      auto mi = m * dist + step;
      // sometimes we have a lonely merge in the end, so we have to guarantee
      // that we do not access out of bounds
      auto l = std::min(m * dist + dist, nunits);

      // tuple of chunk displacements
      auto chunk_displs = std::make_tuple(
          target_displs[f], target_displs[mi], target_displs[l]);

      // pair of merge dependencies
      auto merge_deps =
          std::make_pair(impl::ChunkRange{f, mi}, impl::ChunkRange{mi, l});

      // Start a thread that blocks until the two previous merges are ready.
      auto&&     fut = thread_pool.submit([nunits,
                                       out,
                                       buffer,
                                       displs = std::move(chunk_displs),
                                       deps   = std::move(merge_deps),
                                       sort_comp,
                                       in_place,
                                       &team,
                                       &chunk_dependencies]() {
        // indexes for displacements
        static constexpr int c_first  = 0;
        static constexpr int c_middle = 1;
        static constexpr int c_last   = 2;

        auto first = std::next(buffer, std::get<c_first>(displs));
        auto mid   = std::next(buffer, std::get<c_middle>(displs));
        auto last  = std::next(buffer, std::get<c_last>(displs));
        // Wait for the left and right chunks to be copied/merged
        // This guarantees that for
        //
        // [____________________________]
        // ^f           ^mi             ^l
        //
        // [f, mi) and [mi, f) are both merged sequences when the task
        // continues.

        static constexpr int left_dep  = 0;
        static constexpr int right_dep = 1;

        auto dep_l = std::get<left_dep>(deps);
        auto dep_r = std::get<right_dep>(deps);

        if (chunk_dependencies[dep_l].valid()) {
          chunk_dependencies[dep_l].wait();
        }
        if (chunk_dependencies[dep_r].valid()) {
          chunk_dependencies[dep_r].wait();
        }

        if (in_place) {
          // The final merge can be done non-inplace, because we need to
          // copy the result to the final buffer anyways.
          if (dep_l.first == 0 && dep_r.second == nunits) {
            // Make sure everyone merged their parts (necessary for the copy
            // into the final buffer)
            team.barrier();
            std::merge(first, mid, mid, last, out, sort_comp);
          }
          else {
            std::inplace_merge(first, mid, last, sort_comp);
          }
        }
        else {
          DASH_THROW(
              dash::exception::NotImplemented,
              "non-inplace merge not supported yet");
          // std::merge(first, mid, mid, last, std::next(out, first),
          // sort_comp);
        }
        DASH_LOG_TRACE("merged chunks", dep_l.first, dep_r.second);
      });
      ChunkRange to_merge(f, l);
      chunk_dependencies.emplace(to_merge, std::move(fut));
    }

    nchunks -= nmerges;
  }
}

}  // namespace impl
}  // namespace dash

#endif
