#ifndef DASH__MEMORY__MEMORY_SPACE_H__INCLUDED
#define DASH__MEMORY__MEMORY_SPACE_H__INCLUDED

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <memory>

#include <cpp17/polymorphic_allocator.h>
#include <dash/Meta.h>
#include <dash/Types.h>

namespace dash {

template <typename ElementType, class MemorySpace>
class GlobPtr;

/**
 * The \c MemorySpace concept follows the STL \c std::pmr::memory_resource.
 *
 * Unlinke shared memory systems where process memory and virtual address
 * translation is realized as system functions and hardware components,
 * PGAS memory resources are also responsible for maintaining and propagating
 * the size and structure of their underlying local memory ranges.
 *
 * Consequently, the memory resource concept is extended by methods and
 * type definitions that are required to maintain global pointer arithmetics
 * and named Memory Space in DASH.
 *
 * STL Reference of \c std::pmr::memory_resource:
 * http://en.cppreference.com/w/cpp/memory/memory_resource
 *
 * \note
 *
 * Defining \c MemorySpace as class template seems to contradict the
 * intention to use it as polymorphic base class as subclass types of
 * \c MemorySpace<T> and \c MemorySpace<U> have incompatible polymorphic
 * base classes.
 * This looks like the kind of unintentional type incompatibility that lead
 * to the introduction of polymorphic allocators.
 * However, the template parameter specifies the memory space domain and
 * two types of \c MemorySpace are in fact incompatible if they do not refer
 * to the same domain, like global and local address space.
 *
 * \todo
 *
 * Clarify:
 * The STL memory_resource concept assumes that any type can be
 * allocated with its size specified in number of bytes so untyped
 * allocation with void pointers is practicable.
 * However, the DART allocation routines are typed for correctness
 * considerations and to optimize communication.
 * If the memory space concept complies to the STL memory_resource
 * interface, only DART_TYPE_BYTE instead of the actual value type
 * is available. This would therefore harm stability and performance.
 */

struct memory_space_global_domain_tag {
};
struct memory_space_local_domain_tag {
};

struct memory_space_host_tag {
};
struct memory_space_hbw_tag {
};
struct memory_space_pmem_tag {
};

///////////////////////////////////////////////////////////////////////////////
// MEMORY SPACE TRAITS
///////////////////////////////////////////////////////////////////////////////
//
class HostSpace;
class HBWSpace;

namespace details {

template <typename _D>
using is_local_memory_space = std::integral_constant<
    bool,
    std::is_same<_D, memory_space_local_domain_tag>::value>;

template <typename _D>
using is_global_memory_space = std::integral_constant<
    bool,
    std::is_same<_D, memory_space_global_domain_tag>::value>;

template <class _Ms>
using memspace_traits_is_global =
    is_global_memory_space<typename _Ms::memory_space_domain_category>;

template <class _Ms>
using memspace_traits_is_local =
    is_local_memory_space<typename _Ms::memory_space_domain_category>;

DASH__META__DEFINE_TRAIT__HAS_TYPE(void_pointer);
DASH__META__DEFINE_TRAIT__HAS_TYPE(const_void_pointer);

template <class _Ms, bool = has_type_void_pointer<_Ms>::value>
struct memspace_traits_void_pointer_type {
  typedef typename _Ms::void_pointer type;
};

template <class _Ms>
struct memspace_traits_void_pointer_type<_Ms, false> {
  typedef typename std::conditional<
      memspace_traits_is_global<_Ms>::value,
      dash::GlobPtr<void, _Ms>,
      void*>::type type;
};

template <class _Ms, bool = has_type_const_void_pointer<_Ms>::value>
struct memspace_traits_const_void_pointer_type {
  typedef typename _Ms::const_void_pointer type;
};

template <class _Ms>
struct memspace_traits_const_void_pointer_type<_Ms, false> {
  typedef typename std::conditional<
      memspace_traits_is_global<_Ms>::value,
      dash::GlobPtr<const void, _Ms>,
      const void*>::type type;
};

}  // namespace details

template <class MemSpace>
struct memory_space_traits {
  /**
   * The underlying memory type (Host, CUDA, HBW, etc.)
   */
  using memory_space_type_category =
      typename MemSpace::memory_space_type_category;

  /**
   * The underlying memory domain (local, global, etc.)
   */
  using memory_space_domain_category =
      typename MemSpace::memory_space_domain_category;

  /**
   * Whether the memory space type is specified for global address space.
   */
  using is_global = typename details::memspace_traits_is_global<MemSpace>;

  /**
   * Whether the memory space type is specified for local address space.
   * As arbitrary address space domains can be defined, this trait is not
   * equivalent to `!is_global`.
   */
  using is_local = typename details::memspace_traits_is_local<MemSpace>;

