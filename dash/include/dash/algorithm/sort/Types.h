#ifndef DASH__ALGORITHM__SORT__TYPES_H
#define DASH__ALGORITHM__SORT__TYPES_H

#include <dash/Types.h>
#include <dash/algorithm/sort/ThreadPool.h>
#include <dash/internal/Macro.h>
#include <future>
#include <limits>
#include <map>
#include <vector>

#define IDX_DIST(nunits) ((nunits)*0)
#define IDX_SUPP(nunits) ((nunits)*1)
//idx source disp
#define IDX_DISP(nunits) ((nunits)*2)

//original: send count
#define IDX_SRC_COUNT(nunits) IDX_DIST(nunits)
#define IDX_TARGET_COUNT(nunits) IDX_SUPP(nunits)
#define NLT_NLE_BLOCK (2)

namespace dash {

namespace impl {

// A range of chunks to be merged/copied
using ChunkRange        = std::pair<std::size_t, std::size_t>;
using ChunkDependencies = std::map<ChunkRange, std::future<void>>;

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

template <
    class Iterator,
    typename std::iterator_traits<Iterator>::difference_type Stride>
class StridedIterator {
  using iterator_traits = std::iterator_traits<Iterator>;
  using stride_t = typename std::iterator_traits<Iterator>::difference_type;

  static_assert(
      std::is_same<
          typename std::iterator_traits<Iterator>::iterator_category,
          std::random_access_iterator_tag>::value,
      "only random access iterators are supported for strided iteration");

public:
  using value_type        = typename iterator_traits::value_type;
  using difference_type   = typename iterator_traits::difference_type;
  using reference         = typename iterator_traits::reference;
  using pointer           = typename iterator_traits::pointer;
  using iterator_category = std::random_access_iterator_tag;

  constexpr StridedIterator() = default;

  constexpr StridedIterator(Iterator begin, Iterator it)
    : m_iter(it)
    , m_begin(begin)
  {
  }

  StridedIterator(const StridedIterator& other)     = default;
  StridedIterator(StridedIterator&& other) noexcept = default;
  StridedIterator& operator=(StridedIterator const& other) = default;
  StridedIterator& operator=(StridedIterator&& other) noexcept = default;
  ~StridedIterator()                                           = default;

  constexpr StridedIterator& operator++() noexcept
  {
    increment(1);
    return *this;
  }

  constexpr StridedIterator operator++(int) const noexcept
  {
    StridedIterator tmp = *this;
    tmp.increment(1);
    return tmp;
  }

  constexpr StridedIterator& operator--() noexcept
  {
    decrement(1);
    return *this;
  }

  constexpr StridedIterator operator--(int) const noexcept
  {
    StridedIterator tmp = *this;
    tmp.decrement(1);
    return tmp;
  }

  constexpr StridedIterator& operator+=(const difference_type n) noexcept
  {
    increment(n);
    return *this;
  }

  constexpr StridedIterator operator+(const difference_type n) const noexcept
  {
    StridedIterator tmp = *this;
    tmp.increment(n);
    return tmp;
  }

  constexpr StridedIterator& operator-=(const difference_type n) noexcept
  {
    decrement(n);
    return *this;
  }

  constexpr StridedIterator operator-(const difference_type n) const noexcept
  {
    StridedIterator tmp = *this;
    tmp.decrement(n);
    return tmp;
  }

  constexpr reference operator*() const noexcept
  {
    return *m_iter;
  }

private:
  constexpr void increment(difference_type n)
  {
    std::advance(m_iter, n * Stride);
  }

  constexpr void decrement(difference_type n)
  {
    std::advance(m_iter, -n * Stride);
  }

public:
  template <class It, typename std::iterator_traits<It>::difference_type S>
  friend DASH_CONSTEXPR bool operator==(
      const StridedIterator<It, S>& lhs,
      const StridedIterator<It, S>& rhs) DASH_NOEXCEPT;

  template <class It, typename std::iterator_traits<It>::difference_type S>
  friend DASH_CONSTEXPR bool operator!=(
      const StridedIterator<It, S>& lhs,
      const StridedIterator<It, S>& rhs) DASH_NOEXCEPT;

  template <class It, typename std::iterator_traits<It>::difference_type S>
  friend DASH_CONSTEXPR bool operator<(
      const StridedIterator<It, S>& lhs,
      const StridedIterator<It, S>& rhs) DASH_NOEXCEPT;

