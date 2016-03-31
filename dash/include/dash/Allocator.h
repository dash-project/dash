#ifndef DASH__ALLOCATOR_H__INCLUDED
#define DASH__ALLOCATOR_H__INCLUDED

#include <dash/dart/if/dart.h>

#include <dash/Types.h>
#include <dash/Team.h>

#include <dash/internal/Logging.h>

#include <vector>
#include <algorithm>
#include <utility>

namespace dash {
namespace allocator {

/**
 * Encapsulates a memory allocation and deallocation strategy of global
 * memory regions located in the active unit's local memory.
 *
 * Satisfied STL concepts:
 *
 * - Allocator
 * - CopyAssignable
 *
 * \concept{DashAllocatorConcept}
 */
template<typename ElementType>
class LocalAllocator
{
  template <class T, class U>
  friend bool operator==(
    const LocalAllocator<T> & lhs,
    const LocalAllocator<U> & rhs);

  template <class T, class U>
  friend bool operator!=(
    const LocalAllocator<T> & lhs,
    const LocalAllocator<U> & rhs);

private:
  typedef LocalAllocator<ElementType>      self_t;

/// Type definitions required for std::allocator concept:
public:
  using value_type                             = ElementType;
  using size_type                              = dash::default_size_t;
  using propagate_on_container_move_assignment = std::true_type;

/// Type definitions required for dash::allocator concept:
public:
  typedef dash::gptrdiff_t        difference_type;
  typedef dart_gptr_t                     pointer;
  typedef dart_gptr_t                void_pointer;
  typedef dart_gptr_t               const_pointer;
  typedef dart_gptr_t          const_void_pointer;

public:
  /**
   * Constructor.
   * Creates a new instance of \c dash::LocalAllocator for a given team.
   */
  LocalAllocator(
    Team & team = dash::Team::Null()) noexcept
  : _team_id(team.dart_id())
  { }

  /**
   * Move-constructor.
   * Takes ownership of the moved instance's allocation.
   */
  LocalAllocator(self_t && other) noexcept
  : _allocated(other._allocated)
  {
    other._allocated.clear();
  }

  /**
   * Default constructor, deleted.
   */
  LocalAllocator() noexcept
    = delete;

  /**
   * Copy constructor.
   *
   * \see DashAllocatorConcept
   */
  LocalAllocator(const self_t & other) noexcept
  : _team_id(other._team_id)
  { }

  /**
   * Copy-constructor.
   * Does not take ownership of the copied instance's allocation.
   */
  template<class U>
  LocalAllocator(const LocalAllocator<U> & other) noexcept
  : _team_id(other._team_id)
  { }

  /**
   * Destructor.
   * Frees all global memory regions allocated by this allocator instance.
   */
  ~LocalAllocator() noexcept
  {
    clear();
  }

  /**
   * Assignment operator.
   *
   * \see DashAllocatorConcept
   */
  self_t & operator=(const self_t & other) noexcept
  {
    // noop
    return *this;
  }

  /**
   * Move-assignment operator.
   */
  self_t & operator=(const self_t && other) noexcept
  {
    // Take ownership of other instance's allocation vector:
    clear();
    _allocated = other._allocated;
    other._allocated.clear();
    return *this;
  }

  /**
   * Whether storage allocated by this allocator can be deallocated
   * through the given allocator instance.
   * Establishes reflexive, symmetric, and transitive relationship.
   * Does not throw exceptions.
   *
   * \returns  true if the storage allocated by this allocator can be
   *           deallocated through the given allocator instance.
   *
   * \see DashAllocatorConcept
   */
  bool operator==(const self_t & rhs) const noexcept
  {
    return (_team_id == rhs._team_id);
  }

  /**
   * Whether storage allocated by this allocator cannot be deallocated
   * through the given allocator instance.
   * Establishes reflexive, symmetric, and transitive relationship.
   * Does not throw exceptions.
   * Same as \c !(operator==(rhs)).
   *
   * \returns  true if the storage allocated by this allocator cannot be
   *           deallocated through the given allocator instance.
   *
   * \see DashAllocatorConcept
   */
  bool operator!=(const self_t & rhs) const noexcept
  {
    return !(*this == rhs);
  }

