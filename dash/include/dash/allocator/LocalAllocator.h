#ifndef DASH__ALLOCATOR__LOCAL_ALLOCATOR_H__INCLUDED
#define DASH__ALLOCATOR__LOCAL_ALLOCATOR_H__INCLUDED

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
  : _team_id(other._team_id), _allocated(std::move(other._allocated))
  { }

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
    DASH_LOG_DEBUG("LocalAllocator.allocate(nlocal)",
                   "number of local values:", num_local_elem);
    pointer gptr = DART_GPTR_NULL;
    if (num_local_elem > 0) {
      dart_storage_t ds = dart_storage<ElementType>(num_local_elem);
      if (dart_memalloc(ds.nelem, ds.dtype, &gptr) == DART_OK) {
        _allocated.push_back(gptr);
      } else {
        gptr = DART_GPTR_NULL;
      }
    }
    DASH_LOG_DEBUG_VAR("LocalAllocator.allocate >", gptr);
    return gptr;
  }

  /**
   * Deallocates memory in global memory space previously allocated in the
   * active unit's local memory.
   *
   * \see DashAllocatorConcept
   */
  void deallocate(pointer gptr)
  {
    DASH_LOG_DEBUG_VAR("LocalAllocator.deallocate(gptr)", gptr);
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
    DASH_LOG_DEBUG("LocalAllocator.deallocate >");
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
  dart_team_t          _team_id;
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

} // namespace allocator
} // namespace dash

#endif // DASH__ALLOCATOR__LOCAL_ALLOCATOR_H__INCLUDED
