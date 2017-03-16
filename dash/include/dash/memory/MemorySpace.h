#ifndef DASH__MEMORY__MEMORY_SPACE_H__INCLUDED
#define DASH__MEMORY__MEMORY_SPACE_H__INCLUDED

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <memory>
#include <ostream>

namespace dash {

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
   *        dash::pointer_traits<pointer>::const_pointer<pointer>::type
   *      const_pointer;
   *
   *      // ...
   *    };
   *
   */
};

struct memory_space_host_tag {
};
struct memory_space_hbw_tag {
};
struct memory_space_pmem_tag {
};

template <class MemorySpace>
struct memory_space_traits {
};

template <>
struct memory_space_traits<memory_space_host_tag> {
  typedef void *void_pointer;
};

template <class MSpaceCategory>
class MemorySpace {
 public:
  // Resolve void pointer type for this MemorySpace, typically
  // `GlobPtr<void>` for global and `void *` for local memory.
  // Allocators use rebind to obtain a fully specified type like
  // `GlobPtr<double>` and can then cast the `void_pointer`
  // returned from a memory space to their value type.
  using void_pointer =
      typename dash::memory_space_traits<MSpaceCategory>::void_pointer;

 public:
  virtual ~MemorySpace();
  virtual void_pointer allocate(size_t bytes, size_t alignment = 0) = 0;
  virtual void deallocate(void_pointer p, size_t bytes) = 0;

  virtual bool is_equal(MemorySpace const &other) const = 0;
};

template <typename MSpaceCategory>
inline bool operator==(MemorySpace<MSpaceCategory> const &a,
                       MemorySpace<MSpaceCategory> const &b)
{
  return &a == &b || a.is_equal(b);
}

// Default Memory Space is HostSpace
//TODO rko: maybe there is a better solution to solve this??
using default_memory_space = dash::MemorySpace<dash::memory_space_host_tag>;

default_memory_space *get_default_memory_space();

}  // namespace dash

#endif  // DASH__MEMORY__MEMORY_SPACE_H__INCLUDED
