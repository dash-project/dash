#ifndef DASH__ALLOCATOR__LOCAL_SPACE_ALLOCATOR_H__INCLUDED
#define DASH__ALLOCATOR__LOCAL_SPACE_ALLOCATOR_H__INCLUDED

#include <dash/memory/MemorySpace.h>

template <typename T, typename MSpaceCategory = dash::memory_space_host_tag>
class LocalSpaceAllocator {
  using memory_space = dash::MemorySpace<MSpaceCategory>;

 public:
  // clang-format off
  typedef T value_type;
  typedef T* pointer;
  // clang-format on

  /*
  template <typename U>
  struct rebind {
    LocalSpaceAllocator<U, MSpaceCategory> other;
  };
  */

 public:
  // Constructor
  explicit LocalSpaceAllocator(memory_space* space);

  // Copy Constructors
  LocalSpaceAllocator(LocalSpaceAllocator const&) = default;
  template <typename U>
  LocalSpaceAllocator(LocalSpaceAllocator<U> const& other);

  // Move Constructor
  // TODO rko: this depends on the underlying memory space
  LocalSpaceAllocator(LocalSpaceAllocator&& other) = default;
  // Move Assignment
  // TODO rko: this depends on the underlying memory space
  LocalSpaceAllocator& operator=(LocalSpaceAllocator&& other) = delete;

  // Copy Assignment
  LocalSpaceAllocator& operator=(LocalSpaceAllocator const& other) = delete;

  ~LocalSpaceAllocator();

  pointer allocate(size_t n);
  void deallocate(pointer, size_t n);

  template <typename... Args>
  void construct(pointer, Args&&...);

  void destroy(pointer p);

  LocalSpaceAllocator select_on_copy_container_construction() const;

  memory_space* space() const { return _space; };
 private:
  memory_space* _space;
};

template <typename T, typename MSpaceCategory>
inline LocalSpaceAllocator<T, MSpaceCategory>::LocalSpaceAllocator(
    memory_space* space)
  : _space(space)
{
  DASH_ASSERT(_space);
}

template <typename T, typename MSpaceCategory>
template <typename U>
LocalSpaceAllocator<T, MSpaceCategory>::LocalSpaceAllocator(
    const LocalSpaceAllocator<U>& other)
  : _space(other.space)
{
}

template <typename T, typename MSpaceCategory>
inline typename LocalSpaceAllocator<T, MSpaceCategory>::pointer
LocalSpaceAllocator<T, MSpaceCategory>::allocate(size_t n)
{
  return static_cast<pointer>(_space->allocate(n * sizeof(T), alignof(T)));
}

template <typename T, typename MSpaceCategory>
inline void LocalSpaceAllocator<T, MSpaceCategory>::deallocate(
    typename LocalSpaceAllocator<T, MSpaceCategory>::pointer p, size_t n)
{
  _space->deallocate(p, n);
}

template <typename T, typename MSpaceCategory, typename... Args>
void construct(typename LocalSpaceAllocator<T, MSpaceCategory>::pointer p,
               Args&&... args)
{
  ::new (p) T(std::forward<Args>(args)...);
}

template <typename T, typename MSpaceCategory>
inline LocalSpaceAllocator<T, MSpaceCategory> LocalSpaceAllocator<
    T, MSpaceCategory>::select_on_copy_container_construction() const
{
  return LocalSpaceAllocator<T, MSpaceCategory>(space());
}

template <typename T, typename MSpaceCategory, typename... Args>
void destroy(typename LocalSpaceAllocator<T, MSpaceCategory>::pointer p)
{
  p->~T();
}
template <typename T, typename MSpaceCategory>
inline LocalSpaceAllocator<T, MSpaceCategory>::~LocalSpaceAllocator()
{
}

#endif