  template <class It, typename std::iterator_traits<It>::difference_type S>
  friend DASH_CONSTEXPR bool operator<=(
      const StridedIterator<It, S>& lhs,
      const StridedIterator<It, S>& rhs) DASH_NOEXCEPT;

  template <class It, typename std::iterator_traits<It>::difference_type S>
  friend DASH_CONSTEXPR bool operator>(
      const StridedIterator<It, S>& lhs,
      const StridedIterator<It, S>& rhs) DASH_NOEXCEPT;

  template <class It, typename std::iterator_traits<It>::difference_type S>
  friend DASH_CONSTEXPR bool operator>=(
      const StridedIterator<It, S>& lhs,
      const StridedIterator<It, S>& rhs) DASH_NOEXCEPT;

  template <class It, typename std::iterator_traits<It>::difference_type S>
  friend DASH_CONSTEXPR typename std::iterator_traits<It>::difference_type
  operator-(
      const StridedIterator<It, S>& lhs,
      const StridedIterator<It, S>& rhs) DASH_NOEXCEPT;

private:
  Iterator       m_iter{};
  Iterator const m_begin{};
};

template <
    class Iterator,
    typename std::iterator_traits<Iterator>::difference_type Stride>
DASH_CONSTEXPR bool operator==(
    const StridedIterator<Iterator, Stride>& lhs,
    const StridedIterator<Iterator, Stride>& rhs) DASH_NOEXCEPT
{
  DASH_ASSERT(lhs.m_begin == rhs.m_begin);
  return lhs.m_iter == rhs.m_iter;
}

template <
    class Iterator,
    typename std::iterator_traits<Iterator>::difference_type Stride>
DASH_CONSTEXPR bool operator!=(
    const StridedIterator<Iterator, Stride>& lhs,
    const StridedIterator<Iterator, Stride>& rhs) DASH_NOEXCEPT
{
  DASH_ASSERT(lhs.m_begin == rhs.m_begin);
  return lhs.m_iter != rhs.m_iter;
}

template <
    class Iterator,
    typename std::iterator_traits<Iterator>::difference_type Stride>
DASH_CONSTEXPR bool operator<(
    const StridedIterator<Iterator, Stride>& lhs,
    const StridedIterator<Iterator, Stride>& rhs) DASH_NOEXCEPT
{
  DASH_ASSERT(lhs.m_begin == rhs.m_begin);
  return (lhs.m_iter < rhs.m_iter);
}

template <
    class Iterator,
    typename std::iterator_traits<Iterator>::difference_type Stride>
DASH_CONSTEXPR bool operator<=(
    const StridedIterator<Iterator, Stride>& lhs,
    const StridedIterator<Iterator, Stride>& rhs) DASH_NOEXCEPT
{
  DASH_ASSERT(lhs.m_begin == rhs.m_begin);
  return (lhs.m_iter <= rhs.m_iter);
}

template <
    class Iterator,
    typename std::iterator_traits<Iterator>::difference_type Stride>
DASH_CONSTEXPR bool operator>(
    const StridedIterator<Iterator, Stride>& lhs,
    const StridedIterator<Iterator, Stride>& rhs) DASH_NOEXCEPT
{
  DASH_ASSERT(lhs.m_begin == rhs.m_begin);
  return lhs.m_iter > rhs.m_iter;
}

template <
    class Iterator,
    typename std::iterator_traits<Iterator>::difference_type Stride>
DASH_CONSTEXPR bool operator>=(
    const StridedIterator<Iterator, Stride>& lhs,
    const StridedIterator<Iterator, Stride>& rhs) DASH_NOEXCEPT
{
  DASH_ASSERT(lhs.m_begin == rhs.m_begin);
  return lhs.m_iter >= rhs.m_iter;
}

template <
    class Iterator,
    typename std::iterator_traits<Iterator>::difference_type Stride>
DASH_CONSTEXPR typename std::iterator_traits<Iterator>::difference_type
operator-(
    const StridedIterator<Iterator, Stride>& lhs,
    const StridedIterator<Iterator, Stride>& rhs) DASH_NOEXCEPT
{
  DASH_ASSERT(lhs.m_begin == rhs.m_begin);

  return (lhs.m_iter - rhs.m_iter) / 2;
}

template <class Iter>
constexpr StridedIterator<Iter, 2> make_strided_iterator(Iter begin)
{
  return StridedIterator<Iter, 2>{begin, begin};
}

}  // namespace impl
}  // namespace dash
#endif
