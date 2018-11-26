#ifndef DASH__ALGORITHM__INTERNAL__SORT_H__INCLUDED
#define DASH__ALGORITHM__INTERNAL__SORT_H__INCLUDED

#define IDX_DIST(nunits) ((nunits)*0)
#define IDX_SUPP(nunits) ((nunits)*1)
#define IDX_TARGET_DISP(nunits) ((nunits)*2)

#define IDX_SEND_COUNT(nunits) IDX_DIST(nunits)
#define IDX_TARGET_COUNT(nunits) IDX_SUPP(nunits)

#define NLT_NLE_BLOCK 2

#include <algorithm>
#include <cstddef>
#include <limits>
#include <numeric>
#include <vector>

#include <dash/Array.h>
#include <dash/Types.h>

#include <dash/internal/Logging.h>

namespace detail {

struct UnitInfo {
  std::size_t nunits;
  // prefix sum over the number of local elements of all unit
  std::vector<size_t>                acc_partition_count;
  std::vector<dash::default_index_t> valid_remote_partitions;

  explicit UnitInfo(std::size_t p_nunits)
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
    PartitionBorder<T>& p_borders, std::vector<T>& splitters)
{
  DASH_LOG_TRACE("< psort__calc_boundaries ");
  DASH_ASSERT_EQ(
      p_borders.is_stable.size(),
      splitters.size(),
      "invalid number of partition borders");

  // recalculate partition boundaries
  for (std::size_t idx = 0; idx < splitters.size(); ++idx) {
    DASH_ASSERT(p_borders.lower_bound[idx] <= p_borders.upper_bound[idx]);
    // case A: partition is already stable or skipped
    if (p_borders.is_stable[idx]) {
      continue;
    }
    // case B: we have the last iteration
    //-> test upper bound directly
    if (p_borders.is_last_iter[idx]) {
      splitters[idx]           = p_borders.upper_bound[idx];
      p_borders.is_stable[idx] = true;
    }
    else {
      // case C: ordinary iteration

      splitters[idx] =
          p_borders.lower_bound[idx] +
          ((p_borders.upper_bound[idx] - p_borders.lower_bound[idx]) / 2);

      if (splitters[idx] == p_borders.lower_bound[idx]) {
        // if we cannot move the partition to the left
        //-> last iteration
        p_borders.is_last_iter[idx] = true;
      }
    }
  }
  DASH_LOG_TRACE("psort__calc_boundaries >");
}

template <typename Iter, typename MappedType, typename SortableHash>
inline const std::vector<std::size_t> psort__local_histogram(
    std::vector<MappedType> const&     splitters,
    std::vector<size_t> const&         valid_partitions,
    PartitionBorder<MappedType> const& p_borders,
    Iter                               data_lbegin,
    Iter                               data_lend,
    SortableHash                       sortable_hash)
{
  DASH_LOG_TRACE("< psort__local_histogram");

  auto const nborders = splitters.size();
  // The first element is 0 and the last element is the total number of local
  // elements in this unit
  auto const sz = splitters.size() + 1;
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
          splitters[idx],
          [&sortable_hash](reference a, const MappedType& b) {
            return sortable_hash(a) < b;
          });
      // search upper bound by starting from the lower bound
      auto ub_it = std::upper_bound(
          lb_it,
          data_lend,
          splitters[idx],
          [&sortable_hash](const MappedType& b, reference a) {
            return b < sortable_hash(a);
          });

      auto const p_left = p_borders.left_partition[idx];
      DASH_ASSERT_NE(p_left, dash::team_unit_t{}, "invalid bounding unit");

      auto const nlt_idx = (p_left)*NLT_NLE_BLOCK;

      l_nlt_nle[nlt_idx]     = std::distance(data_lbegin, lb_it);
      l_nlt_nle[nlt_idx + 1] = std::distance(data_lbegin, ub_it);
    }

    auto const last_valid_border_idx = *std::prev(valid_partitions.cend());
    auto const p_left = p_borders.left_partition[last_valid_border_idx];

    // fill trailing partitions with local capacity
    std::fill(
        std::next(std::begin(l_nlt_nle), (p_left + 1) * NLT_NLE_BLOCK),
        std::end(l_nlt_nle),
        n_l_elem);
  }

  DASH_LOG_TRACE("psort__local_histogram >");
  return l_nlt_nle;
}

