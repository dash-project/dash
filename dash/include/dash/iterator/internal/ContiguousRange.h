#ifndef DASH__ITERATOR_INTERNAL_CONTIGUOUSRANGE_H__
#define DASH__ITERATOR_INTERNAL_CONTIGUOUSRANGE_H__

#include <dash/GlobPtr.h>
#include <dash/internal/Macro.h>
#include <dash/Exception.h>

namespace dash {
namespace internal {

/**
 * Iterator used to find consecutive memory ranges across a global memory range.
 */
template<typename IteratorT, bool HasView = IteratorT::has_view::value>
struct ContiguousRangeIterator {

public:
  /// Iterator Traits
  using iterator_category = std::forward_iterator_tag;
  using pattern_type = typename IteratorT::pattern_type;
  using index_type   = typename pattern_type::index_type;
  using size_type    = typename pattern_type::size_type;
  using value_type   = std::pair<IteratorT, size_type>;

  using Self_t = ContiguousRangeIterator<IteratorT, HasView>;

  DASH_CONSTEXPR ContiguousRangeIterator() = default;

  ContiguousRangeIterator(IteratorT begin, IteratorT end)
  : m_pos(begin),
    m_end(end),
    m_num_copy_elems(next_range().second)
  { }

  Self_t&
  operator++() {
    auto range = next_range();
    m_pos            = range.first;
    m_num_copy_elems = range.second;
    return *this;
  }

  std::pair<IteratorT, size_type>
  operator*() {
    return std::make_pair(m_pos, m_num_copy_elems);
  }

  DASH_CONSTEXPR bool operator<(const Self_t& other) const DASH_NOEXCEPT
  {
    return (m_pos < other.m_pos);
  }

  DASH_CONSTEXPR bool operator<=(const Self_t& other) const DASH_NOEXCEPT
  {
    return (m_pos <= other.m_pos);
  }

  DASH_CONSTEXPR bool operator>(const Self_t& other) const DASH_NOEXCEPT
  {
    return (m_pos > other.m_pos);
  }

  DASH_CONSTEXPR bool operator>=(const Self_t& other) const DASH_NOEXCEPT
  {
    return (m_pos >= other.m_pos);
  }

  DASH_CONSTEXPR bool operator==(const Self_t& other) const DASH_NOEXCEPT
  {
    return m_pos == other.m_pos;
  }

  DASH_CONSTEXPR bool operator!=(const Self_t& other) const DASH_NOEXCEPT
  {
    return m_pos != other.m_pos;
  }


private:

  std::pair<IteratorT, size_type>
  next_range() const {
    auto cur_first = m_pos + m_num_copy_elems;
    auto cur_last  = cur_first;
    size_type num_copy_elem = 0;

    const auto& pattern = cur_last.pattern();

    // unit and local index of first element in current range segment:
    auto local_pos      = pattern.local(static_cast<index_type>(
                                            m_pos.pos()));
    auto last_local_pos = local_pos;
    do {
      ++cur_last;
      ++num_copy_elem;
      auto pos = pattern.local(static_cast<index_type>(
                                            cur_last.pos()));
      if (m_end == cur_last ||
          pos.unit  != local_pos.unit ||
          pos.index != (last_local_pos.index + 1)) {
        break;
      }
      last_local_pos = pos;
    } while (1);

    return std::make_pair(cur_last, num_copy_elem);
  }


private:

  /// Start of the current contiguous range
  IteratorT m_pos;
  /// End position of the total range
  const IteratorT m_end;
  /// Number of elements in current contiguous range
  size_type m_num_copy_elems = 0;
};


/**
 * Specialization for non-view iterators.
 */
template<typename IteratorT>
struct ContiguousRangeIterator<IteratorT, false> {

public:
  /// Iterator Traits
  using iterator_category = std::forward_iterator_tag;
  using pattern_type = typename IteratorT::pattern_type;
  using index_type   = typename pattern_type::index_type;
  using size_type    = typename pattern_type::size_type;
  using value_type   = std::pair<IteratorT, size_type>;

