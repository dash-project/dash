#ifndef DASH__ALLOCATOR__SYMMETRIC_ALLOCATOR_H__INCLUDED
#define DASH__ALLOCATOR__SYMMETRIC_ALLOCATOR_H__INCLUDED

#include <dash/dart/if/dart.h>

#include <dash/Types.h>
#include <dash/Team.h>

#include <dash/internal/Logging.h>
#include <dash/internal/StreamConversion.h>

#include <dash/allocator/internal/Types.h>
#include <dash/allocator/AllocatorTraits.h>
#include <dash/allocator/LocalSpaceAllocator.h>


#include <vector>
#include <algorithm>
#include <utility>
#include <cassert>
#include <tuple>


namespace dash {
namespace allocator {

/**
 * Encapsulates a memory allocation and deallocation strategy of global
 * memory regions distributed across local memory of units in a specified
 * team.
 *
 * \note This allocator allocates a symmetric amount of memory on each node.
 *
 * Satisfied STL concepts:
 *
 * - Allocator
 * - CopyAssignable
 *
 * \concept{DashAllocatorConcept}
 */
template<typename ElementType, typename MSpaceCategory = dash::memory_space_host_tag,
  typename LocalAlloc = dash::allocator::LocalSpaceAllocator<ElementType, MSpaceCategory>>
class SymmetricAllocator
{
  template <class T, class U>
  friend bool operator==(const SymmetricAllocator<T>& lhs,
                         const SymmetricAllocator<U>& rhs);
  template <class T, class U>
  friend bool operator!=(const SymmetricAllocator<T>& lhs,
                         const SymmetricAllocator<U>& rhs);

private:
  using self_t = SymmetricAllocator<ElementType>;

  using AllocatorTraits = std::allocator_traits<LocalAlloc>;
  using allocator_type  = typename AllocatorTraits::allocator_type;
  //  using propagate_on_container_move_assignment  = std::true_type;

public:
  using value_type = typename AllocatorTraits::value_type;
  using size_type  = dash::default_size_t;
  using propagate_on_container_move_assignment = std::true_type;

  using allocator_category = dash::collective_allocator_tag;

  /// Type definitions required for dash::allocator concept:
public:
  typedef dash::gptrdiff_t difference_type;
  typedef dart_gptr_t      pointer;
  typedef dart_gptr_t      void_pointer;
  typedef dart_gptr_t      const_pointer;
  typedef dart_gptr_t      const_void_pointer;

  typedef typename AllocatorTraits::pointer       local_pointer;
  typedef typename AllocatorTraits::const_pointer const_local_pointer;

private:
  using internal_value_type = std::tuple<local_pointer, size_t, pointer>;

private:
  dart_team_t    _team_id = DART_TEAM_NULL;
  allocator_type _alloc;
  std::vector<internal_value_type> _segments;

public:
  /**
   * Constructor.
   * Creates a new instance of \c dash::SymmetricAllocator for a given team.
   */
  explicit SymmetricAllocator(Team& team = dash::Team::All()) noexcept
    : _team_id(team.dart_id())
  {
  }

  SymmetricAllocator(
      const allocator_type& local_alloc,
      dash::Team&           team = dash::Team::All()) noexcept
    : _team_id(team.dart_id())
    , _alloc(local_alloc)
  {
  }

  SymmetricAllocator(
      const allocator_type&& local_alloc,
      dash::Team&            team = dash::Team::All()) noexcept
    : _team_id(team.dart_id())
    , _alloc(local_alloc)
  {
  }

  /**
   * Move-constructor.
   * Takes ownership of the moved instance's allocation.
   */
  SymmetricAllocator(self_t&& other) noexcept
    : _team_id(other._team_id)
    , _alloc(other._alloc)
    , _segments(std::move(other._segments))
  {
    // clear origin without deallocating gptrs
    other._segments.clear();
  }

  /**
   * Copy constructor.
   *
   * \see DashAllocatorConcept
   */
  SymmetricAllocator(const self_t& other) noexcept
    : _team_id(other._team_id)
    , _alloc(other._alloc)
  {
  }

  /**
   * Copy-constructor.
   * Does not take ownership of the copied instance's allocation.
   */
  template <class U>
  SymmetricAllocator(const SymmetricAllocator<U>& other) noexcept
    : _team_id(other._team_id)
    , _alloc(other._alloc)
  {
  }

  /**
   * Destructor.
   * Frees all global memory regions allocated by this allocator instance.
   */
  ~SymmetricAllocator() noexcept
  {
    clear();
  }

  /**
   * Copy Assignment operator.
   *
   * \see DashAllocatorConcept
   */
  self_t & operator=(const self_t & other) = delete;

