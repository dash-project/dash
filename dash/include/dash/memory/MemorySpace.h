#ifndef DASH__MEMORY__MEMORY_SPACE_H__INCLUDED
#define DASH__MEMORY__MEMORY_SPACE_H__INCLUDED

#include <algorithm>
#include <memory>
#include <ostream>
#include <cstddef>

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

template <class MemorySpace>
struct memory_space_traits {
 public:
  typedef void *void_pointer;
};

template <class MSpaceCategory>
class MemorySpace {
  using self_t = MemorySpace<MSpaceCategory>;

 public:
  // Resolve void pointer type for this MemorySpace, typically
  // `GlobPtr<void>` for global and `void *` for local memory.
  // Allocators use rebind to obtain a fully specified type like
  // `GlobPtr<double>` and can then cast the `void_pointer`
  // returned from a memory space to their value type.
  using void_pointer = typename dash::memory_space_traits<self_t>::void_pointer;

  void_pointer allocate(size_t bytes,
                        std::size_t alignment = alignof(std::max_align_t));

  void deallocate(void_pointer addr, size_t nbytes);
};


namespace memory {
namespace internal {

struct memory_block {
  // friend std::ostream& operator<<(std::ostream& stream, struct memory_block
  // const& block);
  // Default Constructor
  memory_block() noexcept
    : ptr(nullptr)
    , length(0)
  {
  }

  memory_block(void *ptr, size_t length) noexcept
    : ptr(ptr)
    , length(length)
  {
  }

  // Move Constructor
  memory_block(memory_block &&x) noexcept { *this = std::move(x); }
  // Copy Constructor
  memory_block(const memory_block &x) noexcept = default;

  // Move Assignment
  memory_block &operator=(memory_block &&x) noexcept
  {
    ptr = x.ptr;
    length = x.length;
    x.reset();
    return *this;
  }

  // Copy Assignment
  memory_block &operator=(const memory_block &x) noexcept = default;

  /**
  * The Memory Block is not freed
  */
  ~memory_block() {}
  /**
   * Clear the memory block
   */
  void reset() noexcept
  {
    ptr = nullptr;
    length = 0;
  }

  /**
   * Bool operator to make the Allocator code better readable
   */
  explicit operator bool() const noexcept
  {
    return length != 0 && ptr != nullptr;
  }

  bool operator==(const memory_block &rhs) const noexcept
  {
    return static_cast<void *>(ptr) == static_cast<void *>(rhs.ptr) &&
           length == rhs.length;
  }

  bool operator!=(const memory_block &rhs) const noexcept
  {
    return !(*this == rhs);
  }

  std::ostream &operator<<(std::ostream &stream) const noexcept
  {
    stream << "memory_block { ptr: " << ptr << ", length: " << length << "}";
    return stream;
  }

  void *ptr;
  std::size_t length;
};
}  // namespace internal
}  // namespace memory
}  // namespace dash

#endif  // DASH__MEMORY__MEMORY_SPACE_H__INCLUDED
