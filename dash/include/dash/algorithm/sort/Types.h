#ifndef DASH__ALGORITHM__SORT__TYPES_H
#define DASH__ALGORITHM__SORT__TYPES_H

#include <dash/Types.h>
#include <limits>
#include <vector>


#define IDX_DIST(nunits) ((nunits)*0)
#define IDX_SUPP(nunits) ((nunits)*1)
#define IDX_TARGET_DISP(nunits) ((nunits)*2)

#define IDX_SEND_COUNT(nunits) IDX_DIST(nunits)
#define IDX_TARGET_COUNT(nunits) IDX_SUPP(nunits)
#define NLT_NLE_BLOCK (2)

namespace dash {

namespace detail {

template <typename T>
struct Splitter {
public:
  // tracks if we have found a stable partition border
  std::vector<bool> is_stable;
  // tracks if a partition is skipped
  std::vector<bool> is_skipped;
  // lower bound of each partition
  std::vector<T> lower_bound;
  // the splitter values
  std::vector<T> threshold;
  // upper bound of each partition
  std::vector<T> upper_bound;
  // Special case for the last iteration in finding partition borders
  std::vector<bool> is_last_iter;

  // The right unit is always right next to the border. For this reason we
  // track  only the left unit.
  std::vector<dash::default_index_t> left_partition;

  constexpr Splitter(size_t nsplitter, T _lower_bound, T _upper_bound)
    : is_stable(nsplitter, false)
    , is_skipped(nsplitter, false)
    , lower_bound(nsplitter, _lower_bound)
    , threshold(nsplitter, T{})
    , upper_bound(nsplitter, _upper_bound)
    , is_last_iter(nsplitter, false)
    , left_partition(
          nsplitter, std::numeric_limits<dash::default_index_t>::min())
  {
  }

  constexpr size_t count() const noexcept
  {
    return threshold.size();
  }
};

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

}  // namespace detail
}  // namespace dash
#endif
