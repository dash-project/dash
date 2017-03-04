#ifndef DASH__MEMORY__MEMORY_SPACE_H__INCLUDED
#define DASH__MEMORY__MEMORY_SPACE_H__INCLUDED

#include <memory>


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
struct pointer_traits : public std::pointer_traits<Pointer>
{
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

template <
  class MemorySpace,
  class MSpaceCategory
struct memory_space_traits
{

};

} // namespace dash

#endif // DASH__MEMORY__MEMORY_SPACE_H__INCLUDED