  /**
   * Allocates \c num_local_elem local elements at active unit in global
   * memory space.
   *
   * \see DashAllocatorConcept
   */
  pointer allocate(size_type num_local_elem)
  {
    if (num_local_elem > 0) {
      size_type   num_local_bytes = sizeof(ElementType) * num_local_elem;
      dart_gptr_t gptr;
      if (dart_memalloc(num_local_bytes, &gptr) == DART_OK) {
        _allocated.push_back(gptr);
        return gptr;
      }
    }
    return DART_GPTR_NULL;
  }

  /**
   * Deallocates memory in global memory space previously allocated in the
   * active unit's local memory.
   *
   * \see DashAllocatorConcept
   */
  void deallocate(pointer gptr)
  {
    if (!dash::is_initialized()) {
      // If a DASH container is deleted after dash::finalize(), global
      // memory has already been freed by dart_exit() and must not be
      // deallocated again.
      DASH_LOG_DEBUG("LocalAllocator.deallocate >",
                     "DASH not initialized, abort");
      return;
    }
    DASH_ASSERT_RETURNS(
      dart_memfree(gptr),
      DART_OK);
    _allocated.erase(
        std::remove(_allocated.begin(), _allocated.end(), gptr),
        _allocated.end());
  }

private:
  /**
   * Frees all global memory regions allocated by this allocator instance.
   */
  void clear() noexcept
  {
    for (auto gptr : _allocated) {
      // TODO:
      // Inefficient as deallocate() applies vector.erase(std::remove)
      // for every element.
      deallocate(gptr);
    }
  }

private:
  dart_team_t          _team_id   = DART_TEAM_NULL;
  std::vector<pointer> _allocated;

}; // class LocalAllocator

template <class T, class U>
bool operator==(
  const LocalAllocator<T> & lhs,
  const LocalAllocator<U> & rhs)
{
  return (sizeof(T)    == sizeof(U) &&
          lhs._team_id == rhs._team_id);
}

template <class T, class U>
bool operator!=(
  const LocalAllocator<T> & lhs,
  const LocalAllocator<U> & rhs)
{
  return !(lhs == rhs);
}

/**
 * Encapsulates a memory allocation and deallocation strategy of global
 * memory regions distributed across local memory of units in a specified
 * team.
 *
 * Satisfied STL concepts:
 *
 * - Allocator
 * - CopyAssignable
 *
 * \concept{DashAllocatorConcept}
 */
template<typename ElementType>
class CollectiveAllocator
{
  template <class T, class U>
  friend bool operator==(
    const CollectiveAllocator<T> & lhs,
    const CollectiveAllocator<U> & rhs);

  template <class T, class U>
  friend bool operator!=(
    const CollectiveAllocator<T> & lhs,
    const CollectiveAllocator<U> & rhs);

private:
  typedef CollectiveAllocator<ElementType> self_t;

/// Type definitions required for std::allocator concept:
public:
  using value_type                             = ElementType;
  using size_type                              = dash::default_size_t;
  using propagate_on_container_move_assignment = std::true_type;

/// Type definitions required for dash::allocator concept:
public:
  typedef dash::gptrdiff_t        difference_type;
  typedef dart_gptr_t                     pointer;
  typedef dart_gptr_t                void_pointer;
  typedef dart_gptr_t               const_pointer;
  typedef dart_gptr_t          const_void_pointer;

public:
  /**
   * Constructor.
   * Creates a new instance of \c dash::CollectiveAllocator for a given team.
   */
  CollectiveAllocator(
    Team & team = dash::Team::Null()) noexcept
  : _team_id(team.dart_id())
  {
    DASH_ASSERT_RETURNS(
      dart_team_size(_team_id, &_nunits),
      DART_OK);
  }

  /**
   * Move-constructor.
   * Takes ownership of the moved instance's allocation.
   */
  CollectiveAllocator(self_t && other) noexcept
  : _allocated(other._allocated)
  {
    other._allocated.clear();
  }

  /**
   * Default constructor, deleted.
   */
  CollectiveAllocator() noexcept
    = delete;

