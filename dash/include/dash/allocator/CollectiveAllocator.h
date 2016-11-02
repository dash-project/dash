#ifndef DASH__ALLOCATOR__COLLECTIVE_ALLOCATOR_H__INCLUDED
#define DASH__ALLOCATOR__COLLECTIVE_ALLOCATOR_H__INCLUDED

#include <dash/dart/if/dart.h>

#include <dash/Types.h>
#include <dash/Team.h>

#include <dash/internal/Logging.h>

#include <vector>
#include <algorithm>
#include <utility>
#include <cassert>


namespace dash {
namespace allocator {

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
  explicit CollectiveAllocator(
    Team & team = dash::Team::All()) noexcept
  : _team_id(team.dart_id()),
    _nunits(team.size())
  { }

  /**
   * Move-constructor.
   * Takes ownership of the moved instance's allocation.
   */
  CollectiveAllocator(self_t && other) noexcept
  : _team_id(other._team_id),
    _nunits(other._nunits),
    _allocated(other._allocated)
  {
    other._allocated.clear();
  }

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
    DASH_LOG_DEBUG("CollectiveAllocator.allocate(nlocal)",
                   "number of local values:", num_local_elem);
    pointer gptr = DART_GPTR_NULL;
    if (num_local_elem > 0) {
      size_type num_local_bytes = sizeof(ElementType) * num_local_elem;
      if (dart_team_memalloc_aligned(
            _team_id, num_local_bytes, &gptr) == DART_OK) {
        _allocated.push_back(gptr);
      } else {
        gptr = DART_GPTR_NULL;
      }
    }
    DASH_LOG_DEBUG_VAR("CollectiveAllocator.allocate >", gptr);
    return gptr;
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
  size_t               _nunits    = 0;
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

} // namespace allocator
} // namespace dash

#endif // DASH__ALLOCATOR__COLLECTIVE_ALLOCATOR_H__INCLUDED
