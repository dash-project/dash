#ifndef DASH__ALGORITHM__SORT__PARTITION_H
#define DASH__ALGORITHM__SORT__PARTITION_H

#include <dash/Team.h>
#include <dash/algorithm/sort/Types.h>
#include <dash/internal/Logging.h>
#include <dash/meta/NumericRange.h>

#include <algorithm>
#include <limits>
#include <numeric>
#include <vector>

namespace dash {

namespace impl {

template <class GlobIter>
inline auto psort__partition_sizes(GlobIter const begin, GlobIter const end)
{
  auto const& pattern = begin.pattern();

  auto nunits     = pattern.team().size();
  auto unit_begin = pattern.unit_at(begin.pos());
  auto unit_last  = pattern.unit_at(end.pos() - 1);

  std::vector<std::size_t> partition_sizes_psum;
  partition_sizes_psum.reserve(nunits + 1);

  auto local_extent = [&pattern](auto unit) {
    // Number of elements located at current source unit:
    auto const u_extents = pattern.local_extents(unit);

    return std::accumulate(
        std::begin(u_extents),
        std::end(u_extents),
        1,
        std::multiplies<std::size_t>());
  };

  auto gidx_begin = [&pattern](auto unit) {
    // global start index of local segment
    return (unit == pattern.team().myid()) ? pattern.lbegin()
                                           : pattern.global_index(unit, {});
  };

  // 1. fill leading partition with 0 until we reach the first non-empty
  // partition
  std::fill_n(
      std::back_inserter(partition_sizes_psum),
      unit_begin + 1,
      std::size_t{0});

  // 2. first unit: consider the case that we do not sort the full range but
  // start somewhere in the middle of the unit's segment
  auto ucap = local_extent(unit_begin);
  partition_sizes_psum.emplace_back(
      ucap == 0 ? 0 : ucap - (begin.pos() - gidx_begin(unit_begin)));

  if (unit_begin == unit_last) {
    return partition_sizes_psum;
  }

  // 3. units in the middle
  auto range = dash::meta::range(
      static_cast<dash::team_unit_t>(unit_begin + 1), unit_last);

  std::transform(
      std::begin(range),
      std::end(range),
      std::back_inserter(partition_sizes_psum),
      [local_extent](auto unit) -> std::size_t {
        // This is an inner unit
        // TODO(kowalewski): Is this really necessary or can we assume that
        // n_u_elements == u_size, i.e., local_pos.index == 0?
        //
        // auto const local_pos = pattern.local(u_gidx_begin);

        return local_extent(unit);
      });

  // 4. last unit: consider the case that we do not sort the full range but
  // end somewhere in the middle of the unit's segment
  partition_sizes_psum.emplace_back(end.pos() - gidx_begin(unit_last));

  std::fill_n(
      std::back_inserter(partition_sizes_psum),
      nunits - unit_last - 1,
      std::size_t{0});

  DASH_LOG_TRACE_RANGE(
      "partition sizes",
      std::begin(partition_sizes_psum),
      std::end(partition_sizes_psum));

  // calculate the prefix sum
  std::partial_sum(
      std::begin(partition_sizes_psum),
      std::end(partition_sizes_psum),
      std::begin(partition_sizes_psum));

  DASH_LOG_TRACE_RANGE(
      "partition sizes prefix sum",
      std::begin(partition_sizes_psum),
      std::end(partition_sizes_psum));

  return partition_sizes_psum;
}

template <typename T>
inline void psort__init_partition_borders(
    std::vector<size_t> const& acc_partition_count,
    impl::Splitter<T>&         p_borders)
{
  DASH_LOG_TRACE("dash::sort", "< psort__init_partition_borders");

  auto const last = acc_partition_count.cend();

  // find the first non-empty unit
  auto left =
      std::upper_bound(std::next(acc_partition_count.cbegin()), last, 0);

  if (left == last) {
    std::fill(p_borders.is_skipped.begin(), p_borders.is_skipped.end(), true);
    return;
  }