  using void_pointer =
      typename details::memspace_traits_void_pointer_type<MemSpace>::type;

  using const_void_pointer =
      typename details::memspace_traits_const_void_pointer_type<
          MemSpace>::type;
};

///////////////////////////////////////////////////////////////////////////////
// MEMORY SPACE CONCEPT
///////////////////////////////////////////////////////////////////////////////

namespace details {

template <class>
struct empty_base {
  empty_base() = default;

  template <class... Args>
  constexpr empty_base(Args&&...)
  {
  }
};

template <bool Condition, class T>
using conditional_base =
    typename std::conditional<Condition, T, empty_base<T>>::type;

}  // namespace details

template <class ElementType>
class LocalMemorySpace : public cpp17::pmr::memory_resource {
  // We may add something here in figure
};

template <typename T>
class GlobalMemorySpace {
private:
  static constexpr size_t max_align = alignof(dash::max_align_t);

  using self_t = GlobalMemorySpace;

  using pointer = dash::GlobPtr<T, self_t>;
  using const_pointer = dash::GlobPtr<const T, self_t>;

public:
  virtual ~GlobalMemorySpace();

  pointer allocate(size_t bytes, size_t alignment = max_align)
  {
    return do_allocate(bytes, alignment);
  }
  void deallocate(pointer p, size_t bytes, size_t alignment = max_align)
  {
    return do_deallocate(p, bytes, alignment);
  }

  /**
   * \note
   * The most-derived type of other may not match the most derived
   * type of *this.
   * A derived class implementation therefore must typically check
   * whether the most derived types of *this and other match using
   * dynamic_cast, and immediately return false if the cast fails.
   *
   * STL Reference:
   * http://cppreference.com/w/cpp/experimental/memory_resource/do_is_equal
   */
  bool is_equal(const GlobalMemorySpace& other) const noexcept
  {
    return do_is_equal(other);
  }

#if 0
  size_t size() const noexcept;

  size_t capacity() const noexcept;
#endif

protected:
  virtual pointer do_allocate(size_t bytes, size_t alignment) = 0;

  virtual void do_deallocate(
      pointer p, size_t bytes, size_t alignment) = 0;

  virtual bool do_is_equal(const GlobalMemorySpace& other) const noexcept = 0;
};

template <typename ElementType, typename MSpaceDomainCategory>
class MemorySpace
  : public details::conditional_base<
        details::is_local_memory_space<MSpaceDomainCategory>::value,
        LocalMemorySpace<ElementType>>,
    public details::conditional_base<
        details::is_global_memory_space<MSpaceDomainCategory>::value,
        GlobalMemorySpace<ElementType>> {
  // A memory space can be either local or global
  // We may change this in future...
  static_assert(
      details::is_local_memory_space<MSpaceDomainCategory>::value ^
          details::is_global_memory_space<MSpaceDomainCategory>::value,
      "memory space must be either local or global");

public:
  using memory_space_domain_category = memory_space_local_domain_tag;
};

template <typename ElementType, typename MSpaceDomainCategory>
inline bool operator==(
    MemorySpace<ElementType, MSpaceDomainCategory> const& lhs,
    MemorySpace<ElementType, MSpaceDomainCategory> const& rhs)
{
  return &lhs == &rhs || lhs.is_equal(rhs);
}

template <typename ElementType, typename MSpaceDomainCategory>
inline bool operator!=(
    MemorySpace<ElementType, MSpaceDomainCategory> const& lhs,
    MemorySpace<ElementType, MSpaceDomainCategory> const& rhs)
{
  return !(lhs == rhs);
}

///////////////////////////////////////////////////////////////////////////////
// HELPER FUNCTIONS
///////////////////////////////////////////////////////////////////////////////
//

//TODO: this is very ugly and we should prefer to create a factory

template <
    class ElementType,
    class MSpaceDomainCategory,
    class MSpaceTypeCategory>
inline MemorySpace<ElementType, MSpaceDomainCategory>*
get_default_memory_space()
{
  // Current we have only a default host space
  return nullptr;
}

MemorySpace<void, memory_space_local_domain_tag>* get_default_host_space();

MemorySpace<void, memory_space_local_domain_tag>* get_default_hbw_space();

template <>
inline MemorySpace<void, memory_space_local_domain_tag>*
get_default_memory_space<
    void,
    memory_space_local_domain_tag,
    memory_space_host_tag>()
{
  return get_default_host_space();
}

template <>
inline MemorySpace<void, memory_space_local_domain_tag>*
get_default_memory_space<
    void,
    memory_space_local_domain_tag,
    memory_space_hbw_tag>()
{
  return get_default_hbw_space();
}

}  // namespace dash

#include <dash/memory/HostSpace.h>
#include <dash/memory/HBWSpace.h>

#endif  // DASH__MEMORY__MEMORY_SPACE_H__INCLUDED
