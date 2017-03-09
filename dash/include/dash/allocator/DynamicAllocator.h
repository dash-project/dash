#ifndef DASH__ALLOCATOR__DYNAMIC_ALLOCATOR_H__INCLUDED
#define DASH__ALLOCATOR__DYNAMIC_ALLOCATOR_H__INCLUDED

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
  /// Convert DynamicAllocator<T> to DynamicAllocator<U>.
  template<typename U>
  struct rebind {
    typedef DynamicAllocator<U> other;
  };

public:
  /**
   * Constructor.
   * Creates a new instance of \c dash::DynamicAllocator for a given team.
   */
  DynamicAllocator(
    Team & team = dash::Team::All()) noexcept
  : _team(&team),
    _nunits(team.size())
  { }

  /**
   * Move-constructor.
   * Takes ownership of the moved instance's allocation.
   */
  DynamicAllocator(self_t && other) noexcept
  : _team(nullptr)
  {
    std::swap(_allocated, other._allocated);
    std::swap(_team, other._team);
  }

  /**
   * Default constructor, deleted.
   */
  DynamicAllocator() noexcept
    = delete;

  /**
   * Copy constructor.
   *
   * \todo[TF] This copy constructor does not copy _allocated. Correct?
   *
   * \see DashAllocatorConcept
   */
  DynamicAllocator(const self_t & other) noexcept
  : _team(other._team),
    _nunits(other._nunits)
  { }

  /**
   * Copy-constructor.
   * Does not take ownership of the copied instance's allocation.
   */
  template<class U>
  DynamicAllocator(const DynamicAllocator<U> & other) noexcept
  : _team(other._team),
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
   * \todo[TF] This smells like bad a surprise at some point...
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
    DASH_LOG_DEBUG("DynamicAllocator.=(&&)()");
    if (this != &other) {
      // Take ownership of other instance's allocation vector:
      std::swap(_allocated, other._allocated);
    }
    DASH_LOG_DEBUG("DynamicAllocator.=(&&) >");
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
    return (_team->dart_id() == rhs._team->dart_id());
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
   * Team containing units associated with the allocator's memory space.
   */
  inline dash::Team & team() const noexcept
  {
    if (_team == nullptr) {
      return dash::Team::Null();
    }
    return *_team;
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
    DASH_LOG_DEBUG("DynamicAllocator.allocate(nlocal)",
                   "number of local values:", num_local_elem);
    pointer gptr      = DART_GPTR_NULL;
    dart_storage_t ds = dart_storage<ElementType>(num_local_elem);
    if (dart_team_memregister(
        _team->dart_id(), ds.nelem, ds.dtype, lptr, &gptr) == DART_OK) {
      _allocated.push_back(std::make_pair(lptr, gptr));
    } else {
      gptr = DART_GPTR_NULL;
    }
    DASH_LOG_DEBUG("DynamicAllocator.allocate > ", gptr);
    return gptr;
  }

  /**
   * Unregister local memory segment from global memory space.
   * Does not deallocate local memory.
   *
   * Collective operation.
   *
   * \see DashDynamicAllocatorConcept
   */
  void detach(pointer gptr)
  {
    DASH_LOG_DEBUG("DynamicAllocator.detach()", "gptr:", gptr);
    if (!dash::is_initialized()) {
      // If a DASH container is deleted after dash::finalize(), global
      // memory has already been freed by dart_exit() and must not be
      // deallocated again.
      DASH_LOG_DEBUG("DynamicAllocator.detach >",
                     "DASH not initialized, abort");
      return;
    }
    DASH_ASSERT_RETURNS(
      dart_team_memderegister(_team->dart_id(), gptr),
      DART_OK);
    _allocated.erase(
      std::remove_if(
        _allocated.begin(),
        _allocated.end(),
        [&](std::pair<value_type *, pointer> e) {
          return e.second == gptr;
        }),
      _allocated.end());
    DASH_LOG_DEBUG("DynamicAllocator.detach >");
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
    if (lptr != nullptr) {
      delete[] lptr;
    }
  }

  /**
   * Allocates \c num_local_elem local elements at active unit and attaches
   * the local memory segment in global memory space.
   *
   * Collective operation.
   * The number of allocated elements may differ between units.
   *
   * \see DashAllocatorConcept
   */
  pointer allocate(size_type num_local_elem)
  {
    local_pointer lmem = allocate_local(num_local_elem);
    pointer       gmem = attach(lmem, num_local_elem);
    if (DART_GPTR_ISNULL(gmem)) {
      // Attach failed, free requested local memory:
      deallocate_local(lmem);
    }
    return gmem;
  }

  /**
   * Detaches memory segment from global memory space and deallocates the
   * associated local memory region.
   *
   * Collective operation.
   *
   * \see DashAllocatorConcept
   */
  void deallocate(pointer gptr)
  {
    DASH_LOG_DEBUG("DynamicAllocator.deallocate()", "gptr:", gptr);
    if (!dash::is_initialized()) {
      // If a DASH container is deleted after dash::finalize(), global
      // memory has already been freed by dart_exit() and must not be
      // deallocated again.
      DASH_LOG_DEBUG("DynamicAllocator.deallocate >",
                     "DASH not initialized, abort");
      return;
    }
    // Free local memory:
    DASH_LOG_DEBUG("DynamicAllocator.deallocate", "deallocate local memory");
    bool do_detach = false;
    std::for_each(
      _allocated.begin(),
      _allocated.end(),
      [&](std::pair<value_type *, pointer> e) mutable {
        if (e.second == gptr && e.first != nullptr) {
          delete[] e.first;
          e.first   = nullptr;
          do_detach = true;
          DASH_LOG_DEBUG("DynamicAllocator.deallocate",
                         "gptr", e.second, "marked for detach");
        }
      });
    // Unregister from global memory space, removes gptr from _allocated:
    if (do_detach) {
      detach(gptr);
    }
    DASH_LOG_DEBUG("DynamicAllocator.deallocate >");
  }

private:
  /**
   * Frees and detaches all global memory regions allocated by this allocator
   * instance.
   */
  void clear() noexcept
  {
    DASH_LOG_DEBUG("DynamicAllocator.clear()");
    for (auto & e : _allocated) {
      // Null-buckets have lptr set to nullptr
      if (e.first != nullptr) {
        DASH_LOG_DEBUG("DynamicAllocator.clear", "deallocate local memory:",
                       e.first);
        delete[] e.first;
        e.first = nullptr;
      }
      if (!DART_GPTR_ISNULL(e.second)) {
        DASH_LOG_DEBUG("DynamicAllocator.clear", "detach global memory:",
                       e.second);
        // Cannot use DASH_ASSERT due to noexcept qualifier:
        assert(dart_team_memderegister(_team->dart_id(), e.second) == DART_OK);
      }
    }
    _allocated.clear();
    DASH_LOG_DEBUG("DynamicAllocator.clear >");
  }

private:
  dash::Team                                    * _team;
  size_t                                          _nunits    = 0;
  std::vector< std::pair<value_type *, pointer> > _allocated;

}; // class DynamicAllocator

template <class T, class U>
bool operator==(
  const DynamicAllocator<T> & lhs,
  const DynamicAllocator<U> & rhs)
{
  return (sizeof(T)            == sizeof(U) &&
          lhs._team->dart_id() == rhs._team->dart_id() &&
          lhs._nunits          == rhs._nunits );
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

#endif // DASH__ALLOCATOR__DYNAMIC_ALLOCATOR_H__INCLUDED
