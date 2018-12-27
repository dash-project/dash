#ifndef DASH__ALGORITHM__SORT__HISTOGRAM_H
#define DASH__ALGORITHM__SORT__HISTOGRAM_H

#include <dash/algorithm/sort/Types.h>
#include <dash/internal/Logging.h>

#include <algorithm>
#include <numeric>

namespace dash {
namespace impl {

template <typename Iter, typename MappedType, typename Projection>
inline const std::vector<std::size_t> psort__local_histogram(
    Splitter<MappedType> const& splitters,
    std::vector<size_t> const&  valid_partitions,
    Iter                        data_lbegin,
    Iter                        data_lend,
    Projection                  projection)
{
  DASH_LOG_TRACE("dash::sort", "< psort__local_histogram");

  auto const nborders = splitters.count();
  // The first element is 0 and the last element is the total number of local
  // elements in this unit
  auto const sz = splitters.count() + 1;
  // Number of elements less than P
  std::vector<std::size_t> l_nlt_nle(NLT_NLE_BLOCK * sz, 0);

  auto const n_l_elem = std::distance(data_lbegin, data_lend);

  // The value type of the iterator is not necessarily const, however, the
  // reference should definitely be. If that isn't the case the compiler
  // will complain anyway since our lambda required const qualifiers.
  using reference = typename std::iterator_traits<Iter>::reference;

  if (n_l_elem > 0) {
    for (auto const& idx : valid_partitions) {
      // search lower bound of partition value
      auto lb_it = std::lower_bound(
          data_lbegin,
          data_lend,
          splitters.threshold[idx],
          [&projection](reference a, const MappedType& b) {
            return projection(a) < b;
          });
      // search upper bound by starting from the lower bound
      auto ub_it = std::upper_bound(
          lb_it,
          data_lend,
          splitters.threshold[idx],
          [&projection](const MappedType& b, reference a) {
            return b < projection(a);
          });

      DASH_LOG_TRACE(
          "dash::sort",
          "local histogram",
          "distance between ub and lb",
          ub_it - lb_it);

      auto const p_left = splitters.left_partition[idx];
      DASH_ASSERT_NE(p_left, dash::team_unit_t{}, "invalid bounding unit");

      auto const nlt_idx = p_left * NLT_NLE_BLOCK;

      l_nlt_nle[nlt_idx]     = std::distance(data_lbegin, lb_it);
      l_nlt_nle[nlt_idx + 1] = std::distance(data_lbegin, ub_it);
    }

    auto const last_valid_border_idx = *std::prev(valid_partitions.cend());
    auto const p_left = splitters.left_partition[last_valid_border_idx];

    // fill trailing partitions with local capacity
    std::fill(
        std::next(std::begin(l_nlt_nle), (p_left + 1) * NLT_NLE_BLOCK),
        std::end(l_nlt_nle),
        n_l_elem);
  }

  DASH_LOG_TRACE("dash::sort", "psort__local_histogram >");
  return l_nlt_nle;
}

template <class InputIt, class OutputIt>
inline void psort__global_histogram(
    InputIt     local_histo_begin,
    InputIt     local_histo_end,
    OutputIt    output_it,
    dart_team_t dart_team_id)
{
  DASH_LOG_TRACE("dash::sort", "< psort__global_histogram ");

  auto const nels = std::distance(local_histo_begin, local_histo_end);

  dart_allreduce(
      &(*local_histo_begin),
      &(*output_it),
      nels,
      dash::dart_datatype<size_t>::value,
      DART_OP_SUM,
      dart_team_id);

  DASH_LOG_TRACE("dash::sort", "psort__global_histogram >");
}

}  // namespace impl
}  // namespace dash

#endif
