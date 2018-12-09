#ifndef DASH__ALGORITHM__INTERNAL__SORT_H__INCLUDED
#define DASH__ALGORITHM__INTERNAL__SORT_H__INCLUDED

#include <algorithm>
#include <cstddef>
#include <limits>
#include <numeric>
#include <vector>

#include <dash/Array.h>
#include <dash/Types.h>
#include <dash/algorithm/internal/ParallelStl.h>
#include <dash/internal/Logging.h>

#include <dash/algorithm/sort/Types.h>

namespace dash {

namespace detail {

template <typename ElementType, typename InputIt, typename OutputIt>
inline void psort__calc_send_count(
    Splitter<ElementType> const& p_borders,
    std::vector<size_t> const&   valid_partitions,
    InputIt                      target_count,
    OutputIt                     send_count)
{
  using value_t = typename std::iterator_traits<InputIt>::value_type;

  static_assert(
      std::is_same<
          value_t,
          typename std::iterator_traits<OutputIt>::value_type>::value,
      "value types must be equal");

  DASH_LOG_TRACE("< psort__calc_send_count");

  // The number of units is the number of splitters + 1
  auto const           nunits = p_borders.lower_bound.size() + 1;
  std::vector<value_t> tmp_target_count;
  tmp_target_count.reserve(nunits + 1);
  tmp_target_count.emplace_back(0);

  std::copy(
      target_count,
      std::next(target_count, nunits),
      // we copy to index 1 since tmp_target_count[0] == 0
      std::back_inserter(tmp_target_count));

  auto tmp_target_count_begin = std::next(std::begin(tmp_target_count));

  auto const last_skipped = p_borders.is_skipped.cend();
  auto       it_skipped =
      std::find(p_borders.is_skipped.cbegin(), last_skipped, true);

  auto it_valid = valid_partitions.cbegin();

  std::size_t skipped_idx = 0;

  while (std::find(it_skipped, last_skipped, true) != last_skipped) {
    skipped_idx = std::distance(p_borders.is_skipped.cbegin(), it_skipped);

    it_valid =
        std::upper_bound(it_valid, valid_partitions.cend(), skipped_idx);

    if (it_valid == valid_partitions.cend()) {
      break;
    }

    auto const p_left         = p_borders.left_partition[*it_valid];
    auto const n_contig_skips = *it_valid - p_left;

    std::fill_n(
        std::next(tmp_target_count_begin, p_left + 1),
        n_contig_skips,
        *std::next(tmp_target_count_begin, p_left));

    std::advance(it_skipped, n_contig_skips);
    std::advance(it_valid, 1);
  }

  std::transform(
      tmp_target_count.begin() + 1,
      tmp_target_count.end(),
      tmp_target_count.begin(),
      send_count,
      std::minus<value_t>());

  DASH_LOG_TRACE("psort__calc_send_count >");
}

#if 0
template <typename ElementType>
inline void psort__calc_target_displs(
    Splitter<ElementType> const& p_borders,
    std::vector<size_t> const&   valid_partitions,
    dash::Array<size_t>&         g_partition_data)
{
  DASH_LOG_TRACE("< psort__calc_target_displs");
  auto const nunits = g_partition_data.team().size();
  auto const myid   = g_partition_data.team().myid();

  auto* l_target_displs = &(g_partition_data.local[IDX_TARGET_DISP(nunits)]);

  if (0 == myid) {
    // Unit 0 always writes to target offset 0
    std::fill(l_target_displs, l_target_displs + nunits, 0);
  }

  std::vector<size_t> target_displs(nunits, 0);

  auto const u_blocksize = g_partition_data.lsize();

  // What this algorithm does is basically an exclusive can over all send
  // counts across all participating units to find the target displacements of
  // a unit for all partitions. More precisely, each unit has to know the
  // starting offset in each partition where the elements should be copied to.
  //
  // Note: The one-sided approach here is
  // probably not the most efficient way. Something like dart_exscan should be
  // more efficient in large scale scenarios

  for (auto const& border_idx : valid_partitions) {
    auto const   left_u  = p_borders.left_partition[border_idx];
    auto const   right_u = border_idx + 1;
    size_t const val =
        (left_u == myid)
            ?
            /* if we are the bounding unit on the left-hand side we can access
             * the value in local memory */
            g_partition_data.local[left_u + IDX_SEND_COUNT(nunits)]
            :
            /* Otherwise we have to read the send count remotely from the
             * corresponding offset at the unit's memory */
            g_partition_data
                [left_u * u_blocksize + myid + IDX_SEND_COUNT(nunits)];
    target_displs[right_u] = val + target_displs[left_u];

    if (right_u == myid) {
      // we are local
      g_partition_data.local[IDX_TARGET_DISP(nunits) + myid] =
          target_displs[right_u];
    }
    else {
      auto const target_offset =
          right_u * u_blocksize + myid + IDX_TARGET_DISP(nunits);

      g_partition_data.async[target_offset].set(&(target_displs[right_u]));
    }
  }

  DASH_LOG_TRACE("psort__calc_target_displs >");
  g_partition_data.async.flush();
}
#endif


template <class RAI, class Cmp>
inline void local_sort(RAI first, RAI last, Cmp sort_comp, int nthreads = 1)
{
#ifdef DASH_ENABLE_PSTL
  if (nthreads > 1) {
    DASH_LOG_TRACE(
        "dash::sort", "local_sort", "Calling parallel sort using PSTL");
    ::std::sort(pstl::execution::par_unseq, first, last, sort_comp);
  }
  else {
    ::std::sort(first, last, sort_comp);
  }
#else
  DASH_LOG_TRACE("dash::sort", "local_sort", "Calling std::sort");
  ::std::sort(first, last, sort_comp);
#endif
}

template <size_t Stride, class Iter>
inline void log_strided_range(std::string ctx, std::array<Iter, 3> iter_tuple)
{
#ifdef DASH_ENABLE_TRACE_LOGGING
  using strided_iterator_t = detail::
      StridedIterator<typename std::vector<size_t>::const_iterator, Stride>;

  strided_iterator_t begin{// first valid iter in range
                           std::get<0>(iter_tuple),
                           // initial iter to iterate from
                           std::get<1>(iter_tuple),
                           // last valid iter in range
                           std::get<2>(iter_tuple)};
  strided_iterator_t end{// first valid iter in range
                         std::get<0>(iter_tuple),
                         // initial iter to iterate from
                         std::get<2>(iter_tuple),
                         // last valid iter in range
                         std::get<2>(iter_tuple)};

  DASH_LOG_TRACE_RANGE(ctx.c_str(), begin, end);
#endif
}

}  // namespace detail
}  // namespace dash
#endif
