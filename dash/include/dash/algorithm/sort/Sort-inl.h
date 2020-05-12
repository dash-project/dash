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

namespace impl {

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
  auto const           nunits = p_borders.count() + 1;
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
  // find the first empty partition
  auto it_skipped =
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

template <class ElementType>
inline auto psort__get_neighbors(
    dash::team_unit_t            whoami,
    std::size_t                  n_myelems,
    Splitter<ElementType> const& splitters,
    std::vector<size_t> const&   valid_partitions)
{
  auto it_left_splitter = std::lower_bound(
      std::begin(valid_partitions), std::end(valid_partitions), (whoami - 1));

  auto has_left_splitter =
      (n_myelems > 0) && whoami &&
              (it_left_splitter != std::end(valid_partitions))
          ? (*it_left_splitter == whoami - 1)
          : false;

  auto nunits = splitters.count() + 1;

  dash::global_unit_t my_source{
      (has_left_splitter) ? static_cast<dart_unit_t>(
                                splitters.left_partition[*it_left_splitter])
                          : DART_UNDEFINED_UNIT_ID};

  auto it_right_splitter = (n_myelems > 0 && whoami < nunits)
                               ? std::lower_bound(
                                     std::begin(valid_partitions),
                                     std::end(valid_partitions),
                                     whoami)
                               : std::end(valid_partitions);

  dash::global_unit_t my_target{
      (it_right_splitter != std::end(valid_partitions))
          ? static_cast<dart_unit_t>(*it_right_splitter) + 1
          : DART_UNDEFINED_UNIT_ID};

  return std::make_pair(my_source, my_target);
}
}  // namespace impl
}  // namespace dash
#endif
