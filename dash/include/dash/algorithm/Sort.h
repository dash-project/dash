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

#include <dash/internal/Logging.h>
#include <dash/util/Trace.h>

namespace dash {

#ifdef DOXYGEN

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

#else

#define IDX_DIST(nunits) ((nunits)*0)
#define IDX_SUPP(nunits) ((nunits)*1)
#define IDX_TARGET_DISP(nunits) ((nunits)*2)

#define IDX_SEND_COUNT(nunits) IDX_DIST(nunits)
#define IDX_TARGET_COUNT(nunits) IDX_SUPP(nunits)

#define NLT_NLE_BLOCK 2

namespace detail {

struct UnitInfo {
  std::size_t nunits;
  // prefix sum over the number of local elements of all unit
  std::vector<size_t>                acc_partition_count;
  std::vector<dash::default_index_t> valid_remote_partitions;

  UnitInfo(std::size_t p_nunits)
    : nunits(p_nunits)
    , acc_partition_count(nunits + 1)
  {
    valid_remote_partitions.reserve(nunits - 1);
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

  // The right unit is always right next to the border. For this reason we
  // track  only the left unit.
  std::vector<dash::default_index_t> left_partition;

  PartitionBorder(size_t nsplitter, T _lower_bound, T _upper_bound)
    : is_stable(nsplitter, false)
    , is_skipped(nsplitter, false)
    , lower_bound(nsplitter, _lower_bound)
    , upper_bound(nsplitter, _upper_bound)
    , is_last_iter(nsplitter, false)
    , left_partition(
          nsplitter, std::numeric_limits<dash::default_index_t>::min())
  {
  }
};

template <typename T>
inline void psort__calc_boundaries(
    PartitionBorder<T>& p_borders, std::vector<T>& partitions)
{
  DASH_LOG_TRACE("< psort__calc_boundaries ");
  DASH_ASSERT_EQ(
      p_borders.is_stable.size(),
      partitions.size(),
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

template <typename ElementType, typename MappedType, typename SortableHash>
inline std::pair<std::vector<std::size_t>, std::vector<std::size_t>>
psort__local_histogram(
    std::vector<MappedType> const&     partitions,
    std::vector<size_t> const&         valid_partitions,
    PartitionBorder<MappedType> const& p_borders,
    ElementType const*                 lbegin,
    ElementType const*                 lend,
    SortableHash&&                     sortable_hash)
{
  DASH_LOG_TRACE("< psort__local_histogram");

  auto const nborders = partitions.size();
  // The first element is 0 and the last element is the total number of local
  // elements in this unit
  auto const sz = partitions.size() + 2;
  // Number of elements less than P
  std::vector<std::size_t> n_lt(sz, 0);
  // Number of elements less than equals P
  std::vector<std::size_t> n_le(sz, 0);

  auto const n_l_elem = std::distance(lbegin, lend);

  if (n_l_elem == 0) return std::make_pair(n_lt, n_le);

  auto comp_lower = [&sortable_hash](
                        const ElementType& a, const MappedType& b) {
    return sortable_hash(const_cast<ElementType&>(a)) < b;
  };

  auto comp_upper = [&sortable_hash](
                        const MappedType& b, const ElementType& a) {
    return b < sortable_hash(const_cast<ElementType&>(a));
  };

  for (auto const& idx : valid_partitions) {
    auto lb_it = std::lower_bound(lbegin, lend, partitions[idx], comp_lower);
    auto ub_it = std::upper_bound(lbegin, lend, partitions[idx], comp_upper);

    auto const p_left = p_borders.left_partition[idx];
    DASH_ASSERT_NE(p_left, dash::team_unit_t{}, "invalid bounding unit");

    n_lt[p_left + 1] = std::distance(lbegin, lb_it);
    n_le[p_left + 1] = std::distance(lbegin, ub_it);
  }

  auto const last_valid_border_idx = *std::prev(valid_partitions.cend());

  auto const p_left = p_borders.left_partition[last_valid_border_idx];

  std::fill(
      std::next(std::begin(n_lt), p_left + 2), std::end(n_lt), n_l_elem);
  std::fill(
      std::next(std::begin(n_le), p_left + 2), std::end(n_le), n_l_elem);

  DASH_LOG_TRACE("psort__local_histogram >");
  return std::make_pair(std::move(n_lt), std::move(n_le));
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

  auto const last_valid_border = *std::prev(valid_partitions.cend());

  // TODO: implement dash::transform_async
  // We have to add 2
  // --> +1 because we start at idx 1
  // --> +1 to get the last idx

  bool has_pending_op = false;

  for (std::size_t idx = 1; idx < last_valid_border + 2; ++idx) {
    // we communicate only non-zero values
    if (l_nlt[idx] == 0 && l_nle[idx] == 0) continue;

    std::array<SizeType, NLT_NLE_BLOCK> vals{{l_nlt[idx], l_nle[idx]}};
    auto const                          g_idx_nlt = (idx - 1) * NLT_NLE_BLOCK;

    DASH_ASSERT_RETURNS(
        dart_accumulate(
            // dart pointer to first element in target range
            (it_nlt_nle + g_idx_nlt).dart_gptr(),
            // values
            &*(std::begin(vals)),
            // nvalues
            NLT_NLE_BLOCK,
            // dart type
            dash::dart_datatype<SizeType>::value,
            // dart op
            dash::plus<SizeType>().dart_operation()),
        DART_OK);

    has_pending_op = true;
  }

  // flush all outstanding dart_accumulate operations...
  if (has_pending_op) {
    DASH_ASSERT_RETURNS(dart_flush_all(it_nlt_nle.dart_gptr()), DART_OK);
  }

  team.barrier();
  DASH_LOG_TRACE("psort__global_histogram >");
}

template <typename ElementType, typename GlobIter>
bool psort__validate_partitions(
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

  // TODO: dash::copy does not work with BLOCKCYCLIC patterns.
  // TODO: add unit test and create issue
  //
  // For this reason we explicitly use handles instead of dash::copy_async.
  // Moreover it is faster since dash::copy makes a lot of pattern stuff and
  // we definitely know what we are doing here.
  std::vector<dart_handle_t> handles{};

  std::transform(
      p_unit_info.valid_remote_partitions.begin(),
      p_unit_info.valid_remote_partitions.end(),
      std::back_inserter(handles),
      [it_g_nlt_nle, l_nlt_nle_data](std::size_t p) {
        // All elements in input range are remote
        index_t lidx      = p * NLT_NLE_BLOCK;
        auto    src_begin = it_g_nlt_nle + lidx;

        dart_handle_t handle;
        dash::internal::get_handle(
            src_begin.dart_gptr(),
            l_nlt_nle_data + lidx,
            NLT_NLE_BLOCK,
            &handle);
        return handle;
      });

  DASH_ASSERT_EQ(
      handles.size(),
      p_unit_info.valid_remote_partitions.size(),
      "invalid size of handles vector");

  auto const last_p   = p_unit_info.valid_remote_partitions.back();
  auto       out_last = l_nlt_nle_data + last_p * NLT_NLE_BLOCK;

  // copy local portion
  index_t lidx      = myid * NLT_NLE_BLOCK;
  auto    cpy_begin = it_g_nlt_nle + lidx;
  auto    lbegin    = cpy_begin.local();
  std::copy(lbegin, lbegin + NLT_NLE_BLOCK, l_nlt_nle_data + lidx);

  // wait for all copies
  if (handles.size() > 0) {
    if (dart_waitall_local(handles.data(), handles.size()) != DART_OK) {
      DASH_LOG_ERROR(
          "dash::detail::psort_validate_partitions [Future]",
          "  dart_waitall_local failed");
      DASH_THROW(
          dash::exception::RuntimeError,
          "dash::detail::psort_validate_partitions [Future]: "
          "dart_waitall_local failed");
    }
  }

  auto const& acc_partition_count = p_unit_info.acc_partition_count;

  for (auto const& border_idx : valid_partitions) {
    auto const p_left  = p_borders.left_partition[border_idx];
    auto const nlt_idx = p_left * NLT_NLE_BLOCK;

    auto const peer_idx = p_left + 1;

    if (l_nlt_nle[nlt_idx] < acc_partition_count[peer_idx] &&
        acc_partition_count[peer_idx] <= l_nlt_nle[nlt_idx + 1]) {
      p_borders.is_stable[border_idx] = true;
    }
    else {
      if (l_nlt_nle[nlt_idx] >= acc_partition_count[peer_idx]) {
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
    std::vector<typename LocalArrayT::value_type> const& acc_partition_count,
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
  auto const supp_begin = l_partition_dist.begin() + IDX_SUPP(nunits);
  auto       dist_begin = l_partition_dist.begin() + IDX_DIST(nunits);

  auto const n_my_elements = std::accumulate(
      dist_begin, dist_begin + nunits, static_cast<size_t>(0));

  // Calculate the deficit
  auto my_deficit = acc_partition_count[myid + 1] - n_my_elements;

  // If there is a deficit, look how much unit j can supply
  for (auto unit = team_unit_t{0}; unit < nunits && my_deficit > 0; ++unit) {
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
    PartitionBorder<ElementType> const& p_borders,
    std::vector<size_t> const&          valid_partitions,
    LocalArrayT&                        partition_dist)
{
  using value_t = typename LocalArrayT::value_type;
  DASH_LOG_TRACE("< psort__calc_send_count");

  auto const           nunits = partition_dist.pattern().team().size();
  std::vector<value_t> tmp_target_count(nunits + 1);

  tmp_target_count[0] = 0;

  auto l_target_count = &(partition_dist[IDX_TARGET_COUNT(nunits)]);
  auto l_send_count   = &(partition_dist[IDX_SEND_COUNT(nunits)]);

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

    auto const p_left         = p_borders.left_partition[*it_valid];
    auto const n_contig_skips = *it_valid - p_left;
    std::fill_n(
        l_target_count + p_left + 1, n_contig_skips, l_target_count[p_left]);

    std::advance(it_skipped, n_contig_skips);
    std::advance(it_valid, 1);
  }

  std::copy(
      l_target_count, l_target_count + nunits, tmp_target_count.begin() + 1);

  std::transform(
      tmp_target_count.begin() + 1,
      tmp_target_count.end(),
      tmp_target_count.begin(),
      l_send_count,
      std::minus<value_t>());

  DASH_LOG_TRACE("psort__calc_send_count >");
}

template <typename SizeType, typename ElementType>
inline void psort__calc_target_displs(
    PartitionBorder<ElementType> const& p_borders,
    std::vector<size_t> const&          valid_partitions,
    dash::Array<SizeType>&              g_partition_data)
{
  DASH_LOG_TRACE("< psort__calc_target_displs");
  auto const nunits = g_partition_data.team().size();
  auto const myid   = g_partition_data.team().myid();

  auto l_target_displs = &(g_partition_data.local[IDX_TARGET_DISP(nunits)]);

  if (0 == myid) {
    // Unit 0 always writes to target offset 0
    std::fill(l_target_displs, l_target_displs + nunits, 0);
  }

  std::vector<SizeType> target_displs(nunits, 0);

  auto const u_blocksize = g_partition_data.lsize();

  for (size_t idx = 0; idx < valid_partitions.size(); ++idx) {
    auto const     border_idx = valid_partitions[idx];
    auto const     left_u     = p_borders.left_partition[border_idx];
    auto const     right_u    = border_idx + 1;
    SizeType const val =
        (left_u == myid)
            ? g_partition_data.local[left_u + IDX_SEND_COUNT(nunits)]
            : g_partition_data
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

template <typename GlobIterT>
inline UnitInfo psort__find_partition_borders(
    typename GlobIterT::pattern_type const& pattern,
    GlobIterT const                         begin,
    GlobIterT const                         end)
{
  DASH_LOG_TRACE("< psort__find_partition_borders");

  auto const nunits = pattern.team().size();
  auto const myid   = pattern.team().myid();

  dash::team_unit_t       unit{0};
  const dash::team_unit_t last{static_cast<dart_unit_t>(nunits)};

  auto const unit_first = pattern.unit_at(begin.pos());
  auto const unit_last  = pattern.unit_at(end.pos() - 1);

  // Starting offsets of all units
  UnitInfo unit_info(nunits);
  auto&    acc_partition_count = unit_info.acc_partition_count;
  acc_partition_count[0]       = 0;

  for (; unit < last; ++unit) {
    // Number of elements located at current source unit:
    auto const u_extents = pattern.local_extents(unit);
    auto const u_size    = std::accumulate(
        std::begin(u_extents),
        std::end(u_extents),
        1,
        std::multiplies<std::size_t>());
    // first linear global index of unit
    auto const u_gidx_begin =
        (unit == myid) ? pattern.lbegin() : pattern.global_index(unit, {});
    // last global index of unit
    auto const u_gidx_end = u_gidx_begin + u_size;

    DASH_LOG_TRACE(
        "local indexes",
        unit,
        ": ",
        u_gidx_begin,
        " ",
        u_size,
        " ",
        u_gidx_end);

    if (u_size == 0 || u_gidx_end - 1 < begin.pos() ||
        u_gidx_begin >= end.pos()) {
      // This unit does not participate...
      acc_partition_count[unit + 1] = acc_partition_count[unit];
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

      acc_partition_count[unit + 1] =
          n_u_elements + acc_partition_count[unit];
      if (unit != myid) unit_info.valid_remote_partitions.push_back(unit);
    }
  }

  DASH_LOG_TRACE("psort__find_partition_borders >");
  return unit_info;
}

template <typename T>
inline void psort__init_partition_borders(
    UnitInfo const& unit_info, detail::PartitionBorder<T>& p_borders)
{
  DASH_LOG_TRACE("< psort__init_partition_borders");

  auto const& acc_partition_count = unit_info.acc_partition_count;

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

  auto const get_border_idx = [](std::size_t const& idx) {
    return (idx % NLT_NLE_BLOCK) ? (idx / NLT_NLE_BLOCK) * NLT_NLE_BLOCK
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

  // find subsequent partitions
  left = right;

  while ((right = std::upper_bound(right, last, *right)) != last) {
    auto const last_border_idx = border_idx;

    p_left     = std::distance(acc_partition_count.cbegin(), left) - 1;
    right_u    = std::distance(acc_partition_count.cbegin(), right) - 1;
    border_idx = get_border_idx(right_u);

    auto const dist = border_idx - last_border_idx;

    // mark all skipped partitions as stable and skipped
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

  DASH_LOG_TRACE("psort__init_partition_borders >");
}

template <typename T>
struct identity_t : std::unary_function<T, T> {
  constexpr T&& operator()(T&& t) const noexcept
  {
    // A perfect forwarding identity function
    return std::forward<T>(t);
  }
};

/*
 * Obtaining parameter types and return type from a lambda:
 * see http://coliru.stacked-crooked.com/a/6a87fadcf44c6a0f
 */

template <typename T>
struct closure_traits : closure_traits<decltype(&T::operator())> {
};

#define REM_CTOR(...) __VA_ARGS__
#define SPEC(cv, var, is_var)                                                 \
  template <typename C, typename R, typename... Args>                         \
  struct closure_traits<R (C::*)(Args... REM_CTOR var) cv> {                  \
    using arity       = std::integral_constant<std::size_t, sizeof...(Args)>; \
    using is_variadic = std::integral_constant<bool, is_var>;                 \
    using is_const    = std::is_const<int cv>;                                \
                                                                              \
    using result_type = R;                                                    \
                                                                              \
    template <std::size_t i>                                                  \
    using arg = typename std::tuple_element<i, std::tuple<Args...>>::type;    \
  };

SPEC(const, (, ...), 1)
SPEC(const, (), 0)
SPEC(, (, ...), 1)
SPEC(, (), 0)

}  // namespace detail

template <
    class GlobRandomIt,
    class SortableHash =
        detail::identity_t<typename GlobRandomIt::value_type&>>
void sort(
    GlobRandomIt begin,
    GlobRandomIt end,
    SortableHash sortable_hash = SortableHash())
{
  using iter_type    = GlobRandomIt;
  using pattern_type = typename iter_type::pattern_type;
  using value_type   = typename iter_type::value_type;
  using mapped_type  = typename std::decay<
      typename detail::closure_traits<SortableHash>::result_type>::type;

  static_assert(
      std::is_arithmetic<mapped_type>::value,
      "Only arithmetic types are supported");

  auto pattern = begin.pattern();

  dash::util::Trace trace("Sort");

  if (pattern.team() == dash::Team::Null()) {
    DASH_LOG_TRACE("dash::sort", "Sorting on dash::Team::Null()");
    return;
  }
  else if (pattern.team().size() == 1) {
    DASH_LOG_TRACE("dash::sort", "Sorting on a team with only 1 unit");
    trace.enter_state("final_local_sort");
    std::sort(begin.local(), end.local());
    trace.exit_state("final_local_sort");
    return;
  }

  if (begin >= end) {
    DASH_LOG_TRACE("dash::sort", "empty range");
    trace.enter_state("final_barrier");
    pattern.team().barrier();
    trace.exit_state("final_barrier");
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

  auto const sort_comp = [&sortable_hash](
                             const value_type& a, const value_type& b) {
    return sortable_hash(const_cast<value_type&>(a)) <
           sortable_hash(const_cast<value_type&>(b));
  };

  // initial local_sort
  trace.enter_state("1:initial_local_sort");
  std::sort(lbegin, lend, sort_comp);
  trace.exit_state("1:initial_local_sort");

  trace.enter_state("2:init_temporary_global_data");

  auto const lmin = (n_l_elem > 0) ? sortable_hash(*lbegin)
                                   : std::numeric_limits<mapped_type>::max();
  auto const lmax = (n_l_elem > 0) ? sortable_hash(*(lend - 1))
                                   : std::numeric_limits<mapped_type>::min();

  dash::Shared<dash::Atomic<mapped_type>> g_min(dash::team_unit_t{0}, team);
  dash::Shared<dash::Atomic<mapped_type>> g_max(dash::team_unit_t{0}, team);

  g_min.get().op(dash::min<mapped_type>(), lmin);
  g_max.get().op(dash::max<mapped_type>(), lmax);

  using array_t = dash::Array<std::size_t>;

  std::size_t gsize = nunits * NLT_NLE_BLOCK * 2;

  array_t g_nlt_nle(gsize, dash::BLOCKCYCLIC(NLT_NLE_BLOCK), team);

  array_t g_partition_data(nunits * nunits * 3, dash::BLOCKED, team);
  std::fill(g_partition_data.lbegin(), g_partition_data.lend(), 0);

  auto const min = static_cast<mapped_type>(g_min.get());
  auto const max = static_cast<mapped_type>(g_max.get());

  trace.exit_state("2:init_temporary_global_data");

  trace.enter_state("3:init_temporary_local_data");

  auto const p_unit_info =
      detail::psort__find_partition_borders(pattern, begin, end);

  auto const& acc_partition_count = p_unit_info.acc_partition_count;

  auto const               nboundaries = nunits - 1;
  std::vector<mapped_type> partitions(nboundaries, mapped_type{});

  detail::PartitionBorder<mapped_type> p_borders(nboundaries, min, max);

  detail::psort__init_partition_borders(p_unit_info, p_borders);

  DASH_LOG_TRACE_RANGE("locally sorted array", lbegin, lend);
  DASH_LOG_TRACE_RANGE(
      "skipped partitions",
      p_borders.is_skipped.cbegin(),
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
        all_borders.begin(),
        all_borders.end(),
        std::back_inserter(valid_partitions),
        [&is_skipped](size_t idx) { return is_skipped[idx] == false; });
  }

  if (valid_partitions.size() == 0) {
    // Edge case: We may have a team spanning at least 2 units, however the
    // global range is owned by  only 1 unit
    team.barrier();
    return;
  }

  // Temporary local buffer (sorted);
  std::vector<value_type> const lcopy(lbegin, lend);

  trace.exit_state("3:init_temporary_local_data");

  trace.enter_state("4:find_global_partition_borders");

  do {
    detail::psort__calc_boundaries(p_borders, partitions);

    auto const histograms = detail::psort__local_histogram(
        partitions, valid_partitions, p_borders, lbegin, lend, sortable_hash);

    auto const& l_nlt = histograms.first;
    auto const& l_nle = histograms.second;

    DASH_LOG_TRACE_RANGE("local histogram nlt", l_nlt.begin(), l_nlt.end());

    DASH_LOG_TRACE_RANGE("local histogram nle", l_nle.begin(), l_nle.end());

    detail::psort__global_histogram(l_nlt, l_nle, valid_partitions, cur);

    DASH_LOG_TRACE_RANGE(
        "global histogram",
        (cur + (myid * NLT_NLE_BLOCK)).local(),
        (cur + (myid * NLT_NLE_BLOCK)).local() + NLT_NLE_BLOCK);

    done = detail::psort__validate_partitions(
        p_unit_info, partitions, valid_partitions, p_borders, cur);

    // This swap eliminates a barrier as, otherwise, some units may be one
    // iteration ahead and modify shared data while the others are still not
    // done
    std::swap(cur, tmp);
  } while (!done);

  trace.exit_state("4:find_global_partition_borders");

  trace.enter_state("5:final_local_histogram");
  auto histograms = detail::psort__local_histogram(
      partitions, valid_partitions, p_borders, lbegin, lend, sortable_hash);
  trace.exit_state("5:final_local_histogram");

  /* How many elements are less than P
   * or less than equals P */
  auto& l_nlt = histograms.first;
  auto& l_nle = histograms.second;

  DASH_ASSERT_EQ(
      histograms.first.size(),
      histograms.second.size(),
      "length of histogram arrays does not match");

  DASH_LOG_TRACE_RANGE(
      "final partition borders", partitions.begin(), partitions.end());
  DASH_LOG_TRACE_RANGE("final histograms: l_nlt", l_nlt.begin(), l_nlt.end());
  DASH_LOG_TRACE_RANGE("final histograms: l_nle", l_nle.begin(), l_nle.end());

  trace.enter_state("6:transpose_local_histograms (all-to-all)");
  if (n_l_elem > 0) {
    // TODO: minimize communication to copy only until the last valid border
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
          g_partition_data.async[offset + IDX_DIST(nunits)].set(
              &(l_nlt[idx]));

        if (l_nle[idx] > 0)
          g_partition_data.async[offset + IDX_SUPP(nunits)].set(
              &(l_nle[idx]));
      }
      else {
        g_partition_data.local[myid + IDX_DIST(nunits)] = l_nlt[idx];
        g_partition_data.local[myid + IDX_SUPP(nunits)] = l_nle[idx];
      }
    }
    // complete outstanding requests...
    g_partition_data.async.flush();
  }
  trace.exit_state("6:transpose_local_histograms (all-to-all)");

  trace.enter_state("7:barrier");
  team.barrier();
  trace.exit_state("7:barrier");

  DASH_LOG_TRACE_RANGE(
      "initial partition distribution:",
      g_partition_data.lbegin() + IDX_DIST(nunits),
      g_partition_data.lbegin() + IDX_DIST(nunits) + nunits);

  DASH_LOG_TRACE_RANGE(
      "partition supply",
      g_partition_data.lbegin() + IDX_SUPP(nunits),
      g_partition_data.lbegin() + IDX_SUPP(nunits) + nunits);

  /* Calculate final distribution per partition. Each unit calculates their
   * local distribution independently.
   * All accesses are only to local memory
   */

  trace.enter_state("8:calc_final_partition_dist");

  detail::psort__calc_final_partition_dist(
      acc_partition_count, g_partition_data.local);

  DASH_LOG_TRACE_RANGE(
      "final partition distribution",
      g_partition_data.lbegin() + IDX_DIST(nunits),
      g_partition_data.lbegin() + IDX_DIST(nunits) + nunits);

  // Reset local elements to 0 since the following matrix transpose
  // communicates only non-zero values and writes to exactly these offsets.
  std::fill(
      &(g_partition_data.local[IDX_TARGET_COUNT(nunits)]),
      &(g_partition_data.local[IDX_TARGET_COUNT(nunits) + nunits]),
      0);

  trace.exit_state("8:calc_final_partition_dist");

  trace.enter_state("9:barrier");
  team.barrier();
  trace.exit_state("9:barrier");

  trace.enter_state("10:transpose_final_partition_dist (all-to-all)");
  /*
   * Transpose the final distribution again to obtain the end offsets
   */
  dash::team_unit_t unit{0};
  auto const        last = static_cast<dash::team_unit_t>(nunits);

  for (; unit < last; ++unit) {
    if (g_partition_data.local[IDX_DIST(nunits) + unit] == 0) continue;

    if (unit != myid) {
      // We communicate only non-zero values
      auto const offset = unit * g_partition_data.lsize() + myid;
      g_partition_data.async[offset + IDX_TARGET_COUNT(nunits)].set(
          &(g_partition_data.local[IDX_DIST(nunits) + unit]));
    }
    else {
      g_partition_data.local[IDX_TARGET_COUNT(nunits) + myid] =
          g_partition_data.local[IDX_DIST(nunits) + unit];
    }
  }

  g_partition_data.async.flush();

  trace.exit_state("10:transpose_final_partition_dist (all-to-all)");

  trace.enter_state("11:barrier");
  team.barrier();
  trace.exit_state("11:barrier");

  DASH_LOG_TRACE_RANGE(
      "final target count",
      g_partition_data.lbegin() + IDX_TARGET_COUNT(nunits),
      g_partition_data.lbegin() + IDX_TARGET_COUNT(nunits) + nunits);

  trace.enter_state("12:calc_final_send_count");

  std::vector<std::size_t> l_send_displs(nunits, 0);

  if (n_l_elem > 0) {
    detail::psort__calc_send_count(
        p_borders, valid_partitions, g_partition_data.local);

    // Obtain the final send displs which is just an exclusive scan based on
    // the send count
    auto l_send_count = &(g_partition_data.local[IDX_SEND_COUNT(nunits)]);
    std::copy(l_send_count, l_send_count + nunits, std::begin(l_send_displs));

    std::partial_sum(
        std::begin(l_send_displs),
        std::end(l_send_displs),
        std::begin(l_send_displs),
        std::plus<size_t>());

    // Shift by 1 and fill leading element with 0 to obtain the exclusive scan
    // character
    l_send_displs.insert(std::begin(l_send_displs), 0);
    l_send_displs.pop_back();
  }
  else {
    std::fill(
        g_partition_data.lbegin() + IDX_SEND_COUNT(nunits),
        g_partition_data.lbegin() + IDX_SEND_COUNT(nunits) + nunits,
        0);
  }

#if defined(DASH_ENABLE_ASSERTIONS) && defined(DASH_ENABLE_TRACE_LOGGING)
  {
    std::vector<size_t> chksum(nunits, 0);

    DASH_ASSERT_RETURNS(
        dart_allreduce(
            g_partition_data.lbegin() + IDX_SEND_COUNT(nunits),
            chksum.data(),
            nunits,
            dart_datatype<size_t>::value,
            DART_OP_SUM,
            team.dart_id()),
        DART_OK);

    DASH_ASSERT_EQ(
        chksum[myid.id],
        n_l_elem,
        "send count must match the capacity of the unit");
  }
#endif

  DASH_LOG_TRACE_RANGE(
      "send count",
      g_partition_data.lbegin() + IDX_DIST(nunits),
      g_partition_data.lbegin() + IDX_DIST(nunits) + nunits);

  DASH_LOG_TRACE_RANGE(
      "send displs", l_send_displs.begin(), l_send_displs.end());

  trace.exit_state("12:calc_final_send_count");

  trace.enter_state("13:barrier");
  team.barrier();
  trace.exit_state("13:barrier");

  trace.enter_state("14:calc_final_target_displs");

  if (n_l_elem > 0) {
    detail::psort__calc_target_displs(
        p_borders, valid_partitions, g_partition_data);
  }

  trace.exit_state("14:calc_final_target_displs");

  trace.enter_state("15:barrier");
  team.barrier();
  trace.exit_state("15:barrier");

  DASH_LOG_TRACE_RANGE(
      "target displs",
      &(g_partition_data.local[IDX_TARGET_DISP(nunits)]),
      &(g_partition_data.local[IDX_TARGET_DISP(nunits)]) + nunits);

  trace.enter_state("16:exchange_data (all-to-all)");

  std::vector<dash::Future<iter_type>> async_copies{};
  async_copies.reserve(p_unit_info.valid_remote_partitions.size());

  DASH_LOG_TRACE_RANGE("before final sort round", lbegin, lend);

  auto const l_partition_data = g_partition_data.local;

  auto const get_send_info = [l_partition_data, &l_send_displs, nunits](
                                 dash::default_index_t const p_idx) {
    auto const send_count = l_partition_data[p_idx + IDX_SEND_COUNT(nunits)];
    auto const target_disp =
        l_partition_data[p_idx + IDX_TARGET_DISP(nunits)];
    auto const send_disp = l_send_displs[p_idx];
    return std::make_tuple(send_count, send_disp, target_disp);
  };

  std::size_t send_count, send_disp, target_disp;

  for (auto const& unit : p_unit_info.valid_remote_partitions) {
    std::tie(send_count, send_disp, target_disp) = get_send_info(unit);

    iter_type it_copy{};
    if (unit == unit_at_begin) {
      it_copy = begin;
    }
    else {
      // The array passed to global_index is 0 initialized
      auto gidx =
          pattern.global_index(static_cast<dash::team_unit_t>(unit), {});
      it_copy = iter_type{&(begin.globmem()), pattern, gidx};
    }

    auto&& fut = dash::copy_async(
        &(*(lcopy.begin() + send_disp)),
        &(*(lcopy.begin() + send_disp + send_count)),
        it_copy + target_disp);

    async_copies.emplace_back(std::move(fut));
  }

  std::tie(send_count, send_disp, target_disp) = get_send_info(myid);

  if (send_count) {
    std::copy(
        lcopy.begin() + send_disp,
        lcopy.begin() + send_disp + send_count,
        lbegin + target_disp);
  }

  std::for_each(
      std::begin(async_copies),
      std::end(async_copies),
      [](dash::Future<iter_type>& fut) { fut.wait(); });

  trace.exit_state("16:exchange_data (all-to-all)");

  trace.enter_state("17:barrier");
  team.barrier();
  trace.exit_state("17:barrier");

  trace.enter_state("18:final_local_sort");
  std::sort(lbegin, lend, sort_comp);
  trace.exit_state("18:final_local_sort");
  DASH_LOG_TRACE_RANGE("finally sorted range", lbegin, lend);

  trace.enter_state("19:final_barrier");
  team.barrier();
  trace.exit_state("19:final_barrier");
}

#endif  // DOXYGEN

}  // namespace dash

#endif
