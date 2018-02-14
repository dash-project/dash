#ifndef DASH__ALLOCATOR__EPOCH_SYNCHRONIZED_ALLOCATOR_H__INCLUDED
#define DASH__ALLOCATOR__EPOCH_SYNCHRONIZED_ALLOCATOR_H__INCLUDED

#include <dash/dart/if/dart.h>

#include <dash/Team.h>
#include <dash/Types.h>

#include <dash/internal/Logging.h>

#include <dash/GlobPtr.h>
#include <dash/allocator/AllocatorTraits.h>
#include <dash/allocator/LocalSpaceAllocator.h>
#include <dash/allocator/internal/Types.h>
#include <dash/memory/MemorySpace.h>

#include <algorithm>
#include <utility>
#include <vector>

namespace dash {

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
template <typename ElementType, typename LocalMemorySpace = dash::HostSpace>
class EpochSynchronizedAllocator {
  template <class T, class U>
  friend bool operator==(
      const EpochSynchronizedAllocator<T, LocalMemorySpace> &lhs,
      const EpochSynchronizedAllocator<U, LocalMemorySpace> &rhs);
  template <class T, class U>
  friend bool operator!=(
      const EpochSynchronizedAllocator<T, LocalMemorySpace> &lhs,
      const EpochSynchronizedAllocator<U, LocalMemorySpace> &rhs);

  /// Type definitions required for std::allocator concept:
public:
  using memory_traits = typename dash::memory_space_traits<LocalMemorySpace>;

  static_assert(memory_traits::is_local::value, "memory space must be local");

  static_assert(
      std::is_base_of<cpp17::pmr::memory_resource, LocalMemorySpace>::value,
      "Local Memory Space must be a polymorphic memory resource");

  static constexpr size_t const alignment = alignof(ElementType);

  using self_t = EpochSynchronizedAllocator<ElementType, LocalMemorySpace>;
  using allocator_type   = cpp17::pmr::polymorphic_allocator<ElementType>;
  using allocator_traits = std::allocator_traits<allocator_type>;

public:
  /// Allocator Traits
  using value_type      = ElementType;
  using size_type       = dash::default_size_t;
  using difference_type = dash::default_index_t;
  using index_type      = difference_type;

  using pointer            = dart_gptr_t;
  using const_pointer      = dart_gptr_t const;
  using void_pointer       = dart_gptr_t;
  using const_void_pointer = dart_gptr_t const;

  using local_pointer       = typename allocator_traits::pointer;
  using const_local_pointer = typename allocator_traits::const_pointer;
  using local_void_pointer  = typename allocator_traits::void_pointer;
  using const_local_void_pointer =
      typename allocator_traits::const_void_pointer;

private:
  // tuple to hold the local pointer
  using allocation_rec = std::tuple<local_pointer, size_t, dart_gptr_t>;

public:
  /// Convert EpochSynchronizedAllocator<T> to EpochSynchronizedAllocator<U>.
  template <class U>
  struct rebind {
    typedef EpochSynchronizedAllocator<U, LocalMemorySpace> other;
  };

public:
  /**
   * Constructor.
   * Creates a new instance of \c dash::EpochSynchronizedAllocator for a
   * given team.
   */
  explicit EpochSynchronizedAllocator(
      Team &         team = dash::Team::All(),
      allocator_type a    = {get_default_memory_space<
          void,
          typename memory_traits::memory_space_domain_category,
          typename memory_traits::memory_space_type_category>()}) noexcept
    : _team(&team)
    , _nunits(team.size())
    , _alloc(a)
  {
  }

  EpochSynchronizedAllocator(
      allocator_type const &localAlloc,
      Team &                team = dash::Team::All()) noexcept
    : _team(&team)
    , _nunits(team.size())
    , _alloc(localAlloc)
  {
  }

  EpochSynchronizedAllocator(
      allocator_type &&alloc, Team &team = dash::Team::All()) noexcept
    : _team(&team)
    , _nunits(team.size())
    , _alloc(std::move(alloc))
  {
  }