  using Self_t = ContiguousRangeIterator<IteratorT, false>;

  DASH_CONSTEXPR ContiguousRangeIterator() = default;

  ContiguousRangeIterator(IteratorT begin, IteratorT end)
  : m_pos(begin),
    m_end(end),
    m_num_copy_elems(next_range().second)
  { }

  Self_t&
  operator++() {
    auto range = next_range();
    m_pos            = range.first;
    m_num_copy_elems = range.second;
    return *this;
  }

  std::pair<IteratorT, size_type>
  operator*() noexcept {
    return std::make_pair(m_pos, m_num_copy_elems);
  }

  DASH_CONSTEXPR bool operator<(const Self_t& other) const DASH_NOEXCEPT
  {
    return (m_pos < other.m_pos);
  }

  DASH_CONSTEXPR bool operator<=(const Self_t& other) const DASH_NOEXCEPT
  {
    return (m_pos <= other.m_pos);
  }

  DASH_CONSTEXPR bool operator>(const Self_t& other) const DASH_NOEXCEPT
  {
    return (m_pos > other.m_pos);
  }

  DASH_CONSTEXPR bool operator>=(const Self_t& other) const DASH_NOEXCEPT
  {
    return (m_pos >= other.m_pos);
  }

  DASH_CONSTEXPR bool operator==(const Self_t& other) const DASH_NOEXCEPT
  {
    return m_pos == other.m_pos;
  }

  DASH_CONSTEXPR bool operator!=(const Self_t& other) const DASH_NOEXCEPT
  {
    return m_pos != other.m_pos;
  }

private:

  std::pair<IteratorT, size_type>
  next_range() const {
    auto cur_first = m_pos + m_num_copy_elems;
    auto cur_last  = cur_first;
    size_type num_copy_elem = 0;
    constexpr const int ndim = pattern_type::ndim();
    const auto& pattern = m_pos.pattern();
    const int fast_dim = (pattern.memory_order() == dash::ROW_MAJOR) ? ndim - 1 : 0;
    const size_type blocksize_d = pattern.blocksize(fast_dim);


    auto lpos = m_pos.lpos();

    do {

      size_type blocksize = blocksize_d;

      /* Determine coords and offset in first block */
      auto global_coords = pattern.coords(cur_last.gpos());

      // check in which block we currently are
      auto block_coord_d   = global_coords[fast_dim] / blocksize;
      auto phase_d         = global_coords[fast_dim] % blocksize;

      // check for underful blocks at the end of the dimension
      if (block_coord_d == (pattern.blockspec().extent(fast_dim) - 1)) {
        blocksize = pattern.extent(fast_dim) - (block_coord_d * blocksize_d);
      }

      // the number of elements to copy is the blocksize minus the offset in the block
      size_type num_copy_block_elem = blocksize - phase_d;

      // don't try to copy too many elements
      size_type elems_left = dash::distance(cur_last, m_end);
      if (num_copy_block_elem > elems_left) {
        num_copy_block_elem = elems_left;
        // nothing more to do here
        num_copy_elem += num_copy_block_elem;
        break;
      }

      cur_last      += num_copy_block_elem;
      auto next_lpos = cur_last.lpos();

      num_copy_elem += num_copy_block_elem;
      // check whether the contiguous range is over at the end of the block
      if (cur_last == m_end ||
          next_lpos.unit != lpos.unit ||
          next_lpos.index != (lpos.index + 1)) {
        break;
      }

    } while (1);

    return std::make_pair(cur_first, num_copy_elem);
  }


private:

  /// Start of the current contiguous range
  IteratorT m_pos;
  /// End position of the total range
  const IteratorT m_end;
  /// Number of elements in current contiguous range
  size_type m_num_copy_elems = 0;
};


/**
 * Specialization for non-view iterators.
 */
template<typename ValueType, typename GlobMemT>
struct ContiguousRangeIterator<dash::GlobPtr<ValueType, GlobMemT>, false> {

public:
  /// Iterator Traits
  using iterator_category = std::forward_iterator_tag;
  using pointer_type = typename dash::GlobPtr<ValueType, GlobMemT>;
  using index_type   = typename pointer_type::index_type;
  using size_type    = typename pointer_type::size_type;
  using value_type   = std::pair<pointer_type, size_type>;