  // find next unit with a non-zero local portion to obtain first partition
  // border
  auto right = std::upper_bound(left, last, *left);

  if (right == last) {
    std::fill(p_borders.is_skipped.begin(), p_borders.is_skipped.end(), true);
    return;
  }

  auto const get_border_idx = [](std::size_t const idx) {
    return (idx % impl::lower_upper_block) ? (idx / impl::lower_upper_block) * impl::lower_upper_block
                                 : idx - 1;
  };

  auto p_left     = std::distance(acc_partition_count.cbegin(), left) - 1;
  auto right_u    = std::distance(acc_partition_count.cbegin(), right) - 1;
  auto border_idx = get_border_idx(right_u);

  // mark everything as skipped until the first partition border
  std::fill(
      p_borders.is_skipped.begin(),
      p_borders.is_skipped.begin() + border_idx,
      true);

  p_borders.left_partition[border_idx] = p_left;

  // find subsequent splitters
  left = right;

  while ((right = std::upper_bound(right, last, *right)) != last) {
    auto const last_border_idx = border_idx;

    p_left     = std::distance(acc_partition_count.cbegin(), left) - 1;
    right_u    = std::distance(acc_partition_count.cbegin(), right) - 1;
    border_idx = get_border_idx(right_u);

    auto const dist = border_idx - last_border_idx;

    // mark all skipped splitters as stable and skipped
    std::fill_n(
        std::next(p_borders.is_skipped.begin(), last_border_idx + 1),
        dist - 1,
        true);

    p_borders.left_partition[border_idx] = p_left;

    left = right;
  }

  // mark trailing empty parititons as stable and skipped
  std::fill(
      std::next(p_borders.is_skipped.begin(), border_idx + 1),
      p_borders.is_skipped.end(),
      true);

  std::copy(
      p_borders.is_skipped.begin(),
      p_borders.is_skipped.end(),
      p_borders.is_stable.begin());

