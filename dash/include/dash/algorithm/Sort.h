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

namespace dash {

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
inline void psort_calculate_boundaries(
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

template <typename ElementType, typename DifferenceType>
inline std::pair<std::vector<DifferenceType>, std::vector<DifferenceType>>
psort_local_histogram(
    std::vector<ElementType> const& partitions,
    ElementType const*              lbegin,
    ElementType const*              lend)
{
  auto const nborders = partitions.size();
  auto const sz       = partitions.size() + 2;
  // Number of elements less than P
  std::vector<DifferenceType> n_lt(sz, 0);
  // Number of elements less than equals P
  std::vector<DifferenceType> n_le(sz, 0);

  auto const n_l_elem = std::distance(lbegin, lend);

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

template <typename DifferenceType, typename ArrayType>
inline void psort_global_histogram(
    std::vector<DifferenceType> const& l_nlt,
    std::vector<DifferenceType> const& l_nle,
    ArrayType&                         g_nlt,
    ArrayType&                         g_nle)
{
  auto const sz_unit0 = g_nlt.pattern().local_size(dash::team_unit_t{0});

  DASH_ASSERT_EQ(l_nlt.size(), l_nle.size(), "Sizes must match");
  DASH_ASSERT_EQ(g_nlt.size(), l_nlt.size(), "Sizes must match");
  DASH_ASSERT_EQ(g_nle.size(), l_nle.size(), "Sizes must match");

#if defined(DASH_ENABLE_ASSERTIONS)
  DASH_ASSERT_EQ(sz_unit0, 2, "invalid number of local elements at unit 0");
  auto const myid = g_nlt.pattern().team().myid();
  if (myid.id != 0) {
    DASH_ASSERT_EQ(
        g_nlt.pattern().local_size(myid), 1,
        "invalid number of local elements");
    DASH_ASSERT_EQ(g_nlt.lsize(), 1, "invalid number of local elements");
  }
#endif

  /* reduce among all units */
  std::fill(g_nlt.lbegin(), g_nlt.lend(), 0);
  std::fill(g_nle.lbegin(), g_nle.lend(), 0);

  g_nlt.team().barrier();

  using glob_atomic_ref_t = dash::GlobRef<dash::Atomic<DifferenceType>>;

  // TODO: Implement GlobAsyncRef<Atomic>, so we can asynchronously accumulate
  for (std::size_t idx = 0; idx < l_nlt.size(); ++idx) {
    // accumulate g_nlt
    auto ref_clt        = g_nlt[idx];
    auto atomic_ref_clt = glob_atomic_ref_t(ref_clt.dart_gptr());
    atomic_ref_clt.add(l_nlt[idx]);

    // accumulate g_nle
    auto ref_cle        = g_nle[idx];
    auto atomic_ref_cle = glob_atomic_ref_t(ref_cle.dart_gptr());
    atomic_ref_cle.add(l_nle[idx]);
  }

  g_nlt.team().barrier();
}

template <typename ElementType, typename DifferenceType, typename ArrayType>
inline bool psort_validate_partitions(
    PartitionBorder<ElementType>&      p_borders,
    std::vector<ElementType> const&    partitions,
    std::vector<DifferenceType> const& C,
    ArrayType const&                   g_nlt,
    ArrayType const&                   g_nle)
{
  using array_value_t =
      typename std::decay<decltype(g_nlt)>::type::value_type;

  static_assert(
      std::is_same<DifferenceType, array_value_t>::value,
      "local and global array value types must be equal");

  std::vector<DifferenceType> l_nlt(g_nlt.size(), 0);
  std::vector<DifferenceType> l_nle(g_nle.size(), 0);

  // TODO: this step may be more efficient with block wise copy
  // to overlap communication and computation
  dash::copy(g_nle.begin(), g_nle.end(), l_nle.data());
  dash::copy(g_nlt.begin(), g_nlt.end(), l_nlt.data());

  for (std::size_t idx = 0; idx < partitions.size(); ++idx) {
    if (l_nlt[idx + 1] < C[idx + 1] && C[idx + 1] <= l_nle[idx + 1]) {
      p_borders.is_stable[idx] = true;
    }
    else {
      if (l_nlt[idx + 1] >= C[idx + 1]) {
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
}  // namespace detail

template <typename GlobRandomIt>
void sort(GlobRandomIt begin, GlobRandomIt end)
{
  // TODO: this approach supports only blocked patterns with contiguous memory
  // blocks

  auto pattern             = begin.pattern();
  using iter_t             = decltype(begin);
  using iter_pattern_t     = decltype(pattern);
  using index_type         = typename iter_pattern_t::index_type;
  using size_type          = typename iter_pattern_t::size_type;
  using value_type         = typename iter_t::value_type;
  using difference_type    = index_type;
  using const_pointer_type = typename iter_t::const_pointer;

  static_assert(
      std::is_unsigned<value_type>::value,
      "only unsigned types are supported");

  if (pattern.team() == dash::Team::Null()) {
    DASH_LOG_DEBUG("dash::sort", "Sorting on dash::Team::Null()");
    return;
  }
  else if (pattern.team().size() == 1) {
    DASH_LOG_DEBUG("dash::sort", "Sorting on a team with only 1 unit");
    std::sort(begin.local(), end.local());
    return;
  }

  // gl
  auto       begin_ptr = static_cast<const_pointer_type>(begin);
  auto       end_ptr   = static_cast<const_pointer_type>(end);
  auto const n_g_elem  = dash::distance(begin_ptr, end_ptr);
  if (n_g_elem == 0) {
    DASH_LOG_DEBUG("dash::sort", "empty range");
    pattern.team().barrier();
    return;
  }

  dash::Team& team   = pattern.team();
  auto const  nunits = team.size();
  auto const  myid   = team.myid();

  /* number of elements less than (or equal) to splitter border accumulated */
  using pattern_t = dash::CSRPattern<1>;

  // Unit 0 is the only unit with two elements
  std::vector<size_type> lsizes(nunits);
  lsizes[0] = 2;
  std::fill(lsizes.begin() + 1, lsizes.end(), 1);
  pattern_t pat(lsizes, team);

  dash::Array<difference_type, index_type, pattern_t> g_nlt(pat);
  dash::Array<difference_type, index_type, pattern_t> g_nle(pat);

  dash::Array<difference_type> g_nlt_all(nunits * nunits, team);
  dash::Array<difference_type> g_nle_all(nunits * nunits, team);

  // std::vector<difference_type> myT_C(nunits);
  std::vector<difference_type> myC(nunits + 1);

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

  // Starting offsets of all units
  std::vector<difference_type> C(nunits + 1);
  // initial counts
  dash::team_unit_t       unit{0};
  const dash::team_unit_t last{static_cast<dart_unit_t>(nunits)};

  C[0] = 0;

  for (; unit < last; ++unit) {
    auto const u_size = pattern.local_size(unit);
    C[unit + 1]       = u_size + C[unit];
  };

  // TODO: provide dash::min_max
  auto const partitions_min_it = dash::min_element(begin, end);
  auto const partitions_max_it = dash::max_element(begin, end);

  auto const min = static_cast<value_type>(*partitions_min_it);
  auto const max = static_cast<value_type>(*partitions_max_it);

  auto const                          nboundaries = nunits - 1;
  std::vector<value_type>             partitions(nboundaries, 0);
  detail::PartitionBorder<value_type> p_borders(nboundaries, min, max);

  DASH_SORT_LOG_TRACE_RANGE("locally sorted array", lbegin, lend);

  size_t iter = 0;
  bool   done = false;

  do {
    iter++;

    detail::psort_calculate_boundaries(p_borders, partitions);

    auto const histograms =
        detail::psort_local_histogram<value_type, difference_type>(
            partitions, lbegin, lend);

    auto const& l_nlt = histograms.first;
    auto const& l_nle = histograms.second;

    detail::psort_global_histogram(l_nlt, l_nle, g_nlt, g_nle);

    DASH_ASSERT(g_nle.size() == l_nle.size());
    DASH_ASSERT(g_nlt.size() == l_nlt.size());

    done = detail::psort_validate_partitions(
        p_borders, partitions, C, g_nlt, g_nle);

    // ensure that all units are synchronized at the end of an iteration
    team.barrier();

  } while (!done);

  auto const histograms =
      detail::psort_local_histogram<value_type, difference_type>(
          partitions, lbegin, lend);

  auto const& nlt = histograms.first;
  auto const& nle = histograms.second;

  DASH_ASSERT_EQ(
      histograms.first.size(), histograms.second.size(),
      "length of histogram arrays does not match");

  DASH_SORT_LOG_TRACE_RANGE(
      "final partition borders", partitions.begin(), partitions.end());
  DASH_SORT_LOG_TRACE_RANGE("final histograms: nlt", nlt.begin(), nlt.end());
  DASH_SORT_LOG_TRACE_RANGE("final histograms: nle", nle.begin(), nle.end());

  // transpose final histograms
  for (std::size_t idx = 1; idx < nlt.size(); ++idx) {
    auto const unit_id = static_cast<dash::team_unit_t>(idx - 1);
    DASH_ASSERT_EQ(
        g_nlt_all.pattern().local_size(unit_id), nlt.size() - 1,
        "Invalid Array length");
    auto const offset       = unit_id * nunits + myid;
    g_nlt_all.async[offset] = nlt[idx];
    g_nle_all.async[offset] = nle[idx];
  }

  // complete outstanding requests...
  g_nlt_all.async.flush();
  g_nle_all.async.flush();

  team.barrier();

  DASH_SORT_LOG_TRACE_RANGE(
      "transposed histograms: g_nlt_all.local", g_nlt_all.lbegin(),
      g_nlt_all.lend());
  DASH_SORT_LOG_TRACE_RANGE(
      "transposed histograms: g_nle_all.local", g_nle_all.lbegin(),
      g_nle_all.lend());

  // Let us now collect the items which we have to send
  auto const n_my_elements =
      std::accumulate(g_nlt_all.lbegin(), g_nlt_all.lend(), 0);

  auto my_deficit = C[myid + 1] - n_my_elements;

  unit = static_cast<team_unit_t>(0);

  for (; unit < last && my_deficit > 0; ++unit) {
    auto const supply_unit = g_nle_all.local[unit] - g_nlt_all.local[unit];

    DASH_ASSERT_GE(supply_unit, 0, "ivalid supply of target unit");
    if (supply_unit <= my_deficit) {
      g_nlt_all.local[unit] += supply_unit;
      my_deficit -= supply_unit;
    }
    else {
      g_nlt_all.local[unit] += my_deficit;
      my_deficit = 0;
    }
  }

  DASH_ASSERT_GE(my_deficit, 0, "Invalid local deficit");

  // again transpose everything in g_nle_all
  //
  unit = static_cast<team_unit_t>(0);

  for (; unit < last; ++unit) {
    DASH_ASSERT_EQ(
        g_nle_all.pattern().local_size(unit), g_nlt_all.lsize(),
        "Invalid Array length");
    auto const offset       = unit * nunits + myid;
    g_nle_all.async[offset] = g_nlt_all.local[unit];
  }

  g_nle_all.async.flush();
  team.barrier();

  DASH_SORT_LOG_TRACE_RANGE(
      "final target count", g_nle_all.lbegin(), g_nle_all.lend());

  DASH_ASSERT_EQ(
      g_nle_all.lsize(), myC.size() - 1, "Array sizes do not match");

  myC[0] = 0;
  std::copy(g_nle_all.lbegin(), g_nle_all.lend(), myC.begin() + 1);

  // g_nlt_all.local[unit] = myC[unit + 1] - myC[unit];
  std::transform(
      myC.begin() + 1, myC.end(), myC.begin(), g_nlt_all.lbegin(),
      std::minus<difference_type>());

  DASH_SORT_LOG_TRACE_RANGE(
      "send count", g_nlt_all.lbegin(), g_nlt_all.lend());

  std::vector<difference_type> send_offsets(nunits);
  send_offsets[0] = 0;

  DASH_ASSERT_EQ(g_nlt_all.lsize(), nunits, "Array sizes must match");

  std::transform(
      g_nlt_all.lbegin(), g_nlt_all.lend() - 1, send_offsets.begin(),
      send_offsets.begin() + 1, std::plus<difference_type>());

  DASH_SORT_LOG_TRACE_RANGE(
      "send displs", send_offsets.begin(), send_offsets.end());

  //TODO: Maybe there is a better way to obtain the correct type??
  using local_coords_t = typename iter_pattern_t::local_coords_t;
  local_coords_t xx{};
  using coords_array_t = decltype(xx.coords);


  coords_array_t const lcoords{};

  unit = static_cast<team_unit_t>(0);

  std::vector<dash::Future<iter_t>> async_copies{};

  std::vector<value_type> buf;
  buf.reserve(n_l_elem);

  std::copy(lbegin, lend, buf.begin());

  team.barrier();

#if 0
  for (; unit < last; ++unit) {
    if (unit != myid) {
      auto const gidx = pattern.global_index(unit, lcoords);
      auto const send_count = g_nlt_all.local[unit];
      std::size_t target_disp;
      if (unit > myid) {
        target_disp = std::accumulate(
            g_nlt_all.lbegin(), g_nlt_all.lbegin() + unit - 1, 0);
      }
      else {
        target_disp =
            std::accumulate(g_nlt_all.lbegin(), g_nlt_all.lbegin() + unit, 0);
      }
      DASH_ASSERT_RANGE(0, gidx, n_g_elem, "invalid global index");
      DASH_ASSERT_GE(send_offsets[unit], 0, "send offset has to be >= 0");
      auto fut = dash::copy_async(
          lbegin + send_offsets[unit], lbegin + send_count,
          begin + gidx + target_disp);
      async_copies.push_back(fut);
    }
  }

  for (auto & fut : async_copies) {
    fut.wait();
  }
#endif
}

}  // namespace dash

#endif
