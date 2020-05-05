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
template<typename IteratorT>
struct ContiguousRangeIterator {

public:
  /// Iterator Traits
  using iterator_category = std::forward_iterator_tag;
  using pattern_type = typename IteratorT::pattern_type;
  using index_type   = typename pattern_type::index_type;
  using size_type    = typename pattern_type::size_type;
  using value_type   = std::pair<IteratorT, size_type>;

  using Self_t = ContiguousRangeIterator<IteratorT>;

  DASH_CONSTEXPR ContiguousRangeIterator() = default;

  ContiguousRangeIterator(IteratorT begin, IteratorT end)
  : m_pos(begin),
    m_end(end)
  {
    m_num_copy_elems = next_range().second;
  }

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


  template<typename IterT>
  size_type check_iter(const IterT& t, size_type num_elems, dim_t fast_dim) const {

    return num_elems;
  }

  template<typename ElementT, typename PatternT, typename GlobMemT, typename PointerT, typename ReferenceT>
  size_type check_iter(const GlobViewIter<ElementT, PatternT, GlobMemT, PointerT, ReferenceT>& it,
                       size_type num_elems, dim_t fast_dim) const {
    auto view_it_extent = it.viewspec().extent(fast_dim);
    if(num_elems > view_it_extent) {
      num_elems = view_it_extent;
    }

    return num_elems;
  }

  std::pair<IteratorT, size_type>
  next_range() const {

    auto cur_first = m_pos + m_num_copy_elems;

    if (cur_first == m_end) {
      return std::make_pair(m_end, 0);
    }

    auto cur_last  = cur_first;
    size_type num_copy_elem = 0;
    constexpr const dim_t ndim = pattern_type::ndim();
    const auto& pattern = m_pos.pattern();
    const dim_t fast_dim = (pattern.memory_order() == dash::ROW_MAJOR) ? ndim - 1 : 0;


    auto lpos = m_pos.lpos();
    auto prev_lpos = lpos;

    do {

      /* Determine coords and offset in first block */
      auto global_coords = pattern.coords(cur_last.gpos());

      auto block_idx = pattern.block_at(global_coords);

      auto block_viewspec = pattern.block(block_idx);

      auto phase_d = global_coords[fast_dim] - block_viewspec.offset(fast_dim);

      auto blocksize_d = block_viewspec.extent(fast_dim);

      // the number of elements to copy is the blocksize minus the offset in the block
      size_type num_copy_block_elem = blocksize_d - phase_d;
      num_copy_block_elem = check_iter(cur_last, num_copy_block_elem, fast_dim);

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
          next_lpos.index != (prev_lpos.index + 1)) {
        break;
      }

      prev_lpos = next_lpos;

    } while (1);
    DASH_LOG_TRACE("next_range<GlobIter>", "cur_first", cur_first,
                   "num_copy_elem", num_copy_elem);
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
 * Specialization for GlobPtr.
 */
template<typename ValueType, typename GlobMemT>
struct ContiguousRangeIterator<dash::GlobPtr<ValueType, GlobMemT>> {

public:
  /// Iterator Traits
  using iterator_category = std::forward_iterator_tag;
  using pointer_type = typename dash::GlobPtr<ValueType, GlobMemT>;
  using index_type   = typename pointer_type::index_type;
  using size_type    = typename pointer_type::size_type;
  using value_type   = std::pair<pointer_type, size_type>;

  using Self_t = ContiguousRangeIterator<pointer_type>;

  DASH_CONSTEXPR ContiguousRangeIterator() = default;

  ContiguousRangeIterator(pointer_type begin, pointer_type end)
  : m_pos(begin),
    m_end(end)
  {
    m_num_copy_elems = next_range().second;
  }

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
    using element_type = typename pointer_type::value_type;
    auto cur_first = m_pos + m_num_copy_elems;

    if (cur_first == m_end) {
      return std::make_pair(m_end, 0);
    }

    auto cur_last  = cur_first;
    size_type num_copy_elem = 0;

    dart_gptr_t gptr = cur_first.dart_gptr();


    auto& reg = dash::internal::MemorySpaceRegistry::GetInstance();
    auto const * mem_space = static_cast<const GlobMemT *>(reg.lookup(gptr));

    if (mem_space == nullptr) {
      // TODO: how to handle this?
    }

    // treat offsets and sizes in Bytes, convert as late as possible
    size_type offs = gptr.addr_or_offs.offset;
    dash::team_unit_t current_uid{gptr.unitid};
    size_type size_at_unit = mem_space->capacity(current_uid);
    DASH_ASSERT_LT(offs*sizeof(element_type), size_at_unit,
                   "Global pointer points beyond local unit!");

    size_type size_left_at_unit = (size_at_unit - offs) / sizeof(element_type);

    if (size_left_at_unit == 0) {
      DASH_LOG_TRACE("next_range<GlobPtr>",
                     "No space left at unit ", current_uid.id,
                     ", size_at_unit ", size_at_unit,
                     ", offs ", offs);
    }

    size_type elems_left = dash::distance(cur_first, m_end);

    DASH_LOG_TRACE("next_range<GlobPtr>", "size_left_at_unit ", size_left_at_unit,
                   ", elems_left ", elems_left);

    // check if there is enough space at the current unit
    if (elems_left <= size_left_at_unit) {
      num_copy_elem = elems_left;
    } else {
      // go to end of current unit
      num_copy_elem = size_left_at_unit;
    }
    DASH_LOG_TRACE("next_range<GlobPtr>", "cur_first ", cur_first,
                   "num_copy_elem ", num_copy_elem);
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