  DASH_LOG_TRACE("dash::sort", "psort__init_partition_borders >");
}

template <typename T>
inline void psort__calc_boundaries(Splitter<T>& splitters)
{
  DASH_LOG_TRACE("dash::sort", "< psort__calc_boundaries ");

  // recalculate partition boundaries
  for (std::size_t idx = 0; idx < splitters.count(); ++idx) {
    DASH_ASSERT(splitters.lower_bound[idx] <= splitters.upper_bound[idx]);
    // case A: partition is already stable or skipped
    if (splitters.is_stable[idx]) {
      continue;
    }
    // case B: we have the last iteration
    //-> test upper bound directly
    if (splitters.is_last_iter[idx]) {
      splitters.threshold[idx] = splitters.upper_bound[idx];
      splitters.is_stable[idx] = true;
    }
    else {
      // case C: ordinary iteration

      splitters.threshold[idx] =
          splitters.lower_bound[idx] +
          ((splitters.upper_bound[idx] - splitters.lower_bound[idx]) / 2);

      if (splitters.threshold[idx] == splitters.lower_bound[idx]) {
        // if we cannot move the partition to the left
        //-> last iteration
        splitters.is_last_iter[idx] = true;
      }
    }
  }
  DASH_LOG_TRACE("dash::sort", "psort__calc_boundaries >");
}

template <typename ElementType>
inline bool psort__validate_partitions(
    Splitter<ElementType>&     splitters,
    std::vector<size_t> const& acc_partition_count,
    std::vector<size_t> const& valid_partitions,
    std::vector<size_t> const& global_histo)
{
  DASH_LOG_TRACE("dash::sort", "< psort__validate_partitions");

  if (valid_partitions.empty()) {
    return true;
  }

  // This validates if all partititions have been correctly determined. The
  // example below shows 4 units where unit 1 is empty (capacity 0). Thus
  // we have only two valid partitions, i.e. partition borders 1 and 2,
  // respectively. Partition 0 is skipped because the bounding unit on the
  // right-hand side is empty. For partition one, the bounding unit is unit 0,
  // one the right hand side it is 2.
  //
  // The right hand side unit is always (partition index + 1), the unit on
  // the left hand side is calculated at the beginning of dash::sort (@see
  // psort__init_partition_borders) and stored in a vector for lookup.
  //
  // Given this information the validation checks the following constraints
  //
  // - The number of elements in the global histrogram less than the
  //   partitition value must be smaller than the "accumulated" partition size
  // - The "accumulated" partition size must be less than or equal the number
  //   of elements which less than or equal the partition value
  //
  // If either of these two constraints cannot be satisfied we have to move
  // the upper or lower bound of the partition value, respectively.

  //                    -------|-------|-------|-------
  //   Partition Index     u0  |  u1   |   u2  |   u3
  //                    -------|-------|-------|-------
  //    Partition Size     10  |  0    |   10  |   10
  //                       ^           ^    ^
  //                       |           |    |
  //                       -------Partition--
  //                       |      Border 1  |
  //               Left Unit           |    Right Unit
  //                       |           |    |
  //                       |           |    |
  //                    -------|-------|-------|-------
  // Acc Partition Count   10  |  10   |   20  |  30
  //

  for (auto const& border_idx : valid_partitions) {
    auto const p_left  = splitters.left_partition[border_idx];
    auto const nlt_idx = p_left * impl::lower_upper_block;

    auto const peer_idx = p_left + 1;

    if (global_histo[nlt_idx] < acc_partition_count[peer_idx] &&
        acc_partition_count[peer_idx] <= global_histo[nlt_idx + 1]) {
      splitters.is_stable[border_idx] = true;
    }
    else {
      if (global_histo[nlt_idx] >= acc_partition_count[peer_idx]) {
        splitters.upper_bound[border_idx] = splitters.threshold[border_idx];
      }
      else {
        splitters.lower_bound[border_idx] = splitters.threshold[border_idx];
      }
    }
  }

  // Exit condition: is there any non-stable partition
  auto const nonstable_it = std::find(
      std::begin(splitters.is_stable), std::end(splitters.is_stable), false);

  DASH_LOG_TRACE("dash::sort", "psort__validate_partitions >");
  // exit condition
  return nonstable_it == splitters.is_stable.cend();
}

template <class Iter>
inline void psort__calc_final_partition_dist(
    Iter                                            nlt_first,
    Iter                                            nlt_last,
    Iter                                            nle_first,
    typename std::iterator_traits<Iter>::value_type partition_size)
{
  using value_t = typename std::iterator_traits<Iter>::value_type;

  /* Calculate number of elements to receive for each partition:
   * We first assume that we we receive exactly the number of elements which
   * are less than P.
   * The output are the end offsets for each partition
   */
  DASH_LOG_TRACE("dash::sort", "< psort__calc_final_partition_dist");

  auto const nunits = std::distance(nlt_first, nlt_last);

  auto const n_my_elements = std::accumulate(nlt_first, nlt_last, value_t{0});

  // Calculate the deficit
  auto my_deficit = partition_size - n_my_elements;

  // If there is a deficit, look how much unit j can supply
  for (auto unit = dash::team_unit_t{0}; unit < nunits && my_deficit > 0;
       ++unit, ++nlt_first, ++nle_first) {
    auto const supply_unit = *nle_first - *nlt_first;

    DASH_ASSERT_GE(supply_unit, 0, "invalid supply of target unit");
    if (supply_unit <= my_deficit) {
      *(nlt_first) += supply_unit;
      my_deficit -= supply_unit;
    }
    else {
      *(nlt_first) += my_deficit;
      my_deficit = 0;
    }
  }

  DASH_ASSERT_GE(my_deficit, 0, "Invalid local deficit");
  DASH_LOG_TRACE("dash::sort", "psort__calc_final_partition_dist >");
}

}  // namespace impl
}  // namespace dash

#endif
