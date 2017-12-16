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
#define DASH_SORT_LOG_TRACE_RANGE(desc, begin, end)
#endif

namespace detail {

template <typename T>
struct PartitionBorder {
public:
  std::vector<bool> is_last_iter;
  std::vector<bool> is_stable;
  std::vector<T>    lower_bound;
  std::vector<T>    upper_bound;

  PartitionBorder(size_t nsplitter, T _lower_bound, T _upper_bound)
  {
    std::fill_n(std::back_inserter(is_last_iter), nsplitter, false);
    std::fill_n(std::back_inserter(is_stable), nsplitter, false);
    std::fill_n(std::back_inserter(lower_bound), nsplitter, _lower_bound);
    std::fill_n(std::back_inserter(upper_bound), nsplitter, _upper_bound);
  }
};

template <typename T>
inline void psort__calc_boundaries(
    PartitionBorder<T>& p_borders, std::vector<T>& partitions)
{
  DASH_ASSERT_EQ(
      p_borders.is_stable.size(), partitions.size(),
      "invalid number of partition borders");
  // recalculate partition boundaries
  for (std::size_t idx = 0; idx < partitions.size(); ++idx) {
    // case A: partition is already stable -> done
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
}

template <typename ElementType, typename IndexType>
inline std::pair<std::vector<IndexType>, std::vector<IndexType> >
psort__local_histogram(
    std::vector<ElementType> const& partitions,
    ElementType const*              lbegin,
    ElementType const*              lend)
{
  auto const nborders = partitions.size();
  auto const sz       = partitions.size() + 2;
  // Number of elements less than P
  std::vector<IndexType> n_lt(sz, 0);
  // Number of elements less than equals P
  std::vector<IndexType> n_le(sz, 0);

  auto const n_l_elem = std::distance(lbegin, lend);

  if (n_l_elem == 0) return std::make_pair(n_lt, n_le);

 std::size_t idx;

  for (idx = 0; idx < nborders; ++idx) {
    auto lb_it = std::lower_bound(lbegin, lend, partitions[idx]);
    auto ub_it = std::upper_bound(lbegin, lend, partitions[idx]);

    n_lt[idx + 1] = std::distance(lbegin, lb_it);
    n_le[idx + 1] = std::distance(lbegin, ub_it);
  }

  n_lt[idx + 1] = n_l_elem;
  n_le[idx + 1] = n_l_elem;

  return std::make_pair(n_lt, n_le);
}

template <typename IndexType, typename ArrayType>
inline void psort__global_histogram(
    std::vector<IndexType> const& l_nlt,
    std::vector<IndexType> const& l_nle,
    ArrayType&                    g_nlt_nle)
{
  DASH_ASSERT_EQ(l_nlt.size(), l_nle.size(), "Sizes must match");
  DASH_ASSERT_EQ(
      g_nlt_nle.size(), (l_nlt.size() - 1) * 2, "Sizes must match");

  std::fill(g_nlt_nle.lbegin(), g_nlt_nle.lend(), 0);

  g_nlt_nle.team().barrier();

  // TODO: implement dash::transform_async
  for (std::size_t idx = 1; idx < l_nlt.size(); ++idx) {
    std::array<IndexType, 2> vals{{l_nlt[idx], l_nle[idx]}};
    auto const g_idx_nlt = (idx - 1) * 2;

    dash::transform<IndexType>(
        &(*std::begin(vals)),  // A
        &(*std::end(vals)),
        g_nlt_nle.begin() + g_idx_nlt,  // B
        g_nlt_nle.begin() + g_idx_nlt,  // B = op(B,A)
        dash::plus<IndexType>());       // op
  }

  g_nlt_nle.team().barrier();
}

template <typename ElementType, typename IndexType, typename ArrayType>
inline bool psort_validate_partitions(
    PartitionBorder<ElementType>&   p_borders,
    std::vector<ElementType> const& partitions,
    std::vector<IndexType> const&   acc_unit_count,
    ArrayType const&                g_nlt_nle)
{
  using array_value_t =
      typename std::decay<decltype(g_nlt_nle)>::type::value_type;

  static_assert(
      std::is_same<IndexType, array_value_t>::value,
      "local and global array value types must be equal");

  std::vector<IndexType> l_nlt_nle(g_nlt_nle.size() + 2);

  // first two values are 0
  std::fill(l_nlt_nle.begin(), l_nlt_nle.begin() + 2, 0);

  // TODO: this step may be more efficient with block wise copy
  // to overlap communication and computation
  dash::copy(g_nlt_nle.begin(), g_nlt_nle.end(), l_nlt_nle.data() + 2);

  for (std::size_t idx = 0; idx < partitions.size(); ++idx) {
    auto const nlt_idx = (idx + 1) * 2;
    if (l_nlt_nle[nlt_idx] < acc_unit_count[idx + 1] &&
        acc_unit_count[idx + 1] <= l_nlt_nle[nlt_idx + 1]) {
      p_borders.is_stable[idx] = true;
    }
    else {
      if (l_nlt_nle[nlt_idx] >= acc_unit_count[idx + 1]) {
        p_borders.upper_bound[idx] = partitions[idx];
      }
      else {
        p_borders.lower_bound[idx] = partitions[idx];
      }
    }
  }

  // Exit condition: is there any non-stable partition
  auto const nonstable_it = std::find(
      p_borders.is_stable.cbegin(), p_borders.is_stable.cend(), false);

  // exit condition
  return nonstable_it == p_borders.is_stable.cend();
}

template <typename IndexType>
void calc_final_partition_dist(
    dash::Array<IndexType> const& g_partition_supply,
    std::vector<IndexType> const& acc_unit_count,
    dash::Array<IndexType>&       g_partition_dist)
{
  /* Calculate number of elements to receive for each partition:
   * We first assume that we we receive exactly the number of elements which
   * are less than P.
   * The output are the end offsets for each partition
   */

  auto const myid = g_partition_supply.team().myid();
  auto const n_my_elements =
      std::accumulate(g_partition_dist.lbegin(), g_partition_dist.lend(), 0);

  // Calculate the deficit
  auto my_deficit = acc_unit_count[myid + 1] - n_my_elements;

  dash::team_unit_t       unit(0);
  dash::team_unit_t const last(g_partition_supply.team().size());

  // If there is a deficit, look how much unit j can supply
  for (; unit < last && my_deficit > 0; ++unit) {
    auto const supply_unit =
        g_partition_supply.local[unit] - g_partition_dist.local[unit];

    DASH_ASSERT_GE(supply_unit, 0, "invalid supply of target unit");
    if (supply_unit <= my_deficit) {
      g_partition_dist.local[unit] += supply_unit;
      my_deficit -= supply_unit;
    }
    else {
      g_partition_dist.local[unit] += my_deficit;
      my_deficit = 0;
    }
  }

  DASH_ASSERT_GE(my_deficit, 0, "Invalid local deficit");
}

template <typename IndexType>
std::vector<IndexType> psort__calc_send_count(
    dash::Array<IndexType> const& g_target_count,
    dash::Array<IndexType>&       g_send_count)
{
  auto const             nunits = g_target_count.team().size();
  std::vector<IndexType> l_target_count(nunits + 1);

  DASH_ASSERT_EQ(
      g_target_count.lsize(), l_target_count.size() - 1,
      "Array sizes do not match");

  l_target_count[0] = 0;

  std::copy(
      g_target_count.lbegin(), g_target_count.lend(),
      l_target_count.begin() + 1);

  std::transform(
      l_target_count.begin() + 1, l_target_count.end(),
      l_target_count.begin(), g_send_count.lbegin(), std::minus<IndexType>());

  std::vector<IndexType> l_send_displs(nunits);
  l_send_displs[0] = 0;

  DASH_ASSERT_EQ(g_send_count.lsize(), nunits, "Array sizes must match");

  std::transform(
      g_send_count.lbegin(), g_send_count.lend() - 1, l_send_displs.begin(),
      l_send_displs.begin() + 1, std::plus<IndexType>());

  return l_send_displs;
}

template <typename IndexType>
void psort__calc_target_displs(
    dash::Array<IndexType> const& g_send_count,
    dash::Array<IndexType>&       g_target_displs)
{
  auto const nunits = g_target_displs.team().size();
  auto const myid   = g_target_displs.team().myid();

  if (0 == myid) {
    // Unit 0 always writes to target offset 0
    std::fill(g_target_displs.lbegin(), g_target_displs.lend(), 0);
  }

  std::vector<IndexType> l_target_displs(nunits, 0);

  team_unit_t       unit(1);
  team_unit_t const last(nunits);

  auto const target_displs_lbegin = myid * nunits;
  auto const target_displs_lend   = target_displs_lbegin + nunits;

  for (; unit < last; ++unit) {
    auto const      prev_u = unit - 1;
    IndexType const val    = (prev_u == myid)
                              ? g_send_count.local[prev_u]
                              : g_send_count[prev_u * nunits + myid];

    l_target_displs[unit]    = val + l_target_displs[prev_u];
    auto const target_offset = unit * nunits + myid;
    if (target_displs_lbegin <= target_offset &&
        target_offset < target_displs_lend) {
      g_target_displs.local[target_offset % nunits] = l_target_displs[unit];
    }
    else {
      g_target_displs.async[target_offset].set(&(l_target_displs[unit]));
    }
  }

  g_target_displs.async.flush();
}

template <typename GlobIterT, typename PatternT>
auto psort__calc_unit_counts(
    PatternT const& pattern, GlobIterT const begin, GlobIterT const end)
    -> std::vector<typename std::decay<decltype(pattern)>::type::index_type>
{
  using index_type = typename std::decay<decltype(pattern)>::type::index_type;

  auto const nunits = pattern.team().size();

  dash::team_unit_t       unit{0};
  const dash::team_unit_t last{static_cast<dart_unit_t>(nunits)};

  auto const unit_first = pattern.unit_at(begin.pos());
  auto const unit_last  = pattern.unit_at(end.pos() - 1);

  // Starting offsets of all units
  std::vector<index_type> acc_unit_count(nunits + 1);
  acc_unit_count[0] = 0;

  for (; unit < last; ++unit) {
    // first linear global index of unit
    auto const u_gidx_begin = pattern.global_index(unit, {});
    // Number of elements located at current source unit:
    auto const u_size = pattern.local_size(unit);
    // last global index of unit
    auto const u_gidx_end = u_gidx_begin + u_size;

    if (u_gidx_end - 1 < begin.pos() || u_gidx_begin >= end.pos()) {
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
        auto const local_pos =
            pattern.local(static_cast<index_type>(u_gidx_begin));

        n_u_elements = u_size - local_pos.index;

        DASH_ASSERT_EQ(local_pos.unit, unit, "units must match");
      }

      acc_unit_count[unit + 1] = n_u_elements + acc_unit_count[unit];
    }
  }
  return acc_unit_count;
}

}  // namespace detail