  EpochSynchronizedAllocator(self_t &&other) = delete;

  EpochSynchronizedAllocator() = delete;

  /**
   * Copy constructor.
   *
   * This copy constructor does not copy _allocated since memory is strictly
   * coupled to a specific team.
   *
   * \see DashAllocatorConcept
   */
  EpochSynchronizedAllocator(const self_t &other) noexcept
    : _team(other._team)
    , _nunits(other._nunits)
    , _alloc(other._alloc)
  {
    // do not copy all allocated records
  }

  /**
   * Copy-constructor.
   * Does not take ownership of the copied instance's allocation.
   */
  template <class U>
  EpochSynchronizedAllocator(
      const EpochSynchronizedAllocator<U, LocalMemorySpace> &other) noexcept
    : _team(other._team)
    , _nunits(other._nunits)
    , _alloc(other._alloc)
  {
  }

  /**
   * Destructor.
   * Frees all global memory regions allocated by this allocator instance.
   */
  ~EpochSynchronizedAllocator() noexcept
  {
    clear();
  }
  /**
   * Assignment operator deleted.
   *
   */
  self_t &operator=(const self_t &other) = delete;

#if 0
  /**
   * Move-assignment operator.
   */
  self_t &operator=(const self_t &&other) noexcept
  {
    DASH_LOG_DEBUG("EpochSynchronizedAllocator.=(&&)()");
    if (this != &other) {
      // Take ownership of other instance's allocation vector:
      std::swap(_allocated, other._allocated);
    }
    DASH_LOG_DEBUG("EpochSynchronizedAllocator.=(&&) >");
    return *this;
  }
#endif

