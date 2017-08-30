#ifndef DASH__ALLOCATOR__LOCAL_SPACE_ALLOCATOR_H__INCLUDED
#define DASH__ALLOCATOR__LOCAL_SPACE_ALLOCATOR_H__INCLUDED

#include <dash/memory/MemorySpace.h>

namespace dash {
namespace allocator {

template <
  typename T,
  typename MSpaceCategory = dash::memory_space_host_tag>
class LocalSpaceAllocator {
  using memory_space = dash::MemorySpace<MSpaceCategory>;
  using memory_traits = dash::memory_space_traits<memory_space>;

 public:
  typedef                       T value_type;
  typedef                       T* pointer;
  typedef dash::default_size_t  size_type;

  /*
  template <typename U>
  struct rebind {
    LocalSpaceAllocator<U, MSpaceCategory> other;
  };
  */

 public:
  // Constructor
  LocalSpaceAllocator();
  explicit LocalSpaceAllocator(memory_space* space);

  // Copy Constructor
  LocalSpaceAllocator(LocalSpaceAllocator const&) = default;

  template <typename U>
  LocalSpaceAllocator(LocalSpaceAllocator<U> const& other);

  // Move Constructor
  LocalSpaceAllocator(LocalSpaceAllocator&& other);

  // Move Assignment
  LocalSpaceAllocator& operator=(LocalSpaceAllocator&& other) = default;

  // Copy Assignment
  LocalSpaceAllocator& operator=(LocalSpaceAllocator const& other) = delete;

  ~LocalSpaceAllocator();

  pointer allocate(size_t n, size_t alignment = alignof(T));
  void deallocate(pointer p, size_t n);

  size_type max_size() const noexcept;
  /*
  template <typename... Args>
  void construct(pointer, Args&&...);

  void destroy(pointer p);

  LocalSpaceAllocator select_on_copy_container_construction() const;
  */

  memory_space* space() const { return _space; };
 private:
  memory_space* _space;
};

template <typename T, typename MSpaceCategory>
inline LocalSpaceAllocator<T, MSpaceCategory>::LocalSpaceAllocator()
  : _space(get_default_memory_space<MSpaceCategory>())
{
  DASH_ASSERT(_space);
}

template <typename T, typename MSpaceCategory>
inline LocalSpaceAllocator<T, MSpaceCategory>::LocalSpaceAllocator(
    memory_space* space)
  : _space(space)
{
  DASH_ASSERT(_space);
}

template <typename T, typename MSpaceCategory>
LocalSpaceAllocator<T, MSpaceCategory>::LocalSpaceAllocator(
    LocalSpaceAllocator &&  other)
  : _space(other.space())
{
  other.space = nullptr;
}

//TODO rko: Enable if memory resource is copyable (e.g. HostSpace, but not
//Persistent Memory Space
template <typename T, typename MSpaceCategory>
template <typename U>
LocalSpaceAllocator<T, MSpaceCategory>::LocalSpaceAllocator(
    const LocalSpaceAllocator<U>& other)
  : _space(other.space())
{
}

template <typename T, typename MSpaceCategory>
inline typename LocalSpaceAllocator<T, MSpaceCategory>::pointer
LocalSpaceAllocator<T, MSpaceCategory>::allocate(size_t n, size_t alignment)
{
  DASH_LOG_DEBUG("LocalSpaceAllocator.allocate(n)",
                   "number values:", n);

  if (n > this->max_size()) throw std::bad_alloc();

  auto p =  static_cast<pointer>(_space->allocate(n * sizeof(T), alignment));

  DASH_LOG_DEBUG("LocalSpaceAllocator.allocate(n) >");

  return p;
}

template <typename T, typename MSpaceCategory>
inline void LocalSpaceAllocator<T, MSpaceCategory>::deallocate(
    typename LocalSpaceAllocator<T, MSpaceCategory>::pointer p, size_t n)
{
  _space->deallocate(p, n);
}

template <typename T, typename MSpaceCategory>
inline typename LocalSpaceAllocator<T, MSpaceCategory>::size_type
LocalSpaceAllocator<T, MSpaceCategory>::max_size() const noexcept
{
  return std::numeric_limits<size_type>::max() / sizeof(T);
}
/*
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
*/

template <typename T, typename MSpaceCategory>
inline LocalSpaceAllocator<T, MSpaceCategory>::~LocalSpaceAllocator()
{
}

} //namespace alloator
} //namespace dash

#endif