template <class GlobRandomIt>
void sort(GlobRandomIt begin, GlobRandomIt end)
{
  auto pattern             = begin.pattern();
  using iter_t             = decltype(begin);
  using iter_pattern_t     = decltype(pattern);
  using index_type         = typename iter_pattern_t::index_type;
  using size_type          = typename iter_pattern_t::size_type;
  using value_type         = typename iter_t::value_type;
  using const_pointer_type = typename iter_t::const_pointer;

  static_assert(
      std::is_arithmetic<value_type>::value,
      "Only integral types are supported");

  if (pattern.team() == dash::Team::Null()) {
    DASH_LOG_DEBUG("dash::sort", "Sorting on dash::Team::Null()");
    return;
  }
  else if (pattern.team().size() == 1) {
    DASH_LOG_DEBUG("dash::sort", "Sorting on a team with only 1 unit");
    std::sort(begin.local(), end.local());
    return;
  }

  // TODO: instead of std::sort we may accept a user-defined local sort
  // function

  if (begin >= end) {
    DASH_LOG_DEBUG("dash::sort", "empty range");
    pattern.team().barrier();
    return;
  }

  dash::Team& team   = pattern.team();
  auto const  nunits = team.size();
  auto const  myid   = team.myid();

  auto const unit_at_begin = pattern.unit_at(begin.pos());

  // local distance
  auto const l_range = dash::local_index_range(begin, end);

  if (l_range.begin == l_range.end) {
    // TODO: empty local size
  }

  auto l_mem_begin = begin.globmem().lbegin();

  if (l_mem_begin == nullptr) {
    // TODO: handle local nullptr
  }

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
  // constructors

  using array_t = dash::Array<index_type>;

  array_t g_nlt_nle(nunits * 2, dash::BLOCKED, team);

  // Buffer for partition loops
  array_t g_nlt_nle_buf(g_nlt_nle.pattern());

  array_t g_partition_dist(nunits * nunits, dash::BLOCKED, team);
  array_t g_partition_supply(nunits * nunits, dash::BLOCKED, team);

  auto const min = static_cast<value_type>(g_min.get());
  auto const max = static_cast<value_type>(g_max.get());

  auto const acc_unit_count = detail::psort__calc_unit_counts(pattern, begin, end);

  auto const                          nboundaries = nunits - 1;
  std::vector<value_type>             partitions(nboundaries, 0);
  detail::PartitionBorder<value_type> p_borders(nboundaries, min, max);

  DASH_SORT_LOG_TRACE_RANGE("locally sorted array", lbegin, lend);

  bool done = false;


  do {
    detail::psort__calc_boundaries(p_borders, partitions);

    auto const histograms =
        detail::psort__local_histogram<value_type, index_type>(
            partitions, lbegin, lend);

    auto const& l_nlt = histograms.first;
    auto const& l_nle = histograms.second;

    detail::psort__global_histogram(l_nlt, l_nle, g_nlt_nle);

    done = detail::psort_validate_partitions(
        p_borders, partitions, acc_unit_count, g_nlt_nle);

    // This swap eliminates a barrier as, otherwise, some units may be one
    // iteration ahead and modify shared data while the others are still not
    // done
    if (!done) {
      std::swap(g_nlt_nle, g_nlt_nle_buf);
    }

  } while (!done);

  auto const histograms =
      detail::psort__local_histogram<value_type, index_type>(
          partitions, lbegin, lend);

  /* How many elements are less than P
   * or less than equals P */
  auto const& l_nlt = histograms.first;
  auto const& l_nle = histograms.second;

  DASH_ASSERT_EQ(
      histograms.first.size(), histograms.second.size(),
      "length of histogram arrays does not match");

  DASH_SORT_LOG_TRACE_RANGE(
      "final partition borders", partitions.begin(), partitions.end());
  DASH_SORT_LOG_TRACE_RANGE(
      "final histograms: l_nlt", l_nlt.begin(), l_nlt.end());
  DASH_SORT_LOG_TRACE_RANGE(
      "final histograms: l_nle", l_nle.begin(), l_nle.end());

  /*
   * Transpose (Shuffle) the final histograms to communicate
   * the partition distribution
   */

  for (std::size_t idx = 1; idx < l_nlt.size(); ++idx) {
    auto const transposed_unit = idx - 1;
    DASH_ASSERT_EQ(
        g_partition_dist.pattern().local_size(
            static_cast<dash::team_unit_t>(transposed_unit)),
        l_nlt.size() - 1, "Invalid Array length");
    auto const offset = transposed_unit * nunits + myid.id;

    g_partition_dist.async[offset].set(&(l_nlt[idx]));
    g_partition_supply.async[offset].set(&(l_nle[idx]));
  }

  // complete outstanding requests...
  g_partition_dist.async.flush();
  g_partition_supply.async.flush();

  team.barrier();

  DASH_SORT_LOG_TRACE_RANGE(
      "initial partition distribution: g_partition_dist.local",
      g_partition_dist.lbegin(), g_partition_dist.lend());

  DASH_SORT_LOG_TRACE_RANGE(
      "partition supply: g_partition_supply.local",
      g_partition_supply.lbegin(), g_partition_supply.lend());

  /* Calculate final distribution per partition. Each unit calculate their
   * local distribution independently.
   * All accesses are only to local memory
   */
  detail::calc_final_partition_dist(
      g_partition_supply, acc_unit_count, g_partition_dist);

  DASH_SORT_LOG_TRACE_RANGE(
      "final partition distribution", g_partition_dist.lbegin(),
      g_partition_dist.lend());

  team.barrier();

  /*
   * Transpose the final distribution again to obtain the end offsets
   */
  auto&             g_target_count = g_partition_supply;
  dash::team_unit_t unit{0};
  auto const        last = static_cast<dash::team_unit_t>(nunits);

  for (; unit < last; ++unit) {
    DASH_ASSERT_EQ(
        g_target_count.pattern().local_size(unit), g_partition_dist.lsize(),
        "Invalid Array length");
    auto const offset = unit * nunits + myid;
    g_target_count.async[offset].set(&(g_partition_dist.local[unit]));
  }

  g_target_count.async.flush();

  team.barrier();

  DASH_SORT_LOG_TRACE_RANGE(
      "final target count", g_target_count.lbegin(), g_target_count.lend());

  /* g_send_count only local access */
  auto& g_send_count  = g_partition_dist;
  auto  l_send_displs = detail::psort__calc_send_count(g_target_count, g_send_count);

  DASH_SORT_LOG_TRACE_RANGE(
      "send count", g_send_count.lbegin(), g_send_count.lend());

  DASH_SORT_LOG_TRACE_RANGE(
      "send displs", l_send_displs.begin(), l_send_displs.end());

  // Implicit barrier in Array Constructor
  // team.barrier();
  array_t g_target_displs(nunits * nunits, dash::BLOCKED, team);

  detail::psort__calc_target_displs(g_send_count, g_target_displs);

  team.barrier();

  DASH_SORT_LOG_TRACE_RANGE(
      "target displs", g_target_displs.lbegin(), g_target_displs.lend());

  unit = static_cast<team_unit_t>(0);

  std::vector<dash::Future<iter_t> > async_copies{};

  DASH_SORT_LOG_TRACE_RANGE("before final sort round", lbegin, lend);

  for (; unit < last; ++unit) {
    auto const send_count = g_send_count.local[unit];
    DASH_ASSERT_GE(send_count, 0, "invalid send count");
    if (send_count == 0) continue;
    auto target_disp = g_target_displs.local[unit];
    DASH_ASSERT_GE(target_disp, 0, "invalid target disp");
    auto const send_disp = l_send_displs[unit];
    DASH_ASSERT_GE(send_disp, 0, "invalid send disp");

    if (unit != myid) {
      iter_t it_copy{};
      if (unit == unit_at_begin) {
        it_copy = begin;
      }
      else {
        // The array passed to global_index is 0 initialized
        index_type gidx = pattern.global_index(unit, {});
        it_copy         = iter_t{&(begin.globmem()), pattern, gidx};
      }

      auto fut = dash::copy_async(
          &(*(lcopy.begin() + send_disp)),
          &(*(lcopy.begin() + send_disp + send_count)),
          it_copy + target_disp);

      async_copies.push_back(fut);
    }
    else {
      std::copy(
          lcopy.begin() + send_disp, lcopy.begin() + send_disp + send_count,
          lbegin + target_disp);
    }
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

}  // namespace dash

#endif