  /**
   * Copy constructor.
   *
   * \see DashAllocatorConcept
   */
  CollectiveAllocator(const self_t & other) noexcept
  : _team_id(other._team_id),
    _nunits(other._nunits)
  { }

  /**
   * Copy-constructor.
   * Does not take ownership of the copied instance's allocation.
   */
  template<class U>
  CollectiveAllocator(const CollectiveAllocator<U> & other) noexcept
  : _team_id(other._team_id),
    _nunits(other._nunits)
  { }

  /**
   * Destructor.
   * Frees all global memory regions allocated by this allocator instance.
   */
  ~CollectiveAllocator() noexcept
  {
    clear();
  }

  /**
   * Assignment operator.
   *
   * \see DashAllocatorConcept
   */
  self_t & operator=(const self_t & other) noexcept
  {
    // noop
    return *this;
  }

  /**
   * Move-assignment operator.
   */
  self_t & operator=(const self_t && other) noexcept
  {
    // Take ownership of other instance's allocation vector:
    clear();
    _allocated = other._allocated;
    other._allocated.clear();
    return *this;
  }

  /**
   * Whether storage allocated by this allocator can be deallocated
   * through the given allocator instance.
   * Establishes reflexive, symmetric, and transitive relationship.
   * Does not throw exceptions.
   *
   * \returns  true if the storage allocated by this allocator can be
   *           deallocated through the given allocator instance.
   *
   * \see DashAllocatorConcept
   */
  bool operator==(const self_t & rhs) const noexcept
  {
    return (_team_id == rhs._team_id);
  }

  /**
   * Whether storage allocated by this allocator cannot be deallocated
   * through the given allocator instance.
   * Establishes reflexive, symmetric, and transitive relationship.
   * Does not throw exceptions.
   * Same as \c !(operator==(rhs)).
   *
   * \returns  true if the storage allocated by this allocator cannot be
   *           deallocated through the given allocator instance.
   *
   * \see DashAllocatorConcept
   */
  bool operator!=(const self_t & rhs) const noexcept
  {
    return !(*this == rhs);
  }

  /**
   * Allocates \c num_local_elem local elements at every unit in global
   * memory space.
   *
   * \see DashAllocatorConcept
   */
  pointer allocate(size_type num_local_elem)
  {
    if (num_local_elem > 0) {
      size_type   num_local_bytes = sizeof(ElementType) * num_local_elem;
      dart_gptr_t gptr;
      if (dart_team_memalloc_aligned(
            _team_id, num_local_bytes, &gptr) == DART_OK) {
        _allocated.push_back(gptr);
        return gptr;
      }
    }
    return DART_GPTR_NULL;
  }

  /**
   * Deallocates memory in global memory space previously allocated across
   * local memory of all units in the team.
   *
   * \see DashAllocatorConcept
   */
  void deallocate(pointer gptr)
  {
    if (!dash::is_initialized()) {
      // If a DASH container is deleted after dash::finalize(), global
      // memory has already been freed by dart_exit() and must not be
      // deallocated again.
      DASH_LOG_DEBUG("CollectiveAllocator.deallocate >",
                     "DASH not initialized, abort");
      return;
    }
    DASH_ASSERT_RETURNS(
      dart_team_memfree(_team_id, gptr),
      DART_OK);
    _allocated.erase(
        std::remove(_allocated.begin(), _allocated.end(), gptr),
        _allocated.end());
  }

private:
  /**
   * Frees all global memory regions allocated by this allocator instance.
   */
  void clear() noexcept
  {
    for (auto gptr : _allocated) {
      // TODO:
      // Inefficient as deallocate() applies vector.erase(std::remove)
      // for every element.
      deallocate(gptr);
    }
  }

private:
  dart_team_t          _team_id   = DART_TEAM_NULL;
  size_type            _nunits    = 0;
  std::vector<pointer> _allocated;

}; // class CollectiveAllocator

template <class T, class U>
bool operator==(
  const CollectiveAllocator<T> & lhs,
  const CollectiveAllocator<U> & rhs)
{
  return (sizeof(T)    == sizeof(U) &&
          lhs._team_id == rhs._team_id &&
          lhs._nunits  == rhs._nunits );
}

template <class T, class U>
bool operator!=(
  const CollectiveAllocator<T> & lhs,
  const CollectiveAllocator<U> & rhs)
{
  return !(lhs == rhs);
}

/**
 * Encapsulates a memory allocation and deallocation strategy of global
 * memory regions distributed across local memory of units in a specified
 * team.
 *
 * Satisfied STL concepts:
 *
 * - Allocator
 * - CopyAssignable
 *
 * \concept{DashAllocatorConcept}
 */
template<typename ElementType>
class DynamicAllocator
{
  template <class T, class U>
  friend bool operator==(
    const DynamicAllocator<T> & lhs,
    const DynamicAllocator<U> & rhs);