  /**
   * Move-assignment operator.
   */
  self_t & operator=(self_t && other) noexcept
  {
    // Take ownership of other instance's allocation vector:
    if (this != &other) {
      clear();
      _alloc = std::move(other._alloc);
      _segments = std::move(other._segments);
      _team_id = other._team_id;
      // clear origin without deallocating gptrs
      other._segments.clear();
    }
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
    return (_team_id == rhs._team_id && _alloc == rhs._alloc);
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
   * \note As allocation is symmetric, each unit has to allocate
   *       an equal number of local elements.
   *
   * \return  Global pointer to allocated memory range, or \c DART_GPTR_NULL
   *          if \c num_local_elem is 0 or less.
   *
   * \see DashAllocatorConcept
   */
  pointer allocate(size_type num_local_elem)
  {
    DASH_LOG_DEBUG("SymmetricAllocator.allocate(nlocal)",
                   "number of local values:", num_local_elem);

    DASH_ASSERT_EQ(_segments.size(), 0, "Number of allocated _segments must be 0");

    local_pointer lp = AllocatorTraits::allocate(_alloc, num_local_elem);


    if (!lp) {
      if (num_local_elem > 0) {
        std::stringstream ss;
        ss << "Allocating local segment (nelem: " << num_local_elem
           << ") failed!";
        DASH_LOG_ERROR("SymmetricAllocator.allocate", ss.str());
        DASH_THROW(dash::exception::RuntimeError, ss.str());
      }
      return DART_GPTR_NULL;
    }

    pointer gptr;
    dash::dart_storage<ElementType> ds(num_local_elem);

    if (dart_team_memregister(_team_id, ds.nelem, ds.dtype, lp, &gptr) !=
        DART_OK) {

      gptr = DART_GPTR_NULL;
      AllocatorTraits::deallocate(_alloc, lp, num_local_elem);

      DASH_LOG_ERROR(
          "SymmetricAllocator.attach", "cannot attach local memory", gptr);
    }
    else {
      DASH_LOG_TRACE("SymmetricAllocator.allocate(nlocal)", "allocated memory segment (lp, nelem, gptr)", lp, num_local_elem, gptr);
      _segments.push_back(std::make_tuple(lp, num_local_elem, gptr));
    }
    DASH_LOG_DEBUG_VAR("SymmetricAllocator.allocate >", gptr);
    return gptr;
  }

  /**
   * Deallocates memory in global memory space previously allocated across
   * local memory of all units in the team.
   *
   * \note collective operation
   *
   * \see DashAllocatorConcept
   */
  void deallocate(pointer gptr)
  {
    _deallocate(gptr, false);
  }

private:
  /**
   * Frees all global memory regions allocated by this allocator instance.
   */
  void clear() noexcept
  {
    for (auto &tuple : _segments) {
      _deallocate(std::get<2>(tuple), true);
    }
    _segments.clear();
  }
  /**
   * Deallocates memory in global memory space previously allocated in the
   * active unit's local memory.
   */
  void _deallocate(
    /// gptr to be deallocated
    pointer gptr,
    /// if true, only free memory but keep gptr in vector
    bool    keep_reference = false)
  {
    DASH_ASSERT_RANGE(0, _segments.size(), 1, "SymmatricAllocator supports only 1 or 0 memory _segments");

    if (!dash::is_initialized()) {
      // If a DASH container is deleted after dash::finalize(), global
      // memory has already been freed by dart_exit() and must not be
      // deallocated again.
      DASH_LOG_DEBUG("SymmetricAllocator.deallocate >",
                     "DASH not initialized, abort");
      return;
    }

    if (_segments.size() == 0) {
      DASH_LOG_WARN("SymmetricAllocator.deallocate >",
                     "cannot free gptr", gptr);
      return;
    }

    auto & tuple = *_segments.begin();

    auto & seg_lptr = std::get<0>(tuple);
    auto & seg_nelem = std::get<1>(tuple);
    auto & seg_gptr = std::get<2>(tuple);

    DASH_ASSERT(DART_GPTR_EQUAL(gptr, seg_gptr));

    DASH_LOG_TRACE("SymmetricAllocator.deallocate", "allocating memory segment (lptr, nelem, gptr)", seg_lptr, seg_nelem, seg_gptr);

    DASH_LOG_DEBUG("SymmetricAllocator.deallocate", "dart_team_memderegister");
    DASH_ASSERT_RETURNS(
      dart_team_memderegister(gptr),
      DART_OK);
    DASH_LOG_DEBUG("SymmetricAllocator.deallocate", "barrier");
    DASH_ASSERT_RETURNS(
      dart_barrier(_team_id),
      DART_OK);

    DASH_LOG_DEBUG("SymmetricAllocator.deallocate", "_segments.erase");
    AllocatorTraits::deallocate(_alloc, seg_lptr, seg_nelem);
    seg_lptr = nullptr;

    if (!keep_reference) {
      _segments.erase(_segments.begin());
    }

    DASH_LOG_DEBUG("SymmetricAllocator.deallocate >");
  }

}; // class SymmetricAllocator

template <class T, class U>
bool operator==(
  const SymmetricAllocator<T> & lhs,
  const SymmetricAllocator<U> & rhs)
{
  return (sizeof(T)    == sizeof(U) &&
          lhs._team_id == rhs._team_id );
}

template <class T, class U>
bool operator!=(
  const SymmetricAllocator<T> & lhs,
  const SymmetricAllocator<U> & rhs)
{
  return !(lhs == rhs);
}

} // namespace allocator
} // namespace dash

#endif // DASH__ALLOCATOR__SYMMETRIC_ALLOCATOR_H__INCLUDED
