#ifndef DASH__ALLOCATOR_H__INCLUDED
#define DASH__ALLOCATOR_H__INCLUDED

#include <dash/dart/if/dart.h>

#include <dash/Types.h>
#include <dash/Team.h>

#include <dash/internal/Logging.h>

namespace dash {
namespace allocator {

/**
 * Allocator of global memory regions in the active unit's local memory.
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

public:
  typedef ElementType                  value_type;
  typedef dash::default_size_t          size_type;
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
    Team & team = dash::Team::Null())
  : _team_id(team.dart_id())
  { }

  template<class U>
  LocalAllocator(const LocalAllocator<U> & other)
  : _team_id(other._team_id)
  { }

  /**
   * Default constructor, deleted.
   */
  LocalAllocator()
    = delete;

  /**
   * Copy constructor.
   *
   * \see DashAllocatorConcept
   */
  LocalAllocator(const self_t & other)
    = default;

  /**
   * Assignment operator.
   *
   * \see DashAllocatorConcept
   */
  self_t & operator=(const self_t & other)
    = default;

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
  }

private:
  dart_team_t _team_id = DART_TEAM_NULL;

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
 * Allocator of global memory regions across units in a specified team.
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

public:
  typedef ElementType                  value_type;
  typedef dash::default_size_t          size_type;
  typedef gptrdiff_t              difference_type;
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
    Team & team = dash::Team::Null())
  : _team_id(team.dart_id())
  {
    DASH_ASSERT_RETURNS(
      dart_team_size(_team_id, &_nunits),
      DART_OK);
  }

  template<class U>
  CollectiveAllocator(const CollectiveAllocator<U> & other)
  : _team_id(other._team_id),
    _nunits(other._nunits)
  { }

  /**
   * Default constructor, deleted.
   */
  CollectiveAllocator()
    = delete;

  /**
   * Copy constructor.
   *
   * \see DashAllocatorConcept
   */
  CollectiveAllocator(const self_t & other)
    = default;

  /**
   * Assignment operator.
   *
   * \see DashAllocatorConcept
   */
  self_t & operator=(const self_t & other)
    = default;

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
  }

private:
  dart_team_t _team_id = DART_TEAM_NULL;
  size_type   _nunits  = 0;

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

} // namespace allocator
} // namespace dash

#endif // DASH__ALLOCATOR_H__INCLUDED
