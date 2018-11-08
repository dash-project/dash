#ifndef DASH__MEMORY__UNIQUE_PTR_H__INCLUDED
#define DASH__MEMORY__UNIQUE_PTR_H__INCLUDED

#include <memory>

namespace dash {

template <class Allocator>
class DefaultDeleter;

template <class Allocator>
constexpr bool operator==(
    DefaultDeleter<Allocator> const &,
    DefaultDeleter<Allocator> const &) noexcept;

template <class Allocator>
class DefaultDeleter {
public:
  using memory  = typename Allocator::memory_resource;
  using pointer = typename std::allocator_traits<Allocator>::pointer;

  template <class Allocator_>
  friend constexpr bool operator==(
      DefaultDeleter<Allocator_> const &,
      DefaultDeleter<Allocator_> const &) noexcept;

  constexpr DefaultDeleter() = default;

  DefaultDeleter(const Allocator &alloc, std::size_t nels)
    : m_alloc(alloc)
    , m_nels(nels)
  {
  }

  void operator()(pointer p)
  {
    m_alloc.deallocate(p, m_nels);
  }

private:
  Allocator   m_alloc{nullptr};
  std::size_t m_nels{};
};

template <class Allocator>
constexpr bool operator==(
    DefaultDeleter<Allocator> const &lhs,
    DefaultDeleter<Allocator> const &rhs) noexcept
{
  return lhs.m_alloc == rhs.m_alloc && lhs.m_nels == rhs.m_nels;
}

template <
    /// The value type of the unique pointer
    class T,
    /// The allocator to allocate from
    class AllocatorT,
    /// The deleter to use for destruction
    template <class> class Deleter = dash::DefaultDeleter>
auto allocate_unique(const AllocatorT &alloc, std::size_t n)
{
  using allocator =
      typename std::allocator_traits<AllocatorT>::template rebind_alloc<T>;
  using deleter    = Deleter<allocator>;
  using unique_ptr = std::unique_ptr<T, deleter>;

  allocator _alloc{alloc};
  return unique_ptr{_alloc.allocate(n), deleter{_alloc, n}};
}

}  // namespace dash
#endif
