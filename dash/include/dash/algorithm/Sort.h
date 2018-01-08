#ifndef DASH__ALGORITHM__SORT_H__
#define DASH__ALGORITHM__SORT_H__

#include <algorithm>
#include <functional>
#include <iterator>
#include <type_traits>
#include <vector>

#include <dash/Array.h>
#include <dash/Atomic.h>
#include <dash/Exception.h>
#include <dash/GlobPtr.h>
#include <dash/dart/if/dart.h>

#include <dash/algorithm/Copy.h>
#include <dash/algorithm/Fill.h>
#include <dash/algorithm/LocalRange.h>
#include <dash/algorithm/MinMax.h>
#include <dash/algorithm/Transform.h>

namespace dash {

#ifdef DOXYGEN

/**
 * Sorts the elements in the range, defined by \c [begin, end) in ascending
 * order. The order of equal elements is not guaranteed to be preserved.
 *
 * In terms of data distribution, source and destination ranges passed to
 * \c dash::sort must be global (\c GlobIter<ValueType>).
 *
 * The operation is collective among the team of the owning dash container.
 *
 * Example:
 *
 * \code
 *       dash::sort(array.begin(),
 *                  array.end();
 * \endcode
 *
 * \ingroup  DashAlgorithms
 */
template <class GlobRandomIt>
void sort(GlobRandomIt begin, GlobRandomIt end);

#else

#ifdef DASH_ENABLE_TRACE_LOGGING
#define MAX_ELEMS_LOGGING 25
#define DASH_SORT_LOG_TRACE_RANGE(desc, begin, end)                         \
  do {                                                                      \
    using value_t =                                                         \
        typename std::iterator_traits<decltype(begin)>::value_type;         \
    using difference_t =                                                    \
        typename std::iterator_traits<decltype(begin)>::difference_type;    \
    auto const nelems = std::distance(begin, end);                          \
    auto const max_elems =                                                  \
        std::min<difference_t>(nelems, MAX_ELEMS_LOGGING);                  \
    std::ostringstream os;                                                  \
    std::copy(                                                              \
        begin, begin + max_elems, std::ostream_iterator<value_t>(os, " ")); \
    if (nelems > MAX_ELEMS_LOGGING) os << "...";                            \
    DASH_LOG_TRACE(desc, os.str());                                         \
  } while (0)
#else
#define DASH_SORT_LOG_TRACE_RANGE(desc, begin, end) \
  do {                                              \
  } while (0)
#endif

#define OFF_DIST(nunits) (nunits * 0)
#define OFF_SUPP(nunits) (nunits * 1)
#define OFF_DISP(nunits) (nunits * 2)

#define NLT_NLE_BLOCK 2

namespace detail {

struct UnitInfo {
  std::size_t nunits;
  // prefix sum over the number of local elements of all unit
  std::vector<size_t>            acc_unit_count;
  std::vector<dash::team_unit_t> contributing_remote_units;

  UnitInfo(std::size_t p_nunits)
    : nunits(p_nunits)
    , acc_unit_count(nunits + 1)
  {
    contributing_remote_units.reserve(nunits - 1);
  }
};

template <typename T>
struct PartitionBorder {
public:
  // tracks if we have found a stable partition border
  std::vector<bool> is_stable;
  // tracks if a partition is skipped
  std::vector<bool> is_skipped;
  // lower bound of each partition
  std::vector<T> lower_bound;
  // upper bound of each partition
  std::vector<T> upper_bound;
  // Special case for the last iteration in finding partition borders
  std::vector<bool> is_last_iter;

  std::vector<std::pair<dash::team_unit_t, dash::team_unit_t> >
      bounding_units;