  template <class T, class U>
  friend bool operator!=(
    const DynamicAllocator<T> & lhs,
    const DynamicAllocator<U> & rhs);

private:
  typedef DynamicAllocator<ElementType> self_t;

/// Type definitions required for std::allocator concept:
public:
  using value_type                             = ElementType;
  using size_type                              = dash::default_size_t;
  using propagate_on_container_move_assignment = std::true_type;

/// Type definitions required for dash::allocator concept:
public:
  typedef dash::gptrdiff_t        difference_type;
  typedef dart_gptr_t                     pointer;
  typedef dart_gptr_t                void_pointer;
  typedef dart_gptr_t               const_pointer;
  typedef dart_gptr_t          const_void_pointer;
  typedef value_type              * local_pointer;
  typedef const value_type *  const_local_pointer;

public:
  /**
   * Constructor.
   * Creates a new instance of \c dash::DynamicAllocator for a given team.
   */
  DynamicAllocator(
    Team & team = dash::Team::Null()) noexcept
  : _team_id(team.dart_id())
  {
    DASH_ASSERT_RETURNS(
      dart_team_size(_team_id, &_nunits),
      DART_OK);
  }

  /**
   * Move-constructor.
   * Takes ownership of the moved instance's allocation.
   */
  DynamicAllocator(self_t && other) noexcept
  : _allocated(other._allocated)
  {
    other._allocated.clear();
  }

  /**
   * Default constructor, deleted.
   */
  DynamicAllocator() noexcept
    = delete;

  /**
   * Copy constructor.
   *
   * \see DashAllocatorConcept
   */
  DynamicAllocator(const self_t & other) noexcept
  : _team_id(other._team_id),
    _nunits(other._nunits)
  { }

  /**
   * Copy-constructor.
   * Does not take ownership of the copied instance's allocation.
   */
  template<class U>
  DynamicAllocator(const DynamicAllocator<U> & other) noexcept
  : _team_id(other._team_id),
    _nunits(other._nunits)
  { }

  /**
   * Destructor.
   * Frees all global memory regions allocated by this allocator instance.
   */
  ~DynamicAllocator() noexcept
  {
    clear();
  }

  /**
   * Assignment operator.
   *
   * \see DashAllocatorConcept
   */
  self_t & operator=(const self_t & other) noexcept
  {
    // noop
    return *this;
  }

  /**
   * Move-assignment operator.
   */
  self_t & operator=(const self_t && other) noexcept
  {
    // Take ownership of other instance's allocation vector:
    clear();
    _allocated = other._allocated;
    other._allocated.clear();
    return *this;
  }

  /**
   * Whether storage allocated by this allocator can be deallocated
   * through the given allocator instance.
   * Establishes reflexive, symmetric, and transitive relationship.
   * Does not throw exceptions.
   *
   * \returns  true if the storage allocated by this allocator can be
   *           deallocated through the given allocator instance.
   *
   * \see DashAllocatorConcept
   */
  bool operator==(const self_t & rhs) const noexcept
  {
    return (_team_id == rhs._team_id);
  }

  /**
   * Whether storage allocated by this allocator cannot be deallocated
   * through the given allocator instance.
   * Establishes reflexive, symmetric, and transitive relationship.
   * Does not throw exceptions.
   * Same as \c !(operator==(rhs)).
   *
   * \returns  true if the storage allocated by this allocator cannot be
   *           deallocated through the given allocator instance.
   *
   * \see DashAllocatorConcept
   */
  bool operator!=(const self_t & rhs) const noexcept
  {
    return !(*this == rhs);
  }

