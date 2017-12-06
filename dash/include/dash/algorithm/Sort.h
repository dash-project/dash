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
struct PartitionInfo {
public:
  bool is_last_iter;
  bool is_stable;
  T    lower_bound;
  T    upper_bound;

  PartitionInfo()
    : is_last_iter(false)
    , is_stable(false)
    , lower_bound(std::numeric_limits<T>::min())
    , upper_bound(std::numeric_limits<T>::max())
  {
  }
  PartitionInfo(
      bool _is_last_iter, bool _is_stable, T _lower_bound, T _upper_bound)
    : is_last_iter(_is_last_iter)
    , is_stable(_is_stable)
    , lower_bound(_lower_bound)
    , upper_bound(_upper_bound)
  {
  }
};
} //namespace detail

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
  } else if (pattern.team().size() == 1) {
    DASH_LOG_DEBUG("dash::sort", "Sorting on a team with only 1 unit");
    std::sort(begin.local(), end.local());
    return;
  }


  dash::Team & team = pattern.team();
  auto const nunits = team.size();
  auto const myid   = team.myid();

  std::vector<value_type> partitions(nunits, 0);

  //Starting offsets of all units
  std::vector<difference_type> C(nunits + 1, 0);
  //Number of elements less than splitter border
  std::vector<difference_type> l_CLT(nunits + 1);
  //Number of elements lass than or equal to splitter border
  std::vector<difference_type> l_CLE(nunits + 1);

  /* number of elements less than (or equal) to splitter border accumulated */
  using pattern_t = dash::CSRPattern<1>;

  //Unit 0 is the only unit with two elements
  std::vector<size_type> lsizes(nunits + 1);
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

  auto const n_l_elem = l_range.begin - l_range.end;

  auto lbegin = l_mem_begin + l_range.begin;
  auto lend   = l_mem_begin + l_range.end;

  std::sort(lbegin, lend);

  // collect local sizes of other units
  // TODO: maybe there is a better approach than attaching to a separate array
  // dash::Array<size_type> local_sizes(pattern.team().size(),
  // pattern.team());
  // local_sizes.local[0] = n_l_elem;
  //
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

  // We have n
  using partition_info_t = detail::PartitionInfo<value_type>;
  auto const npartitions = nunits - 1;
  //TODO: use back_inserter instead  of generate
  std::vector<partition_info_t> sort_partition_info{npartitions};

  //init partitions
  std::generate(
      std::begin(sort_partition_info), std::end(sort_partition_info),
      [max, min]() { return partition_info_t(false, false, min, max); });


  size_t iter = 0, idx;

  auto it_unstable = sort_partition_info.begin();
  do {
    iter++;

    DASH_ASSERT(npartitions == sort_partition_info.size());

    //recalculate partition boundaries
    for (idx = 0; idx < npartitions; ++idx) {
      auto & p_info = sort_partition_info.at(idx);
      //case A: partition is already stable -> done
      if (p_info.is_stable) continue;
      //case B: we have the last iteration
      //-> test upper bound directly
      if (p_info.is_last_iter) {
        partitions[idx] = p_info.upper_bound;
        p_info.is_stable = true;
      } else {
        //case C: ordinary iteration
        partitions[idx] = (p_info.lower_bound + p_info.upper_bound) / 2;

        if (partitions[idx] == p_info.lower_bound) {
          //if we cannot move the partition to the left
          //-> last iteration
          p_info.is_last_iter = true;
        }
      }
    }

    l_CLT[0] = 0;
    l_CLE[0] = 0;

    /* histogram of l_CLT and l_CLE */
    for (idx = 0; idx < npartitions; ++idx)
    {
      auto lb_it = std::upper_bound(lbegin, lend, partitions[idx] - 1);
      auto ub_it = std::upper_bound(lbegin, lend, partitions[idx]);

      l_CLT[idx + 1] = std::distance(lbegin, lb_it);
      l_CLE[idx + 1] = std::distance(lbegin, ub_it);
    }

    l_CLT[idx + 1] = n_l_elem;
    l_CLE[idx + 1] = n_l_elem;

    /* reduce among all units */
    std::fill(g_CLT.lbegin(), g_CLT.lend(), 0);
    std::fill(g_CLT.lbegin(), g_CLT.lend(), 0);

    team.barrier();

    DASH_ASSERT(lsizes[0] == 2);
    dash::transform<difference_type>(std::begin(l_CLT), std::begin(l_CLT) + 2, g_CLT.begin(), dash::plus<difference_type>());
    dash::transform<difference_type>(std::begin(l_CLE), std::begin(l_CLE) + 2, g_CLE.begin(), dash::plus<difference_type>());

    using glob_atomic_ref_t = dash::GlobRef<dash::Atomic<difference_type>>;
    for (idx = 2; idx < l_CLT.size(); ++idx) {
      DASH_ASSERT(lsizes[idx] == 1);

      //accumulate g_CLT
      auto ref_clt = g_CLT.begin() + idx;
      auto atomic_ref_clt = glob_atomic_ref_t(ref_clt.dart_gptr());
      atomic_ref_clt.add(l_CLT[idx]);

      //accumulate g_CLE
      auto ref_cle = g_CLE.begin() + idx;
      auto atomic_ref_cle = glob_atomic_ref_t(ref_cle.dart_gptr());
      atomic_ref_cle.add(l_CLE[idx]);
    }


    team.barrier();

    //TODO: this step may be more efficient with block wise copy local
    //communication
    DASH_ASSERT(g_CLE.size() == l_CLE.size());
    DASH_ASSERT(g_CLT.size() == l_CLT.size());
    dash::copy(g_CLE.begin(), g_CLE.end(), l_CLE.data());
    dash::copy(g_CLT.begin(), g_CLT.end(), l_CLT.data());

    for (idx = 0; idx < npartitions; ++idx) {
      auto & p_info = sort_partition_info.at(idx);
      if (l_CLT[idx + 1] < C[idx + 1] && C[idx + 1] <= l_CLE[idx + 1]) {
        p_info.is_stable = true;
      } else {
        if (l_CLT[idx + 1] <= C[idx + 1]) {
          p_info.upper_bound = partitions[idx];
        } else {
          p_info.lower_bound = partitions[idx];
        }
      }
    }

    auto it_unstable = std::find_if(sort_partition_info.cbegin(), sort_partition_info.cend(),
        [](const partition_info_t & info) {return info.is_stable == false;});

  } while(it_unstable != sort_partition_info.end());
}


}  // namespace dash

#endif
