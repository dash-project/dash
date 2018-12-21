#ifndef DASH__ALGORITHM__SORT_H
#define DASH__ALGORITHM__SORT_H

#include <algorithm>
#include <functional>
#include <future>
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

#ifdef DOXYGEN
namespace dash {
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

}  // namespace dash

#else

#include <dash/algorithm/sort/Communication.h>
#include <dash/algorithm/sort/Histogram.h>
#include <dash/algorithm/sort/Merge.h>
#include <dash/algorithm/sort/NodeParallelismConfig.h>
#include <dash/algorithm/sort/Partition.h>
#include <dash/algorithm/sort/Sort-inl.h>
#include <dash/algorithm/sort/ThreadPool.h>
#include <dash/algorithm/sort/Types.h>

namespace dash {

template <
    class GlobRandomIt,
    class SortableHash,
    class MergeStrategy = impl::sort__final_strategy__merge>
void sort(
    GlobRandomIt begin,
    GlobRandomIt end,
    GlobRandomIt out,
    SortableHash sortable_hash)
{
  using iter_type  = GlobRandomIt;
  using value_type = typename iter_type::value_type;
  using mapped_type =
      typename std::decay<typename dash::functional::closure_traits<
          SortableHash>::result_type>::type;

  static_assert(
      std::is_arithmetic<mapped_type>::value,
      "Only arithmetic types are supported");

  static_assert(
      std::is_same<decltype(begin.pattern()), decltype(out.pattern())>::value,
      "incompatible pattern types for input and output iterator");

  if (begin.pattern().team() != out.pattern().team()) {
    DASH_LOG_ERROR("dash::sort", "incompatible teams");
    return;
  }

  auto const lcapacity = [](auto const& pattern) {
    auto const extents = pattern.local_extents(pattern.team().myid());
    auto const lsize   = std::accumulate(
        std::begin(extents),
        std::end(extents),
        std::size_t(1),
        std::multiplies<std::size_t>());
    return lsize;
  };

  auto lcap_in  = lcapacity(begin.pattern());
  auto lcap_out = lcapacity(out.pattern());

  if (lcap_out < lcap_in) {
    DASH_LOG_ERROR(
        "dash::sort",
        "cannot write into a output buffer which is smaller than the input "
        "buffer");
    return;
  }

  dash::util::Trace trace("Sort");

  auto const sort_comp = [&sortable_hash](
                             const value_type& a, const value_type& b) {
    return sortable_hash(a) < sortable_hash(b);
  };

  auto pattern = begin.pattern();

  dash::util::TeamLocality tloc{pattern.team()};
  auto                     uloc = tloc.unit_locality(pattern.team().myid());
  auto                     nthreads = uloc.num_domain_threads();

  DASH_ASSERT_GE(nthreads, 0, "invalid number of threads");

  dash::impl::NodeParallelismConfig nodeLevelConfig{
      static_cast<uint32_t>(nthreads)};

  impl::ThreadPool thread_pool{nodeLevelConfig.parallelism()};

  DASH_LOG_TRACE(
      "dash::sort",
      "nthreads for local parallelism: ",
      nodeLevelConfig.parallelism());

  if (pattern.team() == dash::Team::Null()) {
    DASH_LOG_TRACE("dash::sort", "Sorting on dash::Team::Null()");
    return;
  }

  if (pattern.team().size() == 1) {
    DASH_LOG_TRACE("dash::sort", "Sorting on a team with only 1 unit");
    trace.enter_state("1: final_local_sort");
    impl::local_sort(
        begin.local(), end.local(), sort_comp, nodeLevelConfig.parallelism());
    trace.exit_state("1: final_local_sort");
    return;
  }

  if (begin >= end) {
    DASH_LOG_TRACE("dash::sort", "empty range");
    trace.enter_state("1: final_barrier");
    pattern.team().barrier();
    trace.exit_state("1: final_barrier");
    return;
  }

  dash::Team& team   = pattern.team();
  auto const  nunits = team.size();
  auto const  myid   = team.myid();

  auto const unit_at_begin = pattern.unit_at(begin.pos());

  // local distance
  auto const l_range = dash::local_index_range(begin, end);

  auto* l_mem_begin = dash::local_begin(
      static_cast<typename GlobRandomIt::pointer>(begin), team.myid());

  auto const n_l_elem = l_range.end - l_range.begin;

  auto* lbegin = l_mem_begin + l_range.begin;
  auto* lend   = l_mem_begin + l_range.end;

  // initial local_sort
  trace.enter_state("1:initial_local_sort");
  impl::local_sort(lbegin, lend, sort_comp, nodeLevelConfig.parallelism());
  trace.exit_state("1:initial_local_sort");

  trace.enter_state("2:find_global_min_max");

  std::array<mapped_type, 2> min_max_in{
      // local minimum
      (n_l_elem > 0) ? sortable_hash(*lbegin)
                     : std::numeric_limits<mapped_type>::max(),
      (n_l_elem > 0) ? sortable_hash(*(std::prev(lend)))
                     : std::numeric_limits<mapped_type>::min()};

  std::array<mapped_type, 2> min_max_out{};

  DASH_ASSERT_RETURNS(
      dart_allreduce(
          &min_max_in,                              // send buffer
          &min_max_out,                             // receive buffer
          2,                                        // buffer size
          dash::dart_datatype<mapped_type>::value,  // data type
          DART_OP_MINMAX,                           // operation
          team.dart_id()                            // team
          ),
      DART_OK);

  auto const min_max = std::make_pair(
      min_max_out[DART_OP_MINMAX_MIN], min_max_out[DART_OP_MINMAX_MAX]);

  trace.exit_state("2:find_global_min_max");

  DASH_LOG_TRACE_VAR("global minimum in range", min_max.first);
  DASH_LOG_TRACE_VAR("global maximum in range", min_max.second);

  if (min_max.first == min_max.second) {
    // all values are equal, so nothing to sort globally.
    pattern.team().barrier();
    return;
  }

  trace.enter_state("3:init_temporary_local_data");

  std::vector<value_type> lcopy;

  decltype(lbegin) lcopy_begin = nullptr;

  auto const in_place = begin == out;

  if (n_l_elem) {
    if (in_place) {
      lcopy.reserve(n_l_elem);
      std::copy(lbegin, lend, std::back_inserter(lcopy));
      lcopy_begin = lcopy.data();
    }
    else {
      lcopy_begin = dash::local_begin(
          static_cast<typename GlobRandomIt::pointer>(out), team.myid());
    }
  }

  auto const p_unit_info =
      impl::psort__find_partition_borders(pattern, begin, end);

  auto const& acc_partition_count = p_unit_info.acc_partition_count;

  auto const nboundaries = nunits - 1;

  impl::Splitter<mapped_type> splitters(
      nboundaries, min_max.first, min_max.second);

  impl::psort__init_partition_borders(p_unit_info, splitters);

  DASH_LOG_TRACE_RANGE(
      "locally sorted array", lcopy_begin, lcopy_begin + n_l_elem);

  DASH_LOG_TRACE_RANGE(
      "skipped splitters",
      std::begin(splitters.is_skipped),
      std::end(splitters.is_skipped));

  bool done = false;

  // collect all valid splitters in a temporary vector
  std::vector<size_t> valid_partitions;

  {
    // make this as a separately scoped block to deallocate non-required
    // temporary memory
    std::vector<size_t> all_borders(nboundaries);
    std::iota(all_borders.begin(), all_borders.end(), 0);

    std::copy_if(
        all_borders.begin(),
        all_borders.end(),
        std::back_inserter(valid_partitions),
        [& is_skipped = splitters.is_skipped](size_t idx) {
          return is_skipped[idx] == false;
        });
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

  size_t iter = 0;

  std::vector<size_t> global_histo(nunits * NLT_NLE_BLOCK, 0);

  trace.exit_state("3:init_temporary_local_data");

  trace.enter_state("4:find_global_partition_borders");

  do {
    ++iter;

    impl::psort__calc_boundaries(splitters);

    DASH_LOG_TRACE_VAR("finding partition borders", iter);

    DASH_LOG_TRACE_RANGE(
        "splitters",
        std::begin(splitters.threshold),
        std::end(splitters.threshold));

    auto const l_nlt_nle = impl::psort__local_histogram(
        splitters,
        valid_partitions,
        lcopy_begin,
        lcopy_begin + n_l_elem,
        sortable_hash);

    DASH_LOG_TRACE_RANGE(
        "local histogram ( < )",
        impl::make_strided_iterator(std::begin(l_nlt_nle)),
        impl::make_strided_iterator(std::begin(l_nlt_nle)) + nunits);

    DASH_LOG_TRACE_RANGE(
        "local histogram ( <= )",
        impl::make_strided_iterator(std::begin(l_nlt_nle) + 1),
        impl::make_strided_iterator(std::begin(l_nlt_nle) + 1) + nunits);

    // allreduce with implicit barrier
    impl::psort__global_histogram(
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

    done = impl::psort__validate_partitions(
        p_unit_info, splitters, valid_partitions, global_histo);
  } while (!done);

  trace.exit_state("4:find_global_partition_borders");

  DASH_LOG_TRACE_VAR("partition borders found after N iterations", iter);

  /********************************************************************/
  /****** Final Histogram *********************************************/
  /********************************************************************/

  trace.enter_state("5:final_local_histogram");

  /* How many elements are less than P
   * or less than equals P */
  auto const histograms = impl::psort__local_histogram(
      splitters,
      valid_partitions,
      lcopy_begin,
      lcopy_begin + n_l_elem,
      sortable_hash);

  trace.exit_state("5:final_local_histogram");

  DASH_LOG_TRACE_RANGE(
      "final splitters",
      std::begin(splitters.threshold),
      std::end(splitters.threshold));

  DASH_LOG_TRACE_RANGE(
      "local histogram ( < )",
      impl::make_strided_iterator(std::begin(histograms)),
      impl::make_strided_iterator(std::begin(histograms)) + nunits);

  DASH_LOG_TRACE_RANGE(
      "local histogram ( <= )",
      impl::make_strided_iterator(std::begin(histograms) + 1),
      impl::make_strided_iterator(std::begin(histograms) + 1) + nunits);

  /********************************************************************/
  /****** Partition Distribution **************************************/
  /********************************************************************/

  /**
   * Each unit 0 <= p < P-1  is responsible for a final refinement around the
   * borders of bucket B_p.
   *
   * Parameters:
   * - Lower bound ( < S_p): The number of elements which definitely belong to
   *   Bucket p.
   * - Bucket size: Local capacity of unit u_p
   * - Uppoer bound ( <= S_p): The number of elements which eventually go into
   *   Bucket p.
   *
   * We first calculate the deficit (Bucket size - lower bound). If the
   * bucket is not fully exhausted (deficit > 0) we fill the space with
   * elements from the upper bound until the bucket is full.
   */

  trace.enter_state("6:transpose_local_histograms (all-to-all)");

  std::vector<size_t> g_partition_data(nunits * 2);

  DASH_ASSERT_RETURNS(
      dart_alltoall(
          // send buffer
          histograms.data(),
          // receive buffer
          g_partition_data.data(),
          // we send / receive 1 element to / from each process
          NLT_NLE_BLOCK,
          // dtype
          dash::dart_datatype<size_t>::value,
          // teamid
          team.dart_id()),
      DART_OK);

  DASH_LOG_TRACE_RANGE(
      "initial partition distribution",
      impl::make_strided_iterator(std::begin(g_partition_data)),
      impl::make_strided_iterator(std::begin(g_partition_data)) + nunits);

  DASH_LOG_TRACE_RANGE(
      "initial partition supply",
      impl::make_strided_iterator(std::begin(g_partition_data) + 1),
      impl::make_strided_iterator(std::begin(g_partition_data) + 1) + nunits);

  trace.exit_state("6:transpose_local_histograms (all-to-all)");

  /* Calculate final distribution per partition. Each unit is responsible for
   * its own bucket.
   */

  trace.enter_state("7:calc_final_partition_dist");

  auto first_nlt = impl::make_strided_iterator(std::begin(g_partition_data));

  auto first_nle =
      impl::make_strided_iterator(std::next(std::begin(g_partition_data)));

  impl::psort__calc_final_partition_dist(
      first_nlt,
      first_nlt + nunits,
      first_nle,
      acc_partition_count[myid + 1]);

  // let us now collapse the data into a contiguous range with unit stride
  std::move(
      impl::make_strided_iterator(std::begin(g_partition_data)) + 1,
      impl::make_strided_iterator(std::begin(g_partition_data)) + nunits,
      std::next(std::begin(g_partition_data)));

  DASH_LOG_TRACE_RANGE(
      "final partition distribution",
      std::next(std::begin(g_partition_data), IDX_DIST(nunits)),
      std::next(std::begin(g_partition_data), IDX_DIST(nunits) + nunits));

  trace.exit_state("7:calc_final_partition_dist");

  /********************************************************************/
  /****** Source Displacements ****************************************/
  /********************************************************************/

  /**
   * Based on the distribution we have to know the source displacements
   * (the offset where we have to read from in each unit). This is just a
   * ring-communication where each unit shift its local distribution downwards
   * to the succeeding neighbor.
   *
   * Worst Case Communication Complexity: O(P)
   * Memory Complexity: O(P)
   *
   * Only Units which contribute local elements participate in the
   * communication
   */

  trace.enter_state("8:comm_source_displs (sendrecv)");

  std::vector<size_t> source_displs(nunits, 0);

  auto neighbors =
      impl::psort__get_neighbors(myid, n_l_elem, splitters, valid_partitions);

  DASH_LOG_TRACE(
      "dash::sort",
      "shift partition dist",
      "my_source",
      neighbors.first,
      "my_target",
      neighbors.second);

  dart_sendrecv(
      std::next(g_partition_data.data(), IDX_DIST(nunits)),
      nunits,
      dash::dart_datatype<size_t>::value,
      101,
      // dest neighbor (right)
      neighbors.second,
      source_displs.data(),
      nunits,
      dash::dart_datatype<size_t>::value,
      101,
      // source neighbor (left)
      neighbors.first);

  DASH_LOG_TRACE_RANGE(
      "source displs", source_displs.begin(), source_displs.end());

  trace.exit_state("8:comm_source_displs (sendrecv)");

  /********************************************************************/
  /****** Target Counts ***********************************************/
  /********************************************************************/

  /**
   * Based on the distribution and the source displacements we can determine
   * the number of elemens we have to copy from each unit (target count) to
   * obtain the finally sorted sequence. This is just a mapping operation
   * where we calculcate for all elements 0 <= i < P:
   *
   * target_count[i] = partition_dist[i+1] - source_displacements[i]
   *
   * Communication Complexity: 0
   * Memory Complexity: O(P)
   */
  trace.enter_state("9:calc_target_offsets");

  std::vector<size_t> target_counts(nunits, 0);

  if (n_l_elem) {
    if (myid) {
      std::transform(
          // in_first
          std::next(g_partition_data.data(), IDX_DIST(nunits)),
          // in_last
          std::next(g_partition_data.data(), IDX_DIST(nunits) + nunits),
          // in_second
          std::begin(source_displs),
          // out_first
          std::begin(target_counts),
          // operation
          std::minus<size_t>());
    }
    else {
      std::copy(
          std::next(g_partition_data.data(), IDX_DIST(nunits)),
          std::next(g_partition_data.data(), IDX_DIST(nunits) + nunits),
          std::begin(target_counts));
    }
  }

  DASH_LOG_TRACE_RANGE(
      "target counts", target_counts.begin(), target_counts.end());

  /********************************************************************/
  /****** Target Counts ***********************************************/
  /********************************************************************/

  /**
   * Based on the target count we calculate the target displace (the offset to
   * which we have to copy remote data). This is just an exclusive scan with a
   * plus opertion.
   *
   * Communication Complexity: 0
   * Memory Complexity: O(P)
   */
  std::vector<size_t> target_displs(nunits + 1, 0);

  std::partial_sum(
      std::begin(target_counts),
      std::prev(std::end(target_counts)),
      std::begin(target_displs) + 1,
      std::plus<size_t>());

  target_displs.back() = n_l_elem;

  DASH_LOG_TRACE_RANGE(
      "target displs", target_displs.begin(), target_displs.end() - 1);

  trace.exit_state("9:calc_target_offsets");

  trace.enter_state("10:exchange_data (all-to-all)");

  /********************************************************************/
  /****** Exchange Data (All-To-All) **********************************/
  /********************************************************************/

  /**
   * Based on the information calculate above we initiate the data exchange.
   * Each process copies P chunks from each Process to the local portion.
   * Assuming all local portions are of equal local size gives us the
   * following complexity:
   *
   * Average Communication Traffic: O(N)
   * Aerage Comunication Overhead: O(P^2)
   */

  impl::ChunkDependencies chunk_dependencies;
  {
    auto const get_send_info =
        [&source_displs, &target_displs, &target_counts](
            dash::default_index_t const p_idx) {
          auto const target_disp  = target_displs[p_idx];
          auto const target_count = target_counts[p_idx];
          auto const src_disp     = source_displs[p_idx];
          return std::make_tuple(target_count, src_disp, target_disp);
        };

    // Note that this call is non-blocking (only enqueues the async_copies)
    auto copy_handles = impl::psort__exchange_data(
        begin,
        lcopy_begin,
        p_unit_info.valid_remote_partitions,
        get_send_info);

    // Schedule all these async copies for parallel processing in a thread
    // pool...
    chunk_dependencies = impl::psort__schedule_copy_tasks(
        lbegin,
        lcopy_begin,
        myid,
        p_unit_info.valid_remote_partitions,
        std::move(copy_handles),
        thread_pool,
        get_send_info);
  }

  /* NOTE: While merging locally sorted sequences is faster than another
   * heavy-weight sort it comes at a cost. std::inplace_merge allocates a
   * temporary buffer internally which is also documented on cppreference. If
   * the allocation of this buffer fails, a less efficient merge method is
   * used. However, in Linux, the allocation nevers fails since the
   * implementation simply allocates memory using malloc and the kernel
   * follows the optimistic strategy. This is ugly and can lead to a
   * segmentation fault later if no physical pages are available to map the
   * allocated virtual memory.
   *
   *
   * std::sort does not suffer from this problem and may be a more safe
   * variant, especially if the user wants to utilize the fully available
   * memory capacity on its own.
   */

  if (std::is_same<MergeStrategy, impl::sort__final_strategy__sort>::value) {
    // Wait for the final merge step
    impl::ChunkRange final_range(0, nunits);
    chunk_dependencies.at(final_range).get();
    trace.exit_state("10:exchange_data (all-to-all)");

    trace.enter_state("11:final_local_sort");
    impl::local_sort(
        lcopy_begin,
        lcopy_begin + n_l_elem,
        sort_comp,
        nodeLevelConfig.parallelism());
    trace.exit_state("11:final_local_sort");

    trace.enter_state("12:barrier");
    team.barrier();
    trace.exit_state("12:barrier");

    trace.enter_state("13:final_local_copy");
    std::copy(lcopy_begin, lcopy_begin + n_l_elem, lbegin);
    trace.exit_state("13:final_local_copy");
  }
  else {
    trace.exit_state("10:exchange_data (all-to-all)");

    trace.enter_state("11:merge_local_sequences");

    // Merge all asynchronous copies into a locally sorted range
    impl::psort__merge_local(
        lcopy_begin,
        lbegin,
        target_displs,
        chunk_dependencies,
        sort_comp,
        team,
        thread_pool,
        in_place);

    // Wait for the final merge step
    impl::ChunkRange final_range(0, nunits);
    chunk_dependencies.at(final_range).get();

    trace.exit_state("11:merge_local_sequences");
  }

  DASH_LOG_TRACE_RANGE("finally sorted range", lbegin, lend);

  trace.enter_state("final_barrier");
  team.barrier();
  trace.exit_state("final_barrier");
}

namespace impl {
template <typename T>
struct identity_t : std::unary_function<T, T> {
  constexpr T&& operator()(T&& t) const noexcept
  {
    // A perfect forwarding identity function
    return std::forward<T>(t);
  }
};
}  // namespace impl

template <class GlobRandomIt>
inline void sort(GlobRandomIt begin, GlobRandomIt end)
{
  using value_t = typename std::remove_cv<
      typename dash::iterator_traits<GlobRandomIt>::value_type>::type;

  dash::sort(begin, end, begin, impl::identity_t<value_t const&>());
}

#endif  // DOXYGEN

}  // namespace dash

#endif  // DASH__ALGORITHM__SORT_H