template <class InputIt, class OutputIt>
inline void psort__global_histogram(
    InputIt     local_histo_begin,
    InputIt     local_histo_end,
    OutputIt    output_it,
    dart_team_t dart_team_id)
{
  DASH_LOG_TRACE("< psort__global_histogram ");

  auto const nels = std::distance(local_histo_begin, local_histo_end);

  dart_allreduce(
      &(*local_histo_begin),
      &(*output_it),
      nels,
      dash::dart_datatype<size_t>::value,
      DART_OP_SUM,
      dart_team_id);

  DASH_LOG_TRACE("psort__global_histogram >");
}

template <typename ElementType>
inline bool psort__validate_partitions(
    UnitInfo const&                 p_unit_info,
    std::vector<ElementType> const& splitters,
    std::vector<size_t> const&      valid_partitions,
    PartitionBorder<ElementType>&   p_borders,
    std::vector<size_t> const&      global_histo)
{
  DASH_LOG_TRACE("< psort__validate_partitions");

  if (valid_partitions.empty()) {
    return true;
  }

  auto const& acc_partition_count = p_unit_info.acc_partition_count;

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
    auto const p_left  = p_borders.left_partition[border_idx];
    auto const nlt_idx = p_left * NLT_NLE_BLOCK;

    auto const peer_idx = p_left + 1;

    if (global_histo[nlt_idx] < acc_partition_count[peer_idx] &&
        acc_partition_count[peer_idx] <= global_histo[nlt_idx + 1]) {
      p_borders.is_stable[border_idx] = true;
    }
    else {
      if (global_histo[nlt_idx] >= acc_partition_count[peer_idx]) {
        p_borders.upper_bound[border_idx] = splitters[border_idx];
      }
      else {
        p_borders.lower_bound[border_idx] = splitters[border_idx];
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

template <class LocalArrayT>
inline void psort__calc_final_partition_dist(
    std::vector<size_t> const& acc_partition_count,
    LocalArrayT&               l_partition_dist)
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
  for (auto unit = dash::team_unit_t{0}; unit < nunits && my_deficit > 0;
       ++unit) {
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

template <typename ElementType, typename InputIt, typename OutputIt>
inline void psort__calc_send_count(
    PartitionBorder<ElementType> const& p_borders,
    std::vector<size_t> const&          valid_partitions,
    InputIt                             target_count,
    OutputIt                            send_count)
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

template <typename ElementType>
inline void psort__calc_target_displs(
    PartitionBorder<ElementType> const& p_borders,
    std::vector<size_t> const&          valid_partitions,
    dash::Array<size_t>&                g_partition_data)
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
        // TODO(kowalewski): Is this really necessary or can we assume that
        // n_u_elements == u_size, i.e., local_pos.index == 0?
        auto const local_pos = pattern.local(u_gidx_begin);

        n_u_elements = u_size - local_pos.index;

        DASH_ASSERT_EQ(local_pos.unit, unit, "units must match");
      }

      acc_partition_count[unit + 1] =
          n_u_elements + acc_partition_count[unit];
      if (unit != myid) {
        unit_info.valid_remote_partitions.emplace_back(unit);
      }
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

  DASH_LOG_TRACE("psort__init_partition_borders >");
}

template <class Iter, class SortableHash>
inline auto find_global_min_max(
    Iter lbegin, Iter lend, dart_team_t teamid, SortableHash sortable_hash)
    -> std::pair<
        typename std::decay<typename dash::functional::closure_traits<
            SortableHash>::result_type>::type,
        typename std::decay<typename dash::functional::closure_traits<
            SortableHash>::result_type>::type>
{
  using mapped_type =
      typename std::decay<typename dash::functional::closure_traits<
          SortableHash>::result_type>::type;

  auto const n_l_elem = std::distance(lbegin, lend);

  std::array<mapped_type, 2> min_max_in{
      // local minimum
      (n_l_elem > 0) ? sortable_hash(*lbegin)
                     : std::numeric_limits<mapped_type>::max(),
      (n_l_elem > 0) ? sortable_hash(*(std::prev(lend)))
                     : std::numeric_limits<mapped_type>::min()};
  std::array<mapped_type, 2> min_max_out{};

  DASH_ASSERT_RETURNS(
      dart_allreduce(
          &min_max_in,                              // send buffer
          &min_max_out,                             // receive buffer
          2,                                        // buffer size
          dash::dart_datatype<mapped_type>::value,  // data type
          DART_OP_MINMAX,                           // operation
          teamid                                    // team
          ),
      DART_OK);

  return std::make_pair(std::get<0>(min_max_out), std::get<1>(min_max_out));
}

#ifdef DASH_ENABLE_TRACE_LOGGING

template <
    class Iterator,
    typename std::iterator_traits<Iterator>::difference_type Stride>
class StridedIterator {
  using iterator_traits = std::iterator_traits<Iterator>;
  using stride_t = typename std::iterator_traits<Iterator>::difference_type;

public:
  using value_type        = typename iterator_traits::value_type;
  using difference_type   = typename iterator_traits::difference_type;
  using reference         = typename iterator_traits::reference;
  using pointer           = typename iterator_traits::pointer;
  using iterator_category = std::bidirectional_iterator_tag;

  StridedIterator() = default;

  constexpr StridedIterator(Iterator first, Iterator it, Iterator last)
    : m_first(first)
    , m_iter(it)
    , m_last(last)
  {
  }

  StridedIterator(const StridedIterator& other)     = default;
  StridedIterator(StridedIterator&& other) noexcept = default;
  StridedIterator& operator=(StridedIterator const& other) = default;
  StridedIterator& operator=(StridedIterator&& other) noexcept = default;
  ~StridedIterator()                                           = default;

  StridedIterator operator++()
  {
    increment();
    return *this;
  }

  StridedIterator operator--()
  {
    decrement();
    return *this;
  }

  StridedIterator operator++(int) const noexcept
  {
    Iterator tmp = *this;
    tmp.increment();
    return tmp;
  }

  StridedIterator operator--(int) const noexcept
  {
    Iterator tmp = *this;
    tmp.decrement();
    return tmp;
  }

  reference operator*() const noexcept
  {
    return *m_iter;
  }

private:
  void increment()
  {
    for (difference_type i = 0; (m_iter != m_last) && (i < Stride); ++i) {
      ++m_iter;
    }
  }

  void decrement()
  {
    for (difference_type i = 0; (m_iter != m_first) && (i < Stride); ++i) {
      --m_iter;
    }
  }

public:
  friend bool operator==(
      const StridedIterator& lhs, const StridedIterator rhs) noexcept
  {
    return lhs.m_iter == rhs.m_iter;
  }
  friend bool operator!=(
      const StridedIterator& lhs, const StridedIterator rhs) noexcept
  {
    return !(lhs.m_iter == rhs.m_iter);
  }

private:
  Iterator m_first{};
  Iterator m_iter{};
  Iterator m_last{};
};

#endif

inline void trace_local_histo(
    std::string&& ctx, std::vector<size_t> const& histograms)
{
#ifdef DASH_ENABLE_TRACE_LOGGING
  using strided_iterator_t = detail::StridedIterator<
      typename std::vector<size_t>::const_iterator,
      NLT_NLE_BLOCK>;

  strided_iterator_t nlt_first{
      std::begin(histograms), std::begin(histograms), std::end(histograms)};
  strided_iterator_t nlt_last{
      std::begin(histograms), std::end(histograms), std::end(histograms)};

  DASH_LOG_TRACE_RANGE(ctx.c_str(), nlt_first, nlt_last);

  strided_iterator_t nle_first{std::begin(histograms),
                               std::next(std::begin(histograms)),
                               std::end(histograms)};
  strided_iterator_t nle_last{
      std::begin(histograms), std::end(histograms), std::end(histograms)};

  DASH_LOG_TRACE_RANGE(ctx.c_str(), nle_first, nle_last);
#endif
}

}  // namespace detail
#endif