  using Self_t = ContiguousRangeIterator<pointer_type, false>;

  DASH_CONSTEXPR ContiguousRangeIterator() = default;

  ContiguousRangeIterator(pointer_type begin, pointer_type end)
  : m_pos(begin),
    m_end(end),
    m_num_copy_elems(next_range().second)
  { }

  Self_t&
  operator++() {
    auto range = next_range();
    m_pos            = range.first;
    m_num_copy_elems = range.second;
    return *this;
  }

  std::pair<pointer_type, size_type>
  operator*() noexcept {
    return std::make_pair(m_pos, m_num_copy_elems);
  }

  DASH_CONSTEXPR bool operator<(const Self_t& other) const DASH_NOEXCEPT
  {
    return (m_pos < other.m_pos);
  }

  DASH_CONSTEXPR bool operator<=(const Self_t& other) const DASH_NOEXCEPT
  {
    return (m_pos <= other.m_pos);
  }

  DASH_CONSTEXPR bool operator>(const Self_t& other) const DASH_NOEXCEPT
  {
    return (m_pos > other.m_pos);
  }

  DASH_CONSTEXPR bool operator>=(const Self_t& other) const DASH_NOEXCEPT
  {
    return (m_pos >= other.m_pos);
  }

  DASH_CONSTEXPR bool operator==(const Self_t& other) const DASH_NOEXCEPT
  {
    return m_pos == other.m_pos;
  }

  DASH_CONSTEXPR bool operator!=(const Self_t& other) const DASH_NOEXCEPT
  {
    return m_pos != other.m_pos;
  }

private:

  std::pair<pointer_type, size_type>
  next_range() const {
    auto cur_first = m_pos + m_num_copy_elems;
    auto cur_last  = cur_first;
    size_type num_copy_elem = 0;

    dart_gptr_t gptr = m_pos.dart_gptr();


    auto& reg = dash::internal::MemorySpaceRegistry::GetInstance();
    auto const * mem_space = static_cast<const GlobMemT *>(reg.lookup(gptr));

    if (mem_space == nullptr) {
      // TODO: how to handle this?
    }

    // treat offsets and sizes in Bytes, convert as late as possible
    size_type offs = gptr.addr_or_offs.offset;
    dash::team_unit_t current_uid{gptr.unitid};
    size_type size_at_unit = mem_space->capacity(current_uid);
    DASH_ASSERT_LT(offs*sizeof(value_type), size_at_unit,
                   "Global pointer points beyond local unit!");

    size_type size_left_at_unit = size_at_unit - offs;
    size_type elems_left = dash::distance(m_pos, m_end);


    // check if there is enough space at the current unit
    if (elems_left*sizeof(value_type) <= size_left_at_unit) {
      num_copy_elem = elems_left;
    } else {
      // go to end of current unit
      num_copy_elem = size_left_at_unit / sizeof(value_type);
    }
    return std::make_pair(cur_first, num_copy_elem);
  }


private:

  /// Start of the current contiguous range
  pointer_type m_pos;
  /// End position of the total range
  const pointer_type m_end;
  /// Number of elements in current contiguous range
  size_type m_num_copy_elems = 0;
};


template<typename IteratorT>
struct ContiguousRangeSet {

  using iterator_type = ContiguousRangeIterator<IteratorT>;

  ContiguousRangeSet(IteratorT begin, IteratorT end)
  : m_range_begin(begin), m_range_end(end), m_end(end, end)
  { }

  iterator_type begin() const {
    return iterator_type(m_range_begin, m_range_end);
  }

  iterator_type end() const {
    return m_end;
  }

private:
  const IteratorT m_range_begin;
  const IteratorT m_range_end;

  const iterator_type m_end;
};

} // namespace internal
} // namespace dash

#endif // DASH__ITERATOR_INTERNAL_CONTIGUOUSRANGE_H__
