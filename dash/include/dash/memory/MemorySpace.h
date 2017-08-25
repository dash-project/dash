#ifndef DASH__MEMORY__MEMORY_SPACE_H__INCLUDED
#define DASH__MEMORY__MEMORY_SPACE_H__INCLUDED

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <memory>

#include <dash/Types.h>

namespace dash {

template <typename ElementType, class MemorySpace>
class GlobPtr;

/**
 * Pointer types depend on a memory space.
 * For example, an allocator could be used for global and native memory.
 * The concrete MemorySpace types define pointer types for their
 * their address space, like `GlobPtr<T>` or `T*`.
 *
 * Note that these are provided as incomplete types via member alias
 * templates. Memory spaces are not concerned with value semantics, they
 * only respect the address concept.
 * Value types `T` are specified by allocators.
 *
 */
template <class Pointer>
struct pointer_traits : public std::pointer_traits<Pointer> {
  /*
   * Something similar to allocator::rebind unless a more elegant
   * solution comes up:
   *
   * template <class U>
   * pointer_type = memory_space_traits<
   *                  typename memory_space::template pointer_type<U> >::type
   *
   * Example:
   *
   *    template <class T>
   *    class GlobMem {
   *    public:
   *      template <class U>
   *      struct pointer_type {
   *        typedef dash::GlobPtr<U> type;
   *      };
   *      // ...
   *    }
   *
   * Usage, for example in Allocator type:
   *
   *    template <class T, class MemorySpace>
   *    class MyAllocator {
   *    public:
   *      // would resolve to T* or GlobPtr<T> etc.:
   *      typedef typename
   *        dash::memory_space_traits<MemorySpace>::pointer_type<T>::type
   *      pointer;
   *
   *      typedef typename
   *        dash::pointer_traits<pointer>::const_pointer<T>::type
   *      const_pointer;
   *
   *      // ...
   *    };
   *
   */
};

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

template <class MemSpace>
struct memory_space_traits {
  using memory_space_type_category =
      typename MemSpace::memory_space_type_category;

  using is_global = std::integral_constant<bool, 0>;
#if 0
  using memory_space_domain_category =
      typename MemSpace::memory_space_domain_category;
  /**
   * Whether the memory space type is specified for global address space.
   */
  using is_global = std::integral_constant<
      bool, std::is_same<memory_space_domain_category,
                         memory_space_global_domain_tag>::value>;
  /**
   * Whether the memory space type is specified for local address space.
   * As arbitrary address space domains can be defined, this trait is not
   * equivalent to `!is_global`.
   */
  using is_local = std::integral_constant<
      bool, std::is_same<memory_space_domain_category,
                         memory_space_local_domain_tag>::value>;
#endif

  using void_pointer =
      typename std::conditional<is_global::value,
                                dash::GlobPtr<void, MemSpace>, void*>::type;
};

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
template <class MemSpaceTypeCategory>
class MemorySpace {
  using self_t = MemorySpace;

 public:
  // using index_type = dash::default_index_t;
  // using size_type  = dash::default_size_t;
  using index_type = std::size_t;
  using size_type  = std::size_t;

  using memory_space_type_category = MemSpaceTypeCategory;

  // Resolve void pointer type for this MemorySpace, typically
  // `GlobPtr<void>` for global and `void *` for local memory.
  // Allocators use rebind to obtain a fully specified type like
  // `GlobPtr<double>` and can then cast the `void_pointer`
  // returned from a memory space to their value type.
  using void_pointer =
      typename dash::memory_space_traits<self_t>::void_pointer;

 protected:
  virtual ~MemorySpace();

  virtual void_pointer do_allocate(std::size_t bytes,
                                   std::size_t alignment) = 0;

  virtual void do_deallocate(void_pointer p, std::size_t bytes,
                             std::size_t alignment) = 0;

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
  // virtual bool         do_is_equal(
  //                      const MemorySpace & other) const noexcept = 0;

 public:
  void_pointer allocate(std::size_t bytes,
                        std::size_t alignment = alignof(dash::max_align_t));

  void deallocate(void_pointer p, std::size_t bytes,
                  std::size_t alignment = alignof(dash::max_align_t));

  /**
   * Two memory spaces compare equal if and only if memory allocated
   * from one memory_resource can be deallocated from the other and
   * vice versa.
   */
  bool is_equal(const MemorySpace& other) const noexcept;

  size_type size() const;

  /*
   * \todo
   *
   * Clarify: This method makes only sense in case of a global adresse space.
   */
  // size_type local_size(dash::team_unit_t unit) const;
};

template <class MemSpaceTypeCategory>
inline MemorySpace<MemSpaceTypeCategory>::~MemorySpace()
{
}

template <class MemSpaceTypeCategory>
inline typename MemorySpace<MemSpaceTypeCategory>::void_pointer
MemorySpace<MemSpaceTypeCategory>::allocate(std::size_t bytes,
                                            std::size_t alignment)
{
  return do_allocate(bytes, alignment);
}

template <class MemSpaceTypeCategory>
inline void MemorySpace<MemSpaceTypeCategory>::deallocate(
    typename MemorySpace<MemSpaceTypeCategory>::void_pointer p,
    std::size_t bytes, std::size_t alignment)
{
  return do_deallocate(p, bytes, alignment);
}

template <class MemSpaceTypeCategory>
inline bool operator==(MemorySpace<MemSpaceTypeCategory> const& lhs,
                       MemorySpace<MemSpaceTypeCategory> const& rhs)
{
  return &lhs == &rhs || lhs.is_equal(rhs);
}

// Default Memory Space is HostSpace
// TODO rko: maybe there is a better solution to solve this??

MemorySpace<memory_space_host_tag>*
get_default_host_space();

template <typename MemSpaceTypeCategory>
inline MemorySpace<MemSpaceTypeCategory>* get_default_memory_space()
{
  // Current we have only a default host space
  return nullptr;
}

template <>
inline MemorySpace<memory_space_host_tag>*
get_default_memory_space<memory_space_host_tag>()
{
  return get_default_host_space();
}

}  // namespace dash

#endif  // DASH__MEMORY__MEMORY_SPACE_H__INCLUDED