  PartitionBorder(size_t nsplitter, T _lower_bound, T _upper_bound)
    : is_stable(nsplitter, false)
    , is_skipped(nsplitter, false)
    , lower_bound(nsplitter, _lower_bound)
    , upper_bound(nsplitter, _upper_bound)
    , is_last_iter(nsplitter, false)
    , bounding_units(
          nsplitter, std::make_pair(dash::team_unit_t{}, dash::team_unit_t{}))
  {
  }
};

template <typename T>
inline void psort__calc_boundaries(
    PartitionBorder<T>& p_borders, std::vector<T>& partitions)
{
  DASH_LOG_TRACE("< psort__calc_boundaries ");
  DASH_ASSERT_EQ(
      p_borders.is_stable.size(), partitions.size(),
      "invalid number of partition borders");
  // recalculate partition boundaries
  for (std::size_t idx = 0; idx < partitions.size(); ++idx) {
    // case A: partition is already stable or skipped
    if (p_borders.is_stable[idx]) continue;
    // case B: we have the last iteration
    //-> test upper bound directly
    if (p_borders.is_last_iter[idx]) {
      partitions[idx]          = p_borders.upper_bound[idx];
      p_borders.is_stable[idx] = true;
    }
    else {
      // case C: ordinary iteration
      partitions[idx] =
          (p_borders.lower_bound[idx] + p_borders.upper_bound[idx]) / 2;

      if (partitions[idx] == p_borders.lower_bound[idx]) {
        // if we cannot move the partition to the left
        //-> last iteration
        p_borders.is_last_iter[idx] = true;
      }
    }
  }
  DASH_LOG_TRACE("psort__calc_boundaries >");
}

template <typename ElementType, typename SizeType>
inline std::pair<std::vector<SizeType>, std::vector<SizeType> >
psort__local_histogram(
    std::vector<ElementType> const&     partitions,
    std::vector<size_t> const&          valid_partitions,
    PartitionBorder<ElementType> const& p_borders,
    ElementType const*                  lbegin,
    ElementType const*                  lend)
{
  DASH_LOG_TRACE("< psort__local_histogram ");
  auto const nborders = partitions.size();
  // The first element is 0 and the last element is the total number of local
  // elements in this unit
  auto const sz = partitions.size() + 2;
  // Number of elements less than P
  std::vector<SizeType> n_lt(sz, 0);
  // Number of elements less than equals P
  std::vector<SizeType> n_le(sz, 0);

  auto const n_l_elem = std::distance(lbegin, lend);

  if (n_l_elem == 0) return std::make_pair(n_lt, n_le);

  for (auto const& idx : valid_partitions) {
    auto lb_it = std::lower_bound(lbegin, lend, partitions[idx]);
    auto ub_it = std::upper_bound(lbegin, lend, partitions[idx]);

    auto const left_u = p_borders.bounding_units[idx].first;
    DASH_ASSERT_NE(left_u, dash::team_unit_t{}, "invalid bounding unit");

    n_lt[left_u + 1] = std::distance(lbegin, lb_it);
    n_le[left_u + 1] = std::distance(lbegin, ub_it);
  }

  auto const last_valid_border_idx = *std::prev(valid_partitions.cend(), 1);

  auto const last_unit =
      p_borders.bounding_units[last_valid_border_idx].first;

  std::fill(
      std::next(std::begin(n_lt), last_unit + 2), std::end(n_lt), n_l_elem);
  std::fill(
      std::next(std::begin(n_le), last_unit + 2), std::end(n_le), n_l_elem);

  DASH_LOG_TRACE("psort__local_histogram >");
  return std::make_pair(n_lt, n_le);
}

template <typename SizeType, typename GlobIter>
inline void psort__global_histogram(
    std::vector<SizeType> const& l_nlt,
    std::vector<SizeType> const& l_nle,
    std::vector<size_t> const&   valid_partitions,
    GlobIter                     it_nlt_nle)
{
  DASH_LOG_TRACE("< psort__global_histogram ");
  DASH_ASSERT_EQ(l_nlt.size(), l_nle.size(), "Sizes must match");

  auto const& team = it_nlt_nle.pattern().team();
  auto const  myid = team.myid();

  auto liter_begin = (it_nlt_nle + (myid.id * NLT_NLE_BLOCK)).local();

  DASH_ASSERT_MSG(liter_begin, "pointer must not be NULL");

  std::fill(liter_begin, liter_begin + NLT_NLE_BLOCK, 0);

  team.barrier();

  auto const last_valid_border = *std::prev(valid_partitions.cend(), 1);

  // TODO: implement dash::transform_async
  // We have to add 2
  // --> +1 because we start at idx 1
  // --> +1 to get the last idx
  for (std::size_t idx = 1; idx < last_valid_border + 2; ++idx) {
    // we communicate only non-zero values
    if (l_nlt[idx] == 0 && l_nle[idx] == 0) continue;

    std::array<SizeType, NLT_NLE_BLOCK> vals{{l_nlt[idx], l_nle[idx]}};
    auto const                          g_idx_nlt = (idx - 1) * NLT_NLE_BLOCK;

    dash::transform<SizeType>(
        &(*std::begin(vals)),  // A
        &(*std::end(vals)),
        it_nlt_nle + g_idx_nlt,   // B
        it_nlt_nle + g_idx_nlt,   // B = op(B,A)
        dash::plus<SizeType>());  // op
  }

  team.barrier();
  DASH_LOG_TRACE("psort__global_histogram >");
}

template <typename ElementType, typename GlobIter>
inline bool psort__validate_partitions(
    UnitInfo const&                 p_unit_info,
    std::vector<ElementType> const& partitions,
    std::vector<size_t> const&      valid_partitions,
    PartitionBorder<ElementType>&   p_borders,
    GlobIter                        it_g_nlt_nle)
{
  DASH_LOG_TRACE("< psort__validate_partitions");
  using value_t = typename GlobIter::value_type;
  using index_t = typename GlobIter::index_type;

  if (valid_partitions.size() == 0) return true;

  auto const nunits = it_g_nlt_nle.pattern().team().size();
  auto const myid   = it_g_nlt_nle.pattern().team().myid();

  //
  // The first two elements are always 0
  std::vector<value_t> l_nlt_nle(NLT_NLE_BLOCK * nunits, 0);
  auto                 l_nlt_nle_data = l_nlt_nle.data();

  std::vector<dash::Future<value_t*> > futures;

  std::transform(
      p_unit_info.contributing_remote_units.begin(),
      p_unit_info.contributing_remote_units.end(),
      std::back_inserter(futures),
      [it_g_nlt_nle, l_nlt_nle_data](std::size_t unit) {
        index_t lidx      = unit * NLT_NLE_BLOCK;
        auto    cpy_begin = it_g_nlt_nle + lidx;
        auto    cpy_end   = cpy_begin + NLT_NLE_BLOCK;
        // TODO: dash::copy does not work with BLOCKCYCLIC patterns.
        // TODO: add unit test and create issue
        //
        //--> That is also the reason why we explicitly use
        // dash::internal::copy_async_impl instead of dash::copy_async
        return dash::internal::copy_async_impl(
            cpy_begin, cpy_end, l_nlt_nle_data + lidx);
      });

  // copy local portion
  index_t lidx      = myid * NLT_NLE_BLOCK;
  auto    cpy_begin = it_g_nlt_nle + lidx;
  auto    lbegin    = cpy_begin.local();
  std::copy(lbegin, lbegin + NLT_NLE_BLOCK, l_nlt_nle_data + lidx);

  for (auto& fut : futures) {
    fut.wait();
  }

  auto const& acc_unit_count = p_unit_info.acc_unit_count;

  for (auto const& border_idx : valid_partitions) {
    auto const left_u  = p_borders.bounding_units[border_idx].first;
    auto const nlt_idx = left_u * NLT_NLE_BLOCK;

    auto const peer_idx = left_u + 1;

    if (l_nlt_nle[nlt_idx] < acc_unit_count[peer_idx] &&
        acc_unit_count[peer_idx] <= l_nlt_nle[nlt_idx + 1]) {
      p_borders.is_stable[border_idx] = true;
    }
    else {
      if (l_nlt_nle[nlt_idx] >= acc_unit_count[peer_idx]) {
        p_borders.upper_bound[border_idx] = partitions[border_idx];
      }
      else {
        p_borders.lower_bound[border_idx] = partitions[border_idx];
      }
    }
  }

  // Exit condition: is there any non-stable partition
  auto const nonstable_it = std::find(
      p_borders.is_stable.cbegin(), p_borders.is_stable.cend(), false);

  DASH_LOG_TRACE("psort__validate_partitions >");
  // exit condition
  return nonstable_it == p_borders.is_stable.cend();
}

template <typename LocalArrayT>
inline void psort__calc_final_partition_dist(
    std::vector<typename LocalArrayT::value_type> const& acc_unit_count,
    LocalArrayT&                                         l_partition_dist)
{
  /* Calculate number of elements to receive for each partition:
   * We first assume that we we receive exactly the number of elements which
   * are less than P.
   * The output are the end offsets for each partition
   */
  DASH_LOG_TRACE("< psort__calc_final_partition_dist");

  auto const myid       = l_partition_dist.pattern().team().myid();
  auto const nunits     = l_partition_dist.pattern().team().size();
  auto       dist_begin = l_partition_dist.begin() + OFF_DIST(nunits);
  auto       supp_begin = l_partition_dist.begin() + OFF_SUPP(nunits);

  auto const n_my_elements =
      std::accumulate(dist_begin, dist_begin + nunits, 0);

  // Calculate the deficit
  auto my_deficit = acc_unit_count[myid + 1] - n_my_elements;

  dash::team_unit_t       unit(0);
  dash::team_unit_t const last(nunits);

  // If there is a deficit, look how much unit j can supply
  for (; unit < last && my_deficit > 0; ++unit) {
    auto const supply_unit = *(supp_begin + unit) - *(dist_begin + unit);

    DASH_ASSERT_GE(supply_unit, 0, "invalid supply of target unit");
    if (supply_unit <= my_deficit) {
      *(dist_begin + unit) += supply_unit;
      my_deficit -= supply_unit;
    }
    else {
      *(dist_begin + unit) += my_deficit;
      my_deficit = 0;
    }
  }

  DASH_ASSERT_GE(my_deficit, 0, "Invalid local deficit");
  DASH_LOG_TRACE("psort__calc_final_partition_dist >");
}

template <typename LocalArrayT, typename ElementType>
inline void psort__calc_send_count(
    PartitionBorder<ElementType> const&            p_borders,
    std::vector<size_t> const&                     valid_partitions,
    LocalArrayT&                                   partition_dist,
    std::vector<typename LocalArrayT::value_type>& l_send_displs)
{
  using value_t = typename LocalArrayT::value_type;
  DASH_LOG_TRACE("< psort__calc_send_count");

  auto const           nunits = partition_dist.pattern().team().size();
  std::vector<value_t> target_count(nunits + 1);

  target_count[0] = 0;

  auto l_target_count = &(partition_dist[OFF_SUPP(nunits)]);
  auto l_send_count   = &(partition_dist[OFF_DIST(nunits)]);

  auto const last_skipped = p_borders.is_skipped.cend();
  auto       it_skipped =
      std::find(p_borders.is_skipped.cbegin(), last_skipped, true);

  auto it_valid = valid_partitions.cbegin();

  std::size_t skipped_idx = 0;

  while (std::find(it_skipped, last_skipped, true) != last_skipped) {
    skipped_idx = std::distance(p_borders.is_skipped.cbegin(), it_skipped);

    it_valid =
        std::upper_bound(it_valid, valid_partitions.cend(), skipped_idx);

    if (it_valid == valid_partitions.cend()) break;

    auto const left_u         = p_borders.bounding_units[*it_valid].first;
    auto const n_contig_skips = *it_valid - left_u;
    std::fill_n(
        l_target_count + left_u + 1, n_contig_skips, l_target_count[left_u]);

    std::advance(it_skipped, n_contig_skips);
    std::advance(it_valid, 1);
  }

  std::copy(
      l_target_count, l_target_count + nunits, target_count.begin() + 1);

  std::transform(
      target_count.begin() + 1, target_count.end(), target_count.begin(),
      l_send_count, std::minus<value_t>());

  DASH_ASSERT_EQ(l_send_displs.size(), nunits, "invalid vector size");

  l_send_displs[0] = 0;

  std::transform(
      l_send_count, l_send_count + nunits - 1, l_send_displs.begin(),
      l_send_displs.begin() + 1, std::plus<value_t>());
  DASH_LOG_TRACE("psort__calc_send_count >");
}

template <typename SizeType>
inline void psort__calc_target_displs(dash::Array<SizeType>& g_partition_data)
{
  DASH_LOG_TRACE("< psort__calc_target_displs");
  auto const nunits = g_partition_data.team().size();
  auto const myid   = g_partition_data.team().myid();

  auto l_target_displs = &(g_partition_data.local[OFF_DISP(nunits)]);

  if (0 == myid) {
    // Unit 0 always writes to target offset 0
    std::fill(l_target_displs, l_target_displs + nunits, 0);
  }

  std::vector<SizeType> target_displs(nunits, 0);

  team_unit_t       unit(1);
  team_unit_t const last(nunits);

  auto const u_blocksize          = g_partition_data.lsize();
  auto const target_displs_lbegin = myid * u_blocksize + OFF_DISP(nunits);
  auto const target_displs_lend   = target_displs_lbegin + nunits;

  for (; unit < last; ++unit) {
    auto const     prev_u = unit - 1;
    SizeType const val =
        (prev_u == myid)
            ? g_partition_data.local[prev_u + OFF_DIST(nunits)]
            : g_partition_data
                  [prev_u * u_blocksize + myid + OFF_DIST(nunits)];
    target_displs[unit]      = val + target_displs[prev_u];
    auto const target_offset = unit * u_blocksize + myid + OFF_DISP(nunits);
    if (target_displs_lbegin <= target_offset &&
        target_offset < target_displs_lend) {
      g_partition_data.local[(target_offset % nunits) + OFF_DISP(nunits)] =
          target_displs[unit];
    }
    else {
      g_partition_data.async[target_offset].set(&(target_displs[unit]));
    }
  }

  DASH_LOG_TRACE("psort__calc_target_displs >");
  g_partition_data.async.flush();
}

template <typename GlobIterT>
inline UnitInfo psort__setup_unit_info(
    typename GlobIterT::pattern_type const& pattern,
    GlobIterT const                         begin,
    GlobIterT const                         end)
{
  DASH_LOG_TRACE("< psort__setup_unit_info");

  auto const nunits = pattern.team().size();
  auto const myid   = pattern.team().myid();

  dash::team_unit_t       unit{0};
  const dash::team_unit_t last{static_cast<dart_unit_t>(nunits)};

  auto const unit_first = pattern.unit_at(begin.pos());
  auto const unit_last  = pattern.unit_at(end.pos() - 1);

  // Starting offsets of all units
  UnitInfo unit_info(nunits);
  auto&    acc_unit_count = unit_info.acc_unit_count;
  acc_unit_count[0]       = 0;

  for (; unit < last; ++unit) {
    // Number of elements located at current source unit:
    auto const u_size = pattern.local_size(unit);
    // first linear global index of unit
    auto const u_gidx_begin = pattern.global_index(unit, {});
    // last global index of unit
    auto const u_gidx_end = u_gidx_begin + u_size;

    DASH_LOG_TRACE(
        "local indexes", unit, ": ", u_gidx_begin, " ", u_size, " ",
        u_gidx_end);

    if (u_size == 0 || u_gidx_end - 1 < begin.pos() ||
        u_gidx_begin >= end.pos()) {
      // This unit does not participate...
      acc_unit_count[unit + 1] = acc_unit_count[unit];
    }
    else {
      std::size_t n_u_elements;
      if (unit == unit_last) {
        // The local range of this unit has the global end
        n_u_elements = end.pos() - u_gidx_begin;
      }
      else if (unit == unit_first) {
        // The local range of this unit has the global begin
        auto const u_begin_disp = begin.pos() - u_gidx_begin;
        n_u_elements            = u_size - u_begin_disp;
      }
      else {
        // This is an inner unit
        // TODO: Is this really necessary or can we assume that
        // n_u_elements == u_size, i.e., local_pos.index == 0?
        auto const local_pos = pattern.local(u_gidx_begin);

        n_u_elements = u_size - local_pos.index;

        DASH_ASSERT_EQ(local_pos.unit, unit, "units must match");
      }

      acc_unit_count[unit + 1] = n_u_elements + acc_unit_count[unit];
      if (unit != myid) unit_info.contributing_remote_units.push_back(unit);
    }
  }

  DASH_LOG_TRACE("psort__setup_unit_info >");
  return unit_info;
}

template <typename T>
inline detail::PartitionBorder<T> psort__setup_partition_borders(
    UnitInfo& unit_info, T min, T max)
{
  DASH_LOG_TRACE("< psort__setup_partition_borders");
  auto const                 nunits = unit_info.nunits;
  detail::PartitionBorder<T> p_borders(nunits - 1, min, max);

  auto const& acc_unit_count = unit_info.acc_unit_count;

  auto const last = acc_unit_count.cend();

  // find the first non-empty unit
  auto left = std::upper_bound(std::next(acc_unit_count.cbegin()), last, 0);

  if (left == last) {
    std::fill(p_borders.is_skipped.begin(), p_borders.is_skipped.end(), true);
    return p_borders;
  }

  // find next unit with a non-zero local portion to obtain first partition
  // border
  auto right = std::upper_bound(left, last, *left);

  if (right == last) {
    std::fill(p_borders.is_skipped.begin(), p_borders.is_skipped.end(), true);
    return p_borders;
  }

  auto const get_border_idx = [](std::size_t const& idx) {
    return (idx % NLT_NLE_BLOCK) ? (idx / NLT_NLE_BLOCK) * NLT_NLE_BLOCK
                                 : idx - 1;
  };

  auto left_u     = std::distance(acc_unit_count.cbegin(), left) - 1;
  auto right_u    = std::distance(acc_unit_count.cbegin(), right) - 1;
  auto border_idx = get_border_idx(right_u);

  // mark everything as skipped until the first partition border
  std::fill(
      p_borders.is_skipped.begin(), p_borders.is_skipped.begin() + border_idx,
      true);

  p_borders.bounding_units[border_idx] = std::make_pair(
      dash::team_unit_t{static_cast<dart_unit_t>(left_u)},
      dash::team_unit_t{static_cast<dart_unit_t>(right_u)});

  // unit_info.border_peer[left_u] = right_u;
  // unit_info.border[left_u]      = border_idx;

  // find subsequent partitions
  left = right;

  while ((right = std::upper_bound(right, last, *right)) != last) {
    auto const last_border_idx = border_idx;

    left_u     = std::distance(acc_unit_count.cbegin(), left) - 1;
    right_u    = std::distance(acc_unit_count.cbegin(), right) - 1;
    border_idx = get_border_idx(right_u);

    auto const dist = border_idx - last_border_idx;

    // mark all skipped partitions as stable and skipped
    std::fill_n(
        std::next(p_borders.is_skipped.begin(), last_border_idx + 1),
        dist - 1, true);

    // unit_info.border_peer[left_u] = right_u;
    // unit_info.border[left_u]      = border_idx;
    p_borders.bounding_units[border_idx] = std::make_pair(
        dash::team_unit_t{static_cast<dart_unit_t>(left_u)},
        dash::team_unit_t{static_cast<dart_unit_t>(right_u)});

    left = right;
  }

  // mark trailing empty parititons as stable and skipped
  std::fill(
      std::next(p_borders.is_skipped.begin(), border_idx + 1),
      p_borders.is_skipped.end(), true);

  std::copy(
      p_borders.is_skipped.begin(), p_borders.is_skipped.end(),
      p_borders.is_stable.begin());

  DASH_LOG_TRACE("psort__setup_partition_borders >");
  return p_borders;
}

}  // namespace detail

template <class GlobRandomIt>
void sort(GlobRandomIt begin, GlobRandomIt end)
{
  using iter_type    = GlobRandomIt;
  using pattern_type = typename iter_type::pattern_type;
  using size_type    = typename pattern_type::size_type;
  using value_type   = typename iter_type::value_type;

  static_assert(
      std::is_arithmetic<value_type>::value,
      "Only arithmetic types are supported");

  auto pattern = begin.pattern();

  if (pattern.team() == dash::Team::Null()) {
    DASH_LOG_TRACE("dash::sort", "Sorting on dash::Team::Null()");
    return;
  }
  else if (pattern.team().size() == 1) {
    DASH_LOG_TRACE("dash::sort", "Sorting on a team with only 1 unit");
    std::sort(begin.local(), end.local());
    return;
  }

  // TODO: instead of std::sort we may accept a user-defined local sort
  // function. Alterantively, we may use a parallel sort algorithm for the
  // local portion

  if (begin >= end) {
    DASH_LOG_TRACE("dash::sort", "empty range");
    pattern.team().barrier();
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

  // initial local sort
  std::sort(lbegin, lend);

  // Temporary local buffer (sorted);
  std::vector<value_type> const lcopy(lbegin, lend);

  auto const lmin =
      (n_l_elem > 0) ? *lbegin : std::numeric_limits<value_type>::max();
  auto const lmax =
      (n_l_elem > 0) ? *(lend - 1) : std::numeric_limits<value_type>::min();

  dash::Shared<dash::Atomic<value_type> > g_min(dash::team_unit_t{0}, team);
  dash::Shared<dash::Atomic<value_type> > g_max(dash::team_unit_t{0}, team);

  g_min.get().op(dash::min<value_type>(), lmin);
  g_max.get().op(dash::max<value_type>(), lmax);
  // No subsequent barriers are needed for Shared due to container
  // constructors...

  using array_t = dash::Array<size_type>;

  std::size_t gsize = nunits * NLT_NLE_BLOCK * 2;

  array_t g_nlt_nle(gsize, dash::BLOCKCYCLIC(NLT_NLE_BLOCK), team);

  array_t g_partition_data(nunits * nunits * 3, dash::BLOCKED, team);
  std::fill(g_partition_data.lbegin(), g_partition_data.lend(), 0);
  DASH_ASSERT_GT(
      g_partition_data.size(), nunits * 2 * 2,
      "array size is not large enough");

  auto const min = static_cast<value_type>(g_min.get());
  auto const max = static_cast<value_type>(g_max.get());

  auto p_unit_info = detail::psort__setup_unit_info(pattern, begin, end);
  auto const& l_acc_unit_count = p_unit_info.acc_unit_count;

  auto const              nboundaries = nunits - 1;
  std::vector<value_type> partitions(nboundaries, 0);

  auto p_borders =
      detail::psort__setup_partition_borders(p_unit_info, min, max);

  DASH_SORT_LOG_TRACE_RANGE("locally sorted array", lbegin, lend);
  DASH_SORT_LOG_TRACE_RANGE(
      "skipped partitions", p_borders.is_skipped.cbegin(),
      p_borders.is_skipped.cend());

  bool done = false;

  auto cur = g_nlt_nle.begin();
  auto tmp = cur + NLT_NLE_BLOCK * nunits;

  // collect all valid partitions in a temporary vector
  std::vector<size_t> valid_partitions;

  {
    // make this as a separately scoped block to deallocate non-required
    // temporary memory
    std::vector<size_t> all_borders(partitions.size());
    std::iota(all_borders.begin(), all_borders.end(), 0);

    auto const& is_skipped = p_borders.is_skipped;

    std::copy_if(
        all_borders.begin(), all_borders.end(),
        std::back_inserter(valid_partitions),
        [&is_skipped](size_t idx) { return is_skipped[idx] == false; });
  }

  if (valid_partitions.size() == 0) {
    // Edge case: We may have a team spanning at least 2 units, however the
    // global range is owned by  only 1 unit
    team.barrier();
    return;
  }

  do {
    detail::psort__calc_boundaries(p_borders, partitions);

    auto const histograms =
        detail::psort__local_histogram<value_type, size_type>(
            partitions, valid_partitions, p_borders, lbegin, lend);

    auto const& l_nlt = histograms.first;
    auto const& l_nle = histograms.second;

    DASH_SORT_LOG_TRACE_RANGE(
        "local histogram nlt", l_nlt.begin(), l_nlt.end());

    DASH_SORT_LOG_TRACE_RANGE(
        "local histogram nle", l_nle.begin(), l_nle.end());

    detail::psort__global_histogram(l_nlt, l_nle, valid_partitions, cur);

    DASH_SORT_LOG_TRACE_RANGE(
        "global histogram", (cur + (myid * NLT_NLE_BLOCK)).local(),
        (cur + (myid * NLT_NLE_BLOCK)).local() + NLT_NLE_BLOCK);

    done = detail::psort__validate_partitions(
        p_unit_info, partitions, valid_partitions, p_borders, cur);

    // This swap eliminates a barrier as, otherwise, some units may be one
    // iteration ahead and modify shared data while the others are still not
    // done
    std::swap(cur, tmp);

  } while (!done);

  auto histograms = detail::psort__local_histogram<value_type, size_type>(
      partitions, valid_partitions, p_borders, lbegin, lend);

  /* How many elements are less than P
   * or less than equals P */
  auto& l_nlt = histograms.first;
  auto& l_nle = histograms.second;

  DASH_ASSERT_EQ(
      histograms.first.size(), histograms.second.size(),
      "length of histogram arrays does not match");

  DASH_SORT_LOG_TRACE_RANGE(
      "final partition borders", partitions.begin(), partitions.end());
  DASH_SORT_LOG_TRACE_RANGE(
      "final histograms: l_nlt", l_nlt.begin(), l_nlt.end());
  DASH_SORT_LOG_TRACE_RANGE(
      "final histograms: l_nle", l_nle.begin(), l_nle.end());

  if (n_l_elem > 0) {
    // TODO; minimize communication to copy only until the last valid border
    /*
     * Transpose (Shuffle) the final histograms to communicate
     * the partition distribution
     */
    for (std::size_t idx = 1; idx < l_nlt.size(); ++idx) {
      auto const transposed_unit = idx - 1;

      if (transposed_unit != myid) {
        auto const offset = transposed_unit * g_partition_data.lsize() + myid;
        // We communicate only non-zero values
        if (l_nlt[idx] > 0)
          g_partition_data.async[offset + OFF_DIST(nunits)].set(
              &(l_nlt[idx]));

        if (l_nle[idx] > 0)
          g_partition_data.async[offset + OFF_SUPP(nunits)].set(
              &(l_nle[idx]));
      }
      else {
        g_partition_data.local[myid + OFF_DIST(nunits)] = l_nlt[idx];
        g_partition_data.local[myid + OFF_SUPP(nunits)] = l_nle[idx];
      }
    }
    // complete outstanding requests...
    g_partition_data.async.flush();
  }

  team.barrier();

  DASH_SORT_LOG_TRACE_RANGE(
      "initial partition distribution:",
      g_partition_data.lbegin() + OFF_DIST(nunits),
      g_partition_data.lbegin() + OFF_DIST(nunits) + nunits);

  DASH_SORT_LOG_TRACE_RANGE(
      "partition supply", g_partition_data.lbegin() + OFF_SUPP(nunits),
      g_partition_data.lbegin() + OFF_SUPP(nunits) + nunits);

  /* Calculate final distribution per partition. Each unit calculates their
   * local distribution independently.
   * All accesses are only to local memory
   */
  detail::psort__calc_final_partition_dist(
      l_acc_unit_count, g_partition_data.local);

  DASH_SORT_LOG_TRACE_RANGE(
      "final partition distribution",
      g_partition_data.lbegin() + OFF_DIST(nunits),
      g_partition_data.lbegin() + OFF_DIST(nunits) + nunits);

  team.barrier();

  /*
   * Transpose the final distribution again to obtain the end offsets
   */

  dash::team_unit_t unit{0};
  auto const        last = static_cast<dash::team_unit_t>(nunits);

  for (; unit < last; ++unit) {
    if (g_partition_data.local[OFF_DIST(nunits) + unit] == 0) continue;

    if (unit != myid) {
      // We communicate only non-zero values
      auto const offset = unit * g_partition_data.lsize() + myid;
      g_partition_data.async[offset + OFF_SUPP(nunits)].set(
          &(g_partition_data.local[OFF_DIST(nunits) + unit]));
    }
    else {
      g_partition_data.local[OFF_SUPP(nunits) + myid] =
          g_partition_data.local[OFF_DIST(nunits) + unit];
    }
  }

  g_partition_data.async.flush();

  team.barrier();

  DASH_SORT_LOG_TRACE_RANGE(
      "final target count", g_partition_data.lbegin() + OFF_SUPP(nunits),
      g_partition_data.lbegin() + OFF_SUPP(nunits) + nunits);

  std::vector<size_type> l_send_displs(nunits, 0);

  if (n_l_elem > 0) {
    // auto const& bounding_units   = p_borders.bounding_units;
    detail::psort__calc_send_count(
        p_borders, valid_partitions, g_partition_data.local, l_send_displs);
  }
  else {
    std::fill(
        g_partition_data.lbegin() + OFF_DIST(nunits),
        g_partition_data.lbegin() + OFF_DIST(nunits) + nunits, 0);
  }

  DASH_SORT_LOG_TRACE_RANGE(
      "send count", g_partition_data.lbegin() + OFF_DIST(nunits),
      g_partition_data.lbegin() + OFF_DIST(nunits) + nunits);

  DASH_SORT_LOG_TRACE_RANGE(
      "send displs", l_send_displs.begin(), l_send_displs.end());

  team.barrier();

  detail::psort__calc_target_displs(g_partition_data);

  team.barrier();

  DASH_SORT_LOG_TRACE_RANGE(
      "target displs", &(g_partition_data.local[OFF_DISP(nunits)]),
      &(g_partition_data.local[OFF_DISP(nunits)]) + nunits);

  std::vector<dash::Future<iter_type> > async_copies{};
  async_copies.reserve(p_unit_info.contributing_remote_units.size());

  DASH_SORT_LOG_TRACE_RANGE("before final sort round", lbegin, lend);

  auto const l_partition_data = g_partition_data.local;

  auto const get_send_info = [l_partition_data, &l_send_displs,
                              nunits](dash::team_unit_t const unit) {
    auto const send_count  = l_partition_data[unit + OFF_DIST(nunits)];
    auto const target_disp = l_partition_data[unit + OFF_DISP(nunits)];
    auto const send_disp   = l_send_displs[unit];
    return std::make_tuple(send_count, send_disp, target_disp);
  };

  size_type send_count, send_disp, target_disp;

  for (auto const& unit : p_unit_info.contributing_remote_units) {
    std::tie(send_count, send_disp, target_disp) = get_send_info(unit);

    iter_type it_copy{};
    if (unit == unit_at_begin) {
      it_copy = begin;
    }
    else {
      // The array passed to global_index is 0 initialized
      auto gidx = pattern.global_index(unit, {});
      it_copy   = iter_type{&(begin.globmem()), pattern, gidx};
    }

    auto fut = dash::copy_async(
        &(*(lcopy.begin() + send_disp)),
        &(*(lcopy.begin() + send_disp + send_count)), it_copy + target_disp);

    async_copies.push_back(fut);
  }

  std::tie(send_count, send_disp, target_disp) = get_send_info(myid);

  if (send_count) {
    std::copy(
        lcopy.begin() + send_disp, lcopy.begin() + send_disp + send_count,
        lbegin + target_disp);
  }

  for (auto& fut : async_copies) {
    fut.wait();
  }

  team.barrier();

  std::sort(lbegin, lend);
  DASH_SORT_LOG_TRACE_RANGE("finally sorted range", lbegin, lend);

  team.barrier();
}

#endif  // DOXYGEN

}  // namespace detail

#endif