  /**
   * Register pre-allocated local memory segment of \c num_local_elem
   * elements in global memory space.
   *
   * Collective operation.
   * The number of allocated elements may differ between units.
   *
   * \see DashDynamicAllocatorConcept
   */
  pointer attach(local_pointer lptr, size_type num_local_elem)
  {
    if (num_local_elem > 0) {
      size_type    num_local_bytes = sizeof(ElementType) * num_local_elem;
      dart_gptr_t  gptr;
      if (dart_team_memregister_aligned(
            _team_id, num_local_bytes, lptr, &gptr) == DART_OK) {
        _allocated.push_back(std::make_pair(lptr, gptr));
        return gptr;
      }
      delete [] lptr;
    }
    return DART_GPTR_NULL;
  }

  /**
   * Unregister local memory segment from global memory space.
   *
   * Collective operation.
   *
   * \see DashDynamicAllocatorConcept
   */
  void detach(pointer gptr)
  {
    if (!dash::is_initialized()) {
      // If a DASH container is deleted after dash::finalize(), global
      // memory has already been freed by dart_exit() and must not be
      // deallocated again.
      DASH_LOG_DEBUG("DynamicAllocator.deallocate >",
                     "DASH not initialized, abort");
      return;
    }
    DASH_ASSERT_RETURNS(
      dart_team_memderegister(_team_id, gptr),
      DART_OK);
    std::for_each(
      _allocated.begin(),
      _allocated.end(),
      [&](std::pair<value_type *, pointer> e) mutable {
        if (e.second == gptr && e.first != nullptr) {
          delete[] e.first;
          e.first = nullptr;
        }
      });
    _allocated.erase(
      std::remove_if(
        _allocated.begin(),
        _allocated.end(),
        [&](std::pair<value_type *, pointer> e) {
          return e.second == gptr;
        }),
      _allocated.end());
  }

  /**
   * Allocates \c num_local_elem local elements in the active unit's local
   * memory.
   *
   * Local operation.
   *
   * \see DashDynamicAllocatorConcept
   */
  local_pointer allocate_local(size_type num_local_elem)
  {
    return new value_type[num_local_elem];
  }

  /**
   * Deallocates memory segment in the active unit's local memory.
   *
   * Local operation.
   *
   * \see DashDynamicAllocatorConcept
   */
  void deallocate_local(local_pointer lptr)
  {
    delete[] lptr;
  }

  /**
   * Allocates \c num_local_elem local elements at active unit in global
   * memory space.
   *
   * Collective operation.
   * The number of allocated elements may differ between units.
   *
   * \see DashAllocatorConcept
   */
  pointer allocate(size_type num_local_elem)
  {
    return attach(allocate_local(num_local_elem), num_local_elem);
  }

  /**
   * Deallocates memory in global memory space previously allocated across
   * local memory of all units in the team.
   *
   * Collective operation.
   *
   * \see DashAllocatorConcept
   */
  void deallocate(pointer gptr)
  {
    return detach(gptr);
  }

private:
  /**
   * Frees all global memory regions allocated by this allocator instance.
   */
  void clear() noexcept
  {
    for (auto gptr : _allocated) {
      // TODO:
      // Inefficient as deallocate() applies vector.erase(std::remove)
      // for every element.
      deallocate(gptr.second);
    }
  }

private:
  dart_team_t                                     _team_id   = DART_TEAM_NULL;
  size_type                                       _nunits    = 0;
  std::vector< std::pair<value_type *, pointer> > _allocated;

}; // class DynamicAllocator

template <class T, class U>
bool operator==(
  const DynamicAllocator<T> & lhs,
  const DynamicAllocator<U> & rhs)
{
  return (sizeof(T)    == sizeof(U) &&
          lhs._team_id == rhs._team_id &&
          lhs._nunits  == rhs._nunits );
}

template <class T, class U>
bool operator!=(
  const DynamicAllocator<T> & lhs,
  const DynamicAllocator<U> & rhs)
{
  return !(lhs == rhs);
}

} // namespace allocator
} // namespace dash

#endif // DASH__ALLOCATOR_H__INCLUDED