  /**
   * Estimate the largest supported size
   */
  size_type max_size() const noexcept
  {
    return size_type(-1) / sizeof(ElementType);
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
  bool operator==(const self_t &rhs) const noexcept
  {
    return (_team->dart_id() == rhs._team->dart_id() && _alloc == rhs._alloc);
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
  bool operator!=(const self_t &rhs) const noexcept
  {
    return !(*this == rhs);
  }
  /**
   * Team containing units associated with the allocator's memory space.
   */
  inline dash::Team &team() const noexcept
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
   * \see DashEpochSynchronizedAllocatorConcept
   */
  pointer attach(local_pointer lptr, size_type num_local_elem)
  {
    DASH_LOG_DEBUG(
        "EpochSynchronizedAllocator.attach(nlocal)",
        "number of local values:",
        num_local_elem,
        "pointer: ",
        lptr);

    if (!lptr && num_local_elem == 0) {
      // PASS through without any allocation
      pointer gptr = do_attach(lptr, 0);
      _allocated.push_back(std::make_tuple(lptr, num_local_elem, gptr));
      return gptr;
    }

    // Search for corresponding memory block
    auto const end   = std::end(_allocated);
    auto const found = std::find_if(
        std::begin(_allocated), end, [&lptr](allocation_rec const &val) {
          return std::get<0>(val) == lptr;
        });

    if (found == end) {
      // memory block not found
      DASH_THROW(dash::exception::InvalidArgument, "attach invalid pointer");
      return DART_GPTR_NULL;
    }
    auto &gptr = std::get<2>(*found);
    if (!DART_GPTR_ISNULL(gptr)) {
      // memory block is already attached
      DASH_LOG_ERROR("local memory alread attached to global memory", gptr);

      DASH_THROW(
          dash::exception::InvalidArgument,
          "cannot repeatedly attach local pointer");
    }

    gptr = do_attach(lptr, num_local_elem);
    DASH_LOG_DEBUG("EpochSynchronizedAllocator.attach > ", gptr);
    return gptr;
  }

  /**
   * Unregister local memory segment from global memory space.
   * Does not deallocate local memory.
   *
   * Collective operation.
   *
   * \see DashEpochSynchronizedAllocatorConcept
   */
  void detach(pointer gptr, size_type num_local_elem)
  {
    DASH_LOG_DEBUG("EpochSynchronizedAllocator.detach()", "gptr:", gptr);
    if (!dash::is_initialized()) {
      // If a DASH container is deleted after dash::finalize(), global
      // memory has already been freed by dart_exit() and must not be
      // deallocated again.
      DASH_LOG_DEBUG(
          "EpochSynchronizedAllocator.detach >",
          "DASH not initialized, abort");
      return;
    }

    auto const end = std::end(_allocated);
    // Look up if we can
    auto const found = std::find_if(
        std::begin(_allocated),
        end,
        [gptr, num_local_elem](allocation_rec const &val) {
          return DART_GPTR_EQUAL(std::get<2>(val), gptr) &&
                 std::get<1>(val) == num_local_elem;
        });

    if (found == end) {
      DASH_LOG_ERROR(
          "EpochSynchronizedAllocator.detach >",
          "cannot detach untracked pointer",
          gptr);
      return;
    }

    do_detach(gptr);

    std::get<2>(*found) = pointer(DART_GPTR_NULL);

    DASH_LOG_DEBUG("EpochSynchronizedAllocator.detach >");
  }

  /**
   * Allocates \c num_local_elem local elements in the active unit's local
   * memory.
   *
   * Local operation.
   *
   * \see DashEpochSynchronizedAllocatorConcept
   */
  local_pointer allocate_local(size_type num_local_elem)
  {
    local_pointer lp = allocator_traits::allocate(_alloc, num_local_elem);

    if (!lp) {
      if (num_local_elem > 0) {
        std::stringstream ss;
        ss << "Allocating local segment (nelem: " << num_local_elem
           << ") failed!";
        DASH_LOG_ERROR("EpochSynchronizedAllocator.allocate_local", ss.str());
        DASH_THROW(dash::exception::RuntimeError, ss.str());
      }
      return nullptr;
    }

    _allocated.push_back(
        std::make_tuple(lp, num_local_elem, pointer(DART_GPTR_NULL)));

    DASH_LOG_TRACE(
        "EpochSynchronizedAllocator.allocate_local",
        "allocated local pointer",
        lp);

    return lp;
  }

  /**
   * Deallocates memory segment in the active unit's local memory.
   *
   * Local operation.
   *
   * \see DashEpochSynchronizedAllocatorConcept
   */
  void deallocate_local(local_pointer lptr, size_type num_local_elem)
  {
    auto const end   = std::end(_allocated);
    auto const found = std::find_if(
        std::begin(_allocated), end, [lptr](allocation_rec const &val) {
          return std::get<0>(val) == lptr;
        });

    if (found == end) return;

    auto &     gptr     = std::get<2>(*found);
    bool const attached = !(DART_GPTR_ISNULL(gptr));
    if (attached) {
      DASH_LOG_ERROR(
          "EpochSynchronizedAllocator.deallocate_local",
          "deallocating local pointer which is still attached",
          gptr);
    }

    // Maybe we should first call the destructor
    allocator_traits::deallocate(_alloc, lptr, num_local_elem);

    if (!attached) _allocated.erase(found);
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
    local_pointer lp = allocate_local(num_local_elem);
    pointer       gp = attach(lp, num_local_elem);
    if (DART_GPTR_ISNULL(gp)) {
      // Attach failed, free requested local memory:
      deallocate_local(lp, num_local_elem);
    }
    return gp;
  }

  /**
   * Detaches memory segment from global memory space and deallocates the
   * associated local memory region.
   *
   * Collective operation.
   *
   * \see DashAllocatorConcept
   */
  void deallocate(pointer gptr, size_type num_local_elem)
  {
    DASH_LOG_DEBUG(
        "EpochSynchronizedAllocator.deallocate", "deallocate local memory");
    auto const end   = std::end(_allocated);
    auto const found = std::find_if(
        std::begin(_allocated),
        end,
        [gptr, num_local_elem](allocation_rec &val) {
          return DART_GPTR_EQUAL(std::get<2>(val), gptr) &&
                 std::get<1>(val) == num_local_elem;
        });
    if (found != end) {
      // Unregister from global memory space, removes gptr from _allocated:
      do_detach(gptr);
      // Free local memory:
      allocator_traits::deallocate(
          _alloc, std::get<0>(*found), num_local_elem);

      // erase from locally tracked blocks
      _allocated.erase(found);
    }
    else {
      DASH_LOG_ERROR(
          "EpochSynchronizedAllocator.deallocate",
          "cannot deallocate gptr",
          gptr);
    }

    DASH_LOG_DEBUG("EpochSynchronizedAllocator.deallocate >");
  }

  allocator_type allocator()
  {
    return _alloc;
  }

private:
  /**
   * Frees and detaches all global memory regions allocated by this allocator
   * instance.
   */
  void clear() noexcept
  {
    DASH_LOG_DEBUG("EpochSynchronizedAllocator.clear()");

    auto &     alloc_capture = _alloc;
    auto const teamId        = _team->dart_id();
    std::for_each(
        std::begin(_allocated),
        std::end(_allocated),
        [&alloc_capture, &teamId](allocation_rec const &block) {
          // Deallocate local memory
          auto &lptr = std::get<0>(block);
          auto const &nels = std::get<1>(block);
          auto const &gptr = std::get<2>(block);
          DASH_LOG_DEBUG(
              "EpochSynchronizedAllocator.clear",
              "deallocate local memory block:",
              lptr);
          allocator_traits::deallocate(
              alloc_capture, lptr, nels);
          // Deregister global memory
          if (!DART_GPTR_ISNULL(gptr)) {
            DASH_LOG_DEBUG(
                "EpochSynchronizedAllocator.clear",
                "detach global memory:",
                gptr);
            // Cannot use DASH_ASSERT due to noexcept qualifier:
            DASH_ASSERT_RETURNS(
                dart_team_memderegister(gptr), DART_OK);
          }
        });
    _allocated.clear();
    DASH_LOG_DEBUG("EpochSynchronizedAllocator.clear >");
  }

  pointer do_attach(local_pointer ptr, size_type num_local_elem)
  {
    // Attach the block
    dash::dart_storage<value_type> ds(num_local_elem);
    dart_gptr_t                    dgptr;
    if (dart_team_memregister(
            _team->dart_id(), ds.nelem, ds.dtype, ptr, &dgptr) != DART_OK) {
      // reset to DART_GPTR_NULL
      // found->second = pointer(DART_GPTR_NULL);
      DASH_LOG_ERROR(
          "EpochSynchronizedAllocator.attach",
          "cannot attach local memory",
          ptr);
    }
    else {
      // found->second = pointer(dgptr);
    }
    DASH_LOG_DEBUG("EpochSynchronizedAllocator.attach > ", dgptr);
    return dgptr;
  }

  void do_detach(pointer gptr)
  {
    if (dart_team_memderegister(gptr) != DART_OK) {
      DASH_LOG_ERROR(
          "EpochSynchronizedAllocator.do_detach >",
          "cannot detach global pointer",
          gptr);
      DASH_THROW(
          dash::exception::RuntimeError, "Cannot detach global pointer");
    }
  }

private:
  dash::Team *                _team;
  size_t                      _nunits = 0;
  std::vector<allocation_rec> _allocated;
  allocator_type              _alloc;

};  // class EpochSynchronizedAllocator

template <class T, class U, class MemSpace>
bool operator==(
    const EpochSynchronizedAllocator<T, MemSpace> &lhs,
    const EpochSynchronizedAllocator<U, MemSpace> &rhs)
{
  return (
      sizeof(T) == sizeof(U) &&
      lhs._team->dart_id() == rhs._team->dart_id() &&
      lhs._nunits == rhs._nunits && lhs._alloc == rhs._alloc);
}

template <class T, class U, class MemSpace>
bool operator!=(
    const EpochSynchronizedAllocator<T, MemSpace> &lhs,
    const EpochSynchronizedAllocator<U, MemSpace> &rhs)
{
  return !(lhs == rhs);
}

}  // namespace dash

#endif  // DASH__ALLOCATOR__EPOCH_SYNCHRONIZED_ALLOCATOR_H__INCLUDED
