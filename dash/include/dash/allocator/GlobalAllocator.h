#ifndef DASH__ALLOCATOR__GLOBAL_ALLOCATOR_H__INCLUDED
#define DASH__ALLOCATOR__GLOBAL_ALLOCATOR_H__INCLUDED

#include <dash/memory/MemorySpace.h>
#include <type_traits>

namespace dash {

namespace detail {
template <class T>
struct remove_cvref {
  typedef std::remove_cv_t<std::remove_reference_t<T>> type;
};
}  // namespace detail

template <class T, class GlobMemType>
class GlobalAllocator {
public:
  /**
   * Rebind to a different allocator type with same Memory Space
   */
  template <class U>
  using rebind = dash::GlobalAllocator<T, GlobMemType>;

  // value type
  using value_type = typename detail::remove_cvref<T>::type;

  // Obtain correct pointer type for T
  using pointer = typename std::pointer_traits<
      typename GlobMemType::void_pointer>::template rebind<value_type>;

  using memory_resource = GlobMemType;

public:
  constexpr GlobalAllocator() noexcept = default;
  constexpr explicit GlobalAllocator(
      memory_resource* memory_resource) noexcept
    : m_resource(memory_resource)
  {
  }

  template <class U>
  GlobalAllocator(const GlobalAllocator<U, GlobMemType>& other) noexcept
    : m_resource(other.resource())
  {
  }

  pointer allocate(std::size_t n)
  {
    using memory_traits          = dash::memory_space_traits<GlobMemType>;
    constexpr bool is_contiguous = std::is_same<
        typename memory_traits::memory_space_layout_tag,
        memory_space_contiguous>::value;
    pointer p = m_resource ? static_cast<pointer>(m_resource->allocate(
                                 n * sizeof(T), alignof(T)))
                           : pointer{nullptr};
    if (is_contiguous && p) {
      auto& reg = internal::MemorySpaceRegistry::GetInstance();
      reg.add(static_cast<dart_gptr_t>(p), m_resource);
    }
    return p;
  }

  void deallocate(pointer p, std::size_t n)
  {
    using memory_traits          = dash::memory_space_traits<GlobMemType>;
    constexpr bool is_contiguous = std::is_same<
        typename memory_traits::memory_space_layout_tag,
        memory_space_contiguous>::value;
    if (m_resource && p) {
      m_resource->deallocate(p, n * sizeof(T), alignof(T));
      if (is_contiguous) {
        auto& reg = internal::MemorySpaceRegistry::GetInstance();
        reg.erase(static_cast<dart_gptr_t>(p));
      }
    }
  }

  memory_resource* resource() const noexcept
  {
    return m_resource;
  }

private:
  memory_resource* m_resource{nullptr};
};

template <class T, class GlobMemType>
constexpr bool operator==(
    GlobalAllocator<T, GlobMemType> lhs,
    GlobalAllocator<T, GlobMemType> rhs) noexcept
{
  // We pass GlobalAllocator by copy, not reference because it is just a
  // pointer.
  // We further define equality in terms of memory resource equality. That is,
  // we have to compare really the pointers instead of the memory resource
  // objects.
  // TODO: Think again if this is the correct decision...
  return lhs.resource() == rhs.resource();
}
}  // namespace dash

#endif
