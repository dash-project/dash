#ifndef DASH__ALGORITHM__SORT_H
#define DASH__ALGORITHM__SORT_H

#include <algorithm>
#include <functional>
#include <iterator>
#include <type_traits>
#include <vector>

#include <dash/Array.h>
#include <dash/Exception.h>
#include <dash/Meta.h>
#include <dash/dart/if/dart.h>

#include <dash/algorithm/Copy.h>
#include <dash/algorithm/LocalRange.h>

#include <dash/internal/Logging.h>
#include <dash/util/Trace.h>

namespace dash {

#ifdef DOXYGEN

/**
 * Sorts the elements in the range, defined by \c [begin, end) in ascending
 * order. The order of equal elements is not guaranteed to be preserved.
 *
 * Elements are sorted using operator<. Additionally, the elements must be
 * arithmetic, i.e. std::is_arithmetic<T> must be satisfied.
 *
 * In terms of data distribution, source and destination ranges passed to
 * \c dash::sort must be global (\c GlobIter<ValueType>).
 *
 * The operation is collective among the team of the owning dash container.
 *
 * Example:
 *
 * \code
 *       dash::Array<int> arr(100);
 *       dash::generate(arr.begin(), arr.end());
 *       dash::sort(array.begin(),
 *                  array.end();
 * \endcode
 *
 * \ingroup  DashAlgorithms
 */
template <class GlobRandomIt>
void sort(GlobRandomIt begin, GlobRandomIt end);

/**
 * Sorts the elements in the range, defined by \c [begin, end) in ascending
 * order. The order of equal elements is not guaranteed to be preserved.
 *
 * Elements are sorted by using a user-defined hash function. Resulting values
 * of the hash function are required to be sortable by operator<.
 *
 * This variant may be appropriate if the underlying container does not hold
 * arithmetic values (e.g. structs).
 *
 * In terms of data distribution, source and destination ranges passed to
 * \c dash::sort must be global (\c GlobIter<ValueType>).
 *
 * The operation is collective among the team of the owning dash container.
 *
 * Example:
 *
 * \code
 *       struct pair { int x; int y; };
 *       dash::Array<pair> arr(100);
 *       dash::generate(arr.begin(), arr.end(), random());
 *       dash::sort(array.begin(),
 *                  array.end(),
 *                  [](pair const & p){return p.x % 13; }
 *                 );
 * \endcode
 *
 * \ingroup  DashAlgorithms
 */
template <class GlobRandomIt, class SortableHash>
void sort(GlobRandomIt begin, GlobRandomIt end, SortableHash hash);

#else

#define __DASH_SORT__FINAL_STEP_BY_MERGE (0)
#define __DASH_SORT__FINAL_STEP_BY_SORT (1)
#define __DASH_SORT__FINAL_STEP_STRATEGY (__DASH_SORT__FINAL_STEP_BY_MERGE)

#include <dash/algorithm/internal/Sort-inl.h>

template <class GlobRandomIt, class SortableHash>
void sort(GlobRandomIt begin, GlobRandomIt end, SortableHash sortable_hash)
{
  using iter_type    = GlobRandomIt;
  using value_type   = typename iter_type::value_type;
  using mapped_type =
      typename std::decay<typename dash::functional::closure_traits<
          SortableHash>::result_type>::type;

  static_assert(
      std::is_arithmetic<mapped_type>::value,
      "Only arithmetic types are supported");

  auto pattern = begin.pattern();

  dash::util::Trace trace("Sort");

  auto const sort_comp = [&sortable_hash](
                             const value_type& a, const value_type& b) {
    return sortable_hash(a) < sortable_hash(b);
  };

  if (pattern.team() == dash::Team::Null()) {
    DASH_LOG_TRACE("dash::sort", "Sorting on dash::Team::Null()");
    return;
  }
  if (pattern.team().size() == 1) {
    DASH_LOG_TRACE("dash::sort", "Sorting on a team with only 1 unit");
    trace.enter_state("final_local_sort");
    std::sort(begin.local(), end.local(), sort_comp);
    trace.exit_state("final_local_sort");
    return;
  }

  if (begin >= end) {
    DASH_LOG_TRACE("dash::sort", "empty range");
    trace.enter_state("final_barrier");
    pattern.team().barrier();
    trace.exit_state("final_barrier");
    return;
  }

  dash::Team& team   = pattern.team();
  auto const  nunits = team.size();
  auto const  myid   = team.myid();

  auto const unit_at_begin = pattern.unit_at(begin.pos());

  // local distance
  auto const l_range = dash::local_index_range(begin, end);

  auto l_mem_begin = begin.globmem().lbegin();

  auto const n_l_elem = l_range.end - l_range.begin;

  auto lbegin = l_mem_begin + l_range.begin;
  auto lend   = l_mem_begin + l_range.end;

  // initial local_sort
  trace.enter_state("1:initial_local_sort");
  std::sort(lbegin, lend, sort_comp);
  trace.exit_state("1:initial_local_sort");

  trace.enter_state("2:init_temporary_global_data");

  using array_t = dash::Array<std::size_t>;

  std::size_t gsize = nunits * NLT_NLE_BLOCK * 2;

  // implicit barrier...
  array_t g_partition_data(nunits * nunits * 3, dash::BLOCKED, team);
  std::uninitialized_fill(
      g_partition_data.lbegin(), g_partition_data.lend(), 0);

  trace.exit_state("2:init_temporary_global_data");

  trace.enter_state("3:find_global_min_max");

  // Temporary local buffer (sorted);
  std::vector<value_type> const lcopy(lbegin, lend);

  auto const min_max = detail::find_global_min_max(
      std::begin(lcopy), std::end(lcopy), team.dart_id(), sortable_hash);

  trace.exit_state("3:find_global_min_max");

  DASH_LOG_TRACE_VAR("global minimum in range", min_max.first);
  DASH_LOG_TRACE_VAR("global maximum in range", min_max.second);

  if (min_max.first == min_max.second) {
    // all values are equal, so nothing to sort globally.
    pattern.team().barrier();
    return;
  }

  trace.enter_state("4:init_temporary_local_data");

  auto const p_unit_info =
      detail::psort__find_partition_borders(pattern, begin, end);

  auto const& acc_partition_count = p_unit_info.acc_partition_count;

  auto const               nboundaries = nunits - 1;
  std::vector<mapped_type> splitters(nboundaries, mapped_type{});

  detail::PartitionBorder<mapped_type> p_borders(
      nboundaries, min_max.first, min_max.second);

  detail::psort__init_partition_borders(p_unit_info, p_borders);

  DASH_LOG_TRACE_RANGE(
      "locally sorted array", std::begin(lcopy), std::end(lcopy));
  DASH_LOG_TRACE_RANGE(
      "skipped splitters",
      p_borders.is_skipped.cbegin(),
      p_borders.is_skipped.cend());

  bool done = false;

  // collect all valid splitters in a temporary vector
  std::vector<size_t> valid_partitions;

  {
    // make this as a separately scoped block to deallocate non-required
    // temporary memory
    std::vector<size_t> all_borders(splitters.size());
    std::iota(all_borders.begin(), all_borders.end(), 0);

    auto const& is_skipped = p_borders.is_skipped;

    std::copy_if(
        all_borders.begin(),
        all_borders.end(),
        std::back_inserter(valid_partitions),
        [&is_skipped](size_t idx) { return is_skipped[idx] == false; });
  }

  DASH_LOG_TRACE_RANGE(
      "valid partitions",
      std::begin(valid_partitions),
      std::end(valid_partitions));

  if (valid_partitions.empty()) {
    // Edge case: We may have a team spanning at least 2 units, however the
    // global range is owned by  only 1 unit
    team.barrier();
    return;
  }

  trace.exit_state("4:init_temporary_local_data");

  trace.enter_state("5:find_global_partition_borders");

  size_t iter = 0;

  std::vector<size_t> global_histo(nunits * NLT_NLE_BLOCK, 0);

  do {
    ++iter;

    detail::psort__calc_boundaries(p_borders, splitters);

    DASH_LOG_TRACE_VAR("finding partition borders", iter);

    DASH_LOG_TRACE_RANGE(
        "partition borders", std::begin(splitters), std::end(splitters));

    auto const l_nlt_nle = detail::psort__local_histogram(
        splitters,
        valid_partitions,
        p_borders,
        std::begin(lcopy),
        std::end(lcopy),
        sortable_hash);

    detail::trace_local_histo("local histogram", l_nlt_nle);

    // allreduce with implicit barrier
    detail::psort__global_histogram(
        // first partition
        std::begin(l_nlt_nle),
        // iterator past last valid partition
        std::next(
            std::begin(l_nlt_nle),
            (valid_partitions.back() + 1) * NLT_NLE_BLOCK),
        std::begin(global_histo),
        team.dart_id());

    DASH_LOG_TRACE_RANGE(
        "global histogram",
        std::next(std::begin(global_histo), myid * NLT_NLE_BLOCK),
        std::next(std::begin(global_histo), (myid + 1) * NLT_NLE_BLOCK));

    done = detail::psort__validate_partitions(
        p_unit_info, splitters, valid_partitions, p_borders, global_histo);
  } while (!done);

  trace.exit_state("5:find_global_partition_borders");

  DASH_LOG_TRACE_VAR("partition borders found after N iterations", iter);

  trace.enter_state("6:final_local_histogram");

  /* How many elements are less than P
   * or less than equals P */
  auto const histograms = detail::psort__local_histogram(
      splitters,
      valid_partitions,
      p_borders,
      std::begin(lcopy),
      std::end(lcopy),
      sortable_hash);
  trace.exit_state("6:final_local_histogram");

  DASH_LOG_TRACE_RANGE("final splitters", splitters.begin(), splitters.end());

  detail::trace_local_histo("final histograms", histograms);

  trace.enter_state("7:transpose_local_histograms (all-to-all)");

  if (n_l_elem > 0) {
    // TODO(kowalewski): minimize communication to copy only until the last
    // valid border
    /*
     * Transpose (Shuffle) the final histograms to communicate
     * the partition distribution
     */

    dash::team_unit_t transposed_unit{0};
    for (auto it = std::begin(histograms); it != std::end(histograms);
         it += NLT_NLE_BLOCK, ++transposed_unit) {
      auto const& nlt_val = *it;
      auto const& nle_val = *std::next(it);
      if (transposed_unit != myid) {
        auto const offset = transposed_unit * g_partition_data.lsize() + myid;
        // We communicate only non-zero values
        if (nlt_val > 0) {
          g_partition_data.async[offset + IDX_DIST(nunits)].set(&(nlt_val));
        }

        if (nle_val > 0) {
          g_partition_data.async[offset + IDX_SUPP(nunits)].set(&(nle_val));
        }
      }
      else {
        g_partition_data.local[myid + IDX_DIST(nunits)] = nlt_val;
        g_partition_data.local[myid + IDX_SUPP(nunits)] = nle_val;
      }
    }
    // complete outstanding requests...
    g_partition_data.async.flush();
  }
  trace.exit_state("7:transpose_local_histograms (all-to-all)");

  trace.enter_state("8:barrier");
  team.barrier();
  trace.exit_state("8:barrier");

  DASH_LOG_TRACE_RANGE(
      "initial partition distribution:",
      std::next(g_partition_data.lbegin(), IDX_DIST(nunits)),
      std::next(g_partition_data.lbegin(), IDX_DIST(nunits) + nunits));

  DASH_LOG_TRACE_RANGE(
      "initial partition supply:",
      std::next(g_partition_data.lbegin(), IDX_SUPP(nunits)),
      std::next(g_partition_data.lbegin(), IDX_SUPP(nunits) + nunits));

  /* Calculate final distribution per partition. Each unit calculates their
   * local distribution independently.
   * All accesses are only to local memory
   */

  trace.enter_state("9:calc_final_partition_dist");

  detail::psort__calc_final_partition_dist(
      acc_partition_count, g_partition_data.local);

  DASH_LOG_TRACE_RANGE(
      "final partition distribution",
      std::next(g_partition_data.lbegin(), IDX_DIST(nunits)),
      std::next(g_partition_data.lbegin(), IDX_DIST(nunits) + nunits));

  // Reset local elements to 0 since the following matrix transpose
  // communicates only non-zero values and writes to exactly these offsets.
  std::fill(
      &(g_partition_data.local[IDX_TARGET_COUNT(nunits)]),
      &(g_partition_data.local[IDX_TARGET_COUNT(nunits) + nunits]),
      0);

  trace.exit_state("9:calc_final_partition_dist");

  trace.enter_state("10:barrier");
  team.barrier();
  trace.exit_state("10:barrier");

  trace.enter_state("11:transpose_final_partition_dist (all-to-all)");
  /*
   * Transpose the final distribution again to obtain the end offsets
   */
  dash::team_unit_t unit{0};
  auto const        last = static_cast<dash::team_unit_t>(nunits);

  for (; unit < last; ++unit) {
    if (g_partition_data.local[IDX_DIST(nunits) + unit] == 0) {
      continue;
    }

    if (unit != myid) {
      // We communicate only non-zero values
      auto const offset = unit * g_partition_data.lsize() + myid;
      g_partition_data.async[offset + IDX_TARGET_COUNT(nunits)].set(
          &(g_partition_data.local[IDX_DIST(nunits) + unit]));
    }
    else {
      g_partition_data.local[IDX_TARGET_COUNT(nunits) + myid] =
          g_partition_data.local[IDX_DIST(nunits) + unit];
    }
  }

  g_partition_data.async.flush();

  trace.exit_state("11:transpose_final_partition_dist (all-to-all)");

  trace.enter_state("12:barrier");
  team.barrier();
  trace.exit_state("12:barrier");

  DASH_LOG_TRACE_RANGE(
      "final target count",
      std::next(g_partition_data.lbegin(), IDX_TARGET_COUNT(nunits)),
      std::next(
          g_partition_data.lbegin(), IDX_TARGET_COUNT(nunits) + nunits));

  trace.enter_state("13:calc_final_send_count");

  std::vector<std::size_t> l_send_displs(nunits, 0);

  if (n_l_elem > 0) {
    auto const* l_target_count =
        &(g_partition_data.local[IDX_TARGET_COUNT(nunits)]);
    auto* l_send_count = &(g_partition_data.local[IDX_SEND_COUNT(nunits)]);

    detail::psort__calc_send_count(
        p_borders, valid_partitions, l_target_count, l_send_count);

    // exclusive scan using partial sum
    std::partial_sum(
        l_send_count,
        std::next(l_send_count, nunits - 1),
        std::next(std::begin(l_send_displs)),
        std::plus<size_t>());
  }
  else {
    std::fill(
        std::next(g_partition_data.lbegin(), IDX_SEND_COUNT(nunits)),
        std::next(g_partition_data.lbegin(), IDX_SEND_COUNT(nunits) + nunits),
        0);
  }

#if defined(DASH_ENABLE_ASSERTIONS) && defined(DASH_ENABLE_TRACE_LOGGING)
  {
    std::vector<size_t> chksum(nunits, 0);

    DASH_ASSERT_RETURNS(
        dart_allreduce(
            std::next(g_partition_data.lbegin(), IDX_SEND_COUNT(nunits)),
            chksum.data(),
            nunits,
            dart_datatype<size_t>::value,
            DART_OP_SUM,
            team.dart_id()),
        DART_OK);

    DASH_ASSERT_EQ(
        chksum[myid.id],
        n_l_elem,
        "send count must match the capacity of the unit");
  }
#endif

  DASH_LOG_TRACE_RANGE(
      "send count",
      std::next(g_partition_data.lbegin(), IDX_SEND_COUNT(nunits)),
      std::next(g_partition_data.lbegin(), IDX_SEND_COUNT(nunits) + nunits));

  DASH_LOG_TRACE_RANGE(
      "send displs", l_send_displs.begin(), l_send_displs.end());

  trace.exit_state("13:calc_final_send_count");

  trace.enter_state("14:calc_recv_count (all-to-all)");

  std::vector<size_t> recv_count(nunits, 0);

  DASH_ASSERT_RETURNS(
  dart_alltoall(
      // send buffer
      std::next(g_partition_data.lbegin(), IDX_SEND_COUNT(nunits)),
      // receive buffer
      recv_count.data(),
      // we send / receive 1 element to / from each process
      1,
      // dtype
      dash::dart_datatype<size_t>::value,
      // teamid
      team.dart_id()), DART_OK);

  DASH_LOG_TRACE_RANGE(
      "recv count", std::begin(recv_count), std::end(recv_count));

  trace.exit_state("14:calc_recv_count (all-to-all)");

  trace.enter_state("15:calc_final_target_displs");

  if (n_l_elem > 0) {
    detail::psort__calc_target_displs(
        p_borders, valid_partitions, g_partition_data);
  }

  trace.exit_state("15:calc_final_target_displs");

  trace.enter_state("16:barrier");
  team.barrier();
  trace.exit_state("16:barrier");

  DASH_LOG_TRACE_RANGE(
      "target displs",
      &(g_partition_data.local[IDX_TARGET_DISP(nunits)]),
      &(g_partition_data.local[IDX_TARGET_DISP(nunits) + nunits]));

  trace.enter_state("17:exchange_data (all-to-all)");

  std::vector<dash::Future<iter_type> > async_copies{};
  async_copies.reserve(p_unit_info.valid_remote_partitions.size());

  auto const l_partition_data = g_partition_data.local;

  auto const get_send_info = [l_partition_data, &l_send_displs, nunits](
                                 dash::default_index_t const p_idx) {
    auto const send_count = l_partition_data[p_idx + IDX_SEND_COUNT(nunits)];
    auto const target_disp =
        l_partition_data[p_idx + IDX_TARGET_DISP(nunits)];
    auto const send_disp = l_send_displs[p_idx];
    return std::make_tuple(send_count, send_disp, target_disp);
  };

  std::size_t send_count, send_disp, target_disp;

  for (auto const& unit : p_unit_info.valid_remote_partitions) {
    std::tie(send_count, send_disp, target_disp) = get_send_info(unit);

    // Get a global iterator to the first local element of a unit within the
    // range to be sorted [begin, end)
    //
    iter_type it_copy =
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

    auto&& fut = dash::copy_async(
        &(*(lcopy.begin() + send_disp)),
        &(*(lcopy.begin() + send_disp + send_count)),
        it_copy + target_disp);

    async_copies.emplace_back(std::move(fut));
  }

  std::tie(send_count, send_disp, target_disp) = get_send_info(myid);

  if (send_count) {
    std::copy(
        std::next(std::begin(lcopy), send_disp),
        std::next(std::begin(lcopy), send_disp + send_count),
        std::next(lbegin, target_disp));
  }

  std::for_each(
      std::begin(async_copies),
      std::end(async_copies),
      [](dash::Future<iter_type>& fut) { fut.wait(); });

  trace.exit_state("17:exchange_data (all-to-all)");

  trace.enter_state("18:barrier");
  team.barrier();
  trace.exit_state("18:barrier");

  /* NOTE: While merging locally sorted sequences is faster than another
   * heavy-weight sort it comes at a cost. std::inplace_merge allocates a
   * temporary buffer internally which is also documented on cppreference. If
   * the allocation of this buffer fails, a less efficient merge method is
   * used. However, in Linux, the allocation nevers fails since the
   * implementation simply allocates memory using malloc and the kernel follows
   * the optimistic strategy. This is ugly and can lead to a segmentation fault
   * later if no physical pages are available to map the allocated
   * virtual memory.
   *
   *
   * std::sort does not suffer from this problem and may be a more safe
   * variant, especially if the user wants to utilize the fully available
   * memory capacity on its own.
   */

#if (__DASH_SORT__FINAL_STEP_STRATEGY == __DASH_SORT__FINAL_STEP_BY_SORT)
  trace.enter_state("19:final_local_sort");
  std::sort(lbegin, lend);
  trace.exit_state("19:final_local_sort");
#else
  trace.enter_state("19:merge_local_sequences");

  // merging sorted sequences
  auto nsequences = nunits;
  // number of merge steps in the tree
  auto const depth = static_cast<size_t>(std::ceil(std::log2(nsequences)));

  // calculate the prefix sum among all receive counts to find the offsets for
  // merging
  std::vector<size_t> recv_count_psum;
  recv_count_psum.reserve(nsequences + 1);
  recv_count_psum.emplace_back(0);

  std::partial_sum(
      std::begin(recv_count),
      std::end(recv_count),
      std::back_inserter(recv_count_psum));

  DASH_LOG_TRACE_RANGE(
      "recv count prefix sum",
      std::begin(recv_count_psum),
      std::end(recv_count_psum));

  for (std::size_t d = 0; d < depth; ++d) {
    // distance between first and mid iterator while merging
    auto const step = std::size_t(0x1) << d;
    // distance between first and last iterator while merging
    auto const dist = step << 1;
    // number of merges
    auto const nmerges = nsequences >> 1;

    // These merges are independent from each other and are candidates for
    // shared memory parallelism
    for (std::size_t m = 0; m < nmerges; ++m) {
      auto first = std::next(lbegin, recv_count_psum[m * dist]);
      auto mid   = std::next(lbegin, recv_count_psum[m * dist + step]);
      // sometimes we have a lonely merge in the end, so we have to guarantee
      // that we do not access out of bounds
      auto last = std::next(
          lbegin,
          recv_count_psum[std::min(
              m * dist + dist, recv_count_psum.size() - 1)]);

      std::inplace_merge(first, mid, last);
    }

    nsequences -= nmerges;
  }

  trace.exit_state("19:merge_local_sequences");
#endif

  DASH_LOG_TRACE_RANGE("finally sorted range", lbegin, lend);

  trace.enter_state("20:final_barrier");
  team.barrier();
  trace.exit_state("20:final_barrier");
}

namespace detail {
template <typename T>
struct identity_t : std::unary_function<T, T> {
  constexpr T&& operator()(T&& t) const noexcept
  {
    // A perfect forwarding identity function
    return std::forward<T>(t);
  }
};
}  // namespace detail

template <class GlobRandomIt>
inline void sort(GlobRandomIt begin, GlobRandomIt end)
{
  using value_t = typename std::remove_cv<
      typename dash::iterator_traits<GlobRandomIt>::value_type>::type;

  dash::sort(begin, end, detail::identity_t<value_t const&>());
}

#endif  // DOXYGEN

}  // namespace dash

#endif  // DASH__ALGORITHM__SORT_H
