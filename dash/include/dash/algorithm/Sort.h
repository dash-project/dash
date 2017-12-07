#ifndef DASH__ALGORITHM__SORT_H__
#define DASH__ALGORITHM__SORT_H__

#include <algorithm>
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
inline void calculate_boundaries(
    PartitionBorder<T>& p_borders, std::vector<T>& partitions)
{
  DASH_ASSERT_EQ(
      p_borders.is_stable.size(), partitions.size() - 1,
      "invalid number of partition borders");
  // recalculate partition boundaries
  for (std::size_t idx = 0; idx < partitions.size() - 1; ++idx) {
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
local_histogram(
    std::vector<ElementType> const& partitions,
    ElementType const*              lbegin,
    ElementType const*              lend)
{
  auto const nborders = partitions.size() - 1;
  auto const sz       = partitions.size() + 1;
  // Number of elements less than P
  std::vector<DifferenceType> n_lt(sz, 0);
  // Number of elements less than equals P
  std::vector<DifferenceType> n_le(sz, 0);

  auto const n_l_elem = std::distance(lbegin, lend);

  std::size_t idx;

  for (idx = 0; idx < nborders; ++idx) {
    auto lb_it = std::lower_bound(lbegin, lend, partitions[idx]);
    if (lb_it != lbegin) --lb_it;
    auto ub_it = std::upper_bound(lbegin, lend, partitions[idx]);

    n_lt[idx + 1] = std::distance(lbegin, lb_it);
    n_le[idx + 1] = std::distance(lbegin, ub_it);
  }

  DASH_ASSERT(idx + 1 == sz);
  n_lt[idx + 1] = n_l_elem;
  n_le[idx + 1] = n_l_elem;

  return std::make_pair(n_lt, n_le);
}

template <typename DifferenceType, typename ArrayType>
inline void global_histogram(
    std::vector<DifferenceType> const& l_nlt,
    std::vector<DifferenceType> const& l_nle,
    ArrayType&                         g_nlt,
    ArrayType&                         g_nle)
{
  auto const sz_unit0 = g_nlt.pattern().local_size(dash::team_unit_t{0});

#if defined(DASH_ENABLE_ASSERTIONS)
  DASH_ASSERT_EQ(sz_unit0, 2, "invalid number of local elements at unit 0");
  auto const myid = g_nlt.pattern().team().myid();
  if (myid != 0) {
    DASH_ASSERT_EQ(
        g_nlt.pattern().local_size(myid), 1,
        "invalid number of local elements");
  }
#endif

  /* reduce among all units */
  std::fill(g_nlt.lbegin(), g_nlt.lend(), 0);
  std::fill(g_nle.lbegin(), g_nle.lend(), 0);

  // TODO: verify if this is really necessary,
  // depends on whether dash::transform is collective or non-collective
  g_nlt.team().barrier();

  dash::transform<DifferenceType>(
      &(*std::begin(l_nlt)), &(*(std::begin(l_nlt) + sz_unit0)),  // A
      g_nlt.begin(),                                              // B
      g_nlt.begin(),                  // B = op(B,A)
      dash::plus<DifferenceType>());  // op

  dash::transform<DifferenceType>(
      &(*std::begin(l_nle)), &(*(std::begin(l_nle) + sz_unit0)),  // A
      g_nle.begin(),                                              // B
      g_nle.begin(),                  // B = op(B,A)
      dash::plus<DifferenceType>());  // op

  using glob_atomic_ref_t = dash::GlobRef<dash::Atomic<DifferenceType>>;

  for (std::size_t idx = sz_unit0; idx < l_nlt.size(); ++idx) {
    // accumulate g_nlt
    auto ref_clt        = g_nlt.begin() + idx;
    auto atomic_ref_clt = glob_atomic_ref_t(ref_clt.dart_gptr());
    atomic_ref_clt.add(l_nlt[idx]);

    // accumulate g_nle
    auto ref_cle        = g_nle.begin() + idx;
    auto atomic_ref_cle = glob_atomic_ref_t(ref_cle.dart_gptr());
    atomic_ref_cle.add(l_nle[idx]);
  }
}

template <typename ElementType, typename DifferenceType, typename ArrayType>
inline bool validate_partitions(
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

  dash::copy(g_nle.begin(), g_nle.end(), l_nle.data());
  dash::copy(g_nlt.begin(), g_nlt.end(), l_nlt.data());

  for (std::size_t idx = 0; idx < partitions.size() - 1; ++idx) {
    if (l_nlt[idx + 1] < C[idx + 1] && C[idx + 1] <= l_nle[idx + 1]) {
      p_borders.is_stable[idx] = true;
    }
    else {
      if (l_nlt[idx + 1] <= C[idx + 1]) {
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
  using index_type         = typename decltype(pattern)::index_type;
  using size_type          = typename decltype(pattern)::size_type;
  using value_type         = typename decltype(begin)::value_type;
  using difference_type    = index_type;
  using const_pointer_type = typename decltype(begin)::const_pointer;

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

  dash::Team& team   = pattern.team();
  auto const  nunits = team.size();
  auto const  myid   = team.myid();

  std::vector<value_type> partitions(nunits, 0);

  // Starting offsets of all units
  std::vector<difference_type> C(nunits + 1, 0);

  /* number of elements less than (or equal) to splitter border accumulated */
  using pattern_t = dash::CSRPattern<1>;

  // Unit 0 is the only unit with two elements
  std::vector<size_type> lsizes(nunits);
  lsizes[0] = 2;
  std::fill(lsizes.begin() + 1, lsizes.end(), 1);
  pattern_t pat(lsizes, team);

  dash::Array<difference_type, index_type, pattern_t> g_CLT(pat);
  dash::Array<difference_type, index_type, pattern_t> g_CLE(pat);

  std::vector<difference_type> myT_CLT(nunits);
  std::vector<difference_type> myT_CLE(nunits);
  std::vector<difference_type> myT_C(nunits);
  std::vector<difference_type> myC(nunits + 1);

  // global distance
  auto       begin_ptr = static_cast<const_pointer_type>(begin);
  auto       end_ptr   = static_cast<const_pointer_type>(end);
  auto const n_g_elem  = dash::distance(begin_ptr, end_ptr);
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

  dash::team_unit_t unit{0};
  dash::team_unit_t last{static_cast<dart_unit_t>(nunits)};

  // Calculate starting offsets of unit
  for (; unit < last; ++unit) {
    C[unit.id + 1] = C[unit.id] + pattern.local_size(unit);
  }

  // TODO: provide dash::min_max
  auto const partitions_min_it = dash::min_element(begin, end);
  auto const partitions_max_it = dash::max_element(begin, end);

  auto const min = static_cast<value_type>(*partitions_min_it);
  auto const max = static_cast<value_type>(*partitions_max_it);

  auto const                          nboundaries = nunits - 1;
  detail::PartitionBorder<value_type> p_borders(nboundaries, min, max);

  size_t iter = 0;
  bool   done = false;

  do {
    iter++;

    detail::calculate_boundaries(p_borders, partitions);

    auto histograms = detail::local_histogram<value_type, difference_type>(
        partitions, lbegin, lend);

    auto l_nlt = histograms.first;
    auto l_nle = histograms.second;

    detail::global_histogram(l_nlt, l_nle, g_CLT, g_CLE);

    team.barrier();

    // TODO: this step may be more efficient with block wise copy local
    // communication
    DASH_ASSERT(g_CLE.size() == l_nle.size());
    DASH_ASSERT(g_CLT.size() == l_nlt.size());

    done = validate_partitions(p_borders, partitions, C, g_CLT, g_CLE);

  } while (!done);

  auto histograms = detail::local_histogram<value_type, difference_type>(
      partitions, lbegin, lend);
}
}  // namespace dash

#endif
