#ifndef DASH__ALLOCATOR__DYNAMIC_ALLOCATOR_H__INCLUDED
#define DASH__ALLOCATOR__DYNAMIC_ALLOCATOR_H__INCLUDED

#include <dash/dart/if/dart.h>

#include <dash/Team.h>
#include <dash/Types.h>

#include <dash/internal/Logging.h>
#include <dash/memory/HostSpace.h>

#include <algorithm>
#include <utility>
#include <vector>

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
template <typename ElementType, typename LocalAlloc = dash::memory::HostSpace>
class DynamicAllocator {
  template <class T, class U>
  friend bool operator==(const DynamicAllocator<T> &lhs,
                         const DynamicAllocator<U> &rhs);

  template <class T, class U>
  friend bool operator!=(const DynamicAllocator<T> &lhs,
                         const DynamicAllocator<U> &rhs);

 private:
  using self_t = DynamicAllocator<ElementType>;
  using block_t = dash::memory::internal::memory_block;
  using internal_value_type = std::pair<block_t, dart_gptr_t>;

  /// Type definitions required for std::allocator concept:
 public:
  // clang-format off
  using value_type                              = ElementType;
  using size_type                               = dash::default_size_t;
  using propagate_on_container_move_assignment  = std::true_type;
  using local_alloc_traits                      = std::allocator_traits<LocalAlloc>;
  // clang-format on

  /// Type definitions required for dash::allocator concept:
 public:
  // clang-format off
  typedef dash::gptrdiff_t        difference_type;
  typedef dart_gptr_t                     pointer;
  typedef dart_gptr_t                void_pointer;
  typedef dart_gptr_t const         const_pointer;
  typedef dart_gptr_t          const_void_pointer;
  typedef value_type              * local_pointer;
  typedef const value_type *  const_local_pointer;
  // clang-format on

 public:
  /// Convert DynamicAllocator<T> to DynamicAllocator<U>.
  template <typename U>
  struct rebind {
    typedef DynamicAllocator<U> other;
  };

 public:
  /**
   * Constructor.
   * Creates a new instance of \c dash::DynamicAllocator for a given team.
   */
  explicit DynamicAllocator(Team &team = dash::Team::All()) noexcept
    : _team(&team)
    , _nunits(team.size())
    , _alloc()
  {
  }

  /**
   * Move-constructor.
   * Takes ownership of the moved instance's allocation.
   */
  DynamicAllocator(self_t &&other) noexcept
    : _team(nullptr)
  {
    std::swap(_allocated, other._allocated);
    std::swap(_team, other._team);
    std::swap(_alloc, other._alloc);
  }

  /**
   * Default constructor, deleted.
   */
  DynamicAllocator() noexcept = delete;

  /**
   * Copy constructor.
   *
   * This copy constructor does not copy _allocated since memory is strictly
   * coupled to a specific team.
   *
   * \see DashAllocatorConcept
   */
  DynamicAllocator(const self_t &other) noexcept
    : _team(other._team)
    , _nunits(other._nunits)
    , _alloc(other._alloc)
  {
  }

  /**
   * Copy-constructor.
   * Does not take ownership of the copied instance's allocation.
   */
  template <class U>
  DynamicAllocator(const DynamicAllocator<U> &other) noexcept
    : _team(other._team)
    , _nunits(other._nunits)
    , _alloc(other._alloc)
  {
  }

  /**
   * Destructor.
   * Frees all global memory regions allocated by this allocator instance.
   */
  ~DynamicAllocator() noexcept { clear(); }
  /**
   * Assignment operator.
   *
   * \todo[TF] This smells like bad a surprise at some point...
   *
   * \see DashAllocatorConcept
   */
  self_t &operator=(const self_t &other) noexcept
  {
    // noop
    return *this;
  }

  /**
   * Move-assignment operator.
   */
  self_t &operator=(const self_t &&other) noexcept
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
  bool operator!=(const self_t &rhs) const noexcept { return !(*this == rhs); }
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
   * \see DashDynamicAllocatorConcept
   */
  pointer attach(local_pointer lptr, size_type num_local_elem)
  {
    DASH_LOG_DEBUG("DynamicAllocator.attach(nlocal)",
                   "number of local values:", num_local_elem, "pointer: ", lptr);

    size_t const nbytes = num_local_elem * sizeof(value_type);

    block_t pseudoBlock(lptr, nbytes);

    // Search for corresponding memory block
    auto const end = std::end(_allocated);
    auto const found =
        std::find_if(std::begin(_allocated), end,
                     [&pseudoBlock](internal_value_type const &val) {
                       return val.first == pseudoBlock;
                     });

    if (found == end) {
      // memory block not found
      DASH_THROW(dash::exception::InvalidArgument, "attach invalid pointer");
    }
    else if (found->second != DART_GPTR_NULL) {
      // memory block is already attached
      DASH_LOG_ERROR("local memory alread attach to memory", found->second);

      DASH_THROW(dash::exception::InvalidArgument,
                 "cannot repeatedly attach local pointer");
    }

    // Attach the block
    dart_storage_t ds = dart_storage<ElementType>(num_local_elem);
    if (dart_team_memregister(_team->dart_id(), ds.nelem, ds.dtype,
                              found->first.ptr, &found->second) != DART_OK) {
      // reset to DART_GPTR_NULL
      found->second = DART_GPTR_NULL;
      DASH_LOG_ERROR("DynamicAllocator.attach", "cannot attach local memory",
                     found->first.ptr);
    }

    DASH_LOG_DEBUG("DynamicAllocator.attach > ", found->second);
    return found->second;
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

    auto const end = std::end(_allocated);
    //Look up if we can
    auto const found = std::find_if(
        std::begin(_allocated), end,
        [&gptr](internal_value_type const &val) { return val.second == gptr; });

    if (found == end) {
      DASH_LOG_DEBUG("DynamicAllocator.detach >",
                     "cannot detach untracked pointer");
      return;
    }

    if (dart_team_memderegister(gptr) != DART_OK) {
      DASH_LOG_ERROR("DynamicAllocator.detach >",
                     "cannot detach global pointer", gptr);
      DASH_ASSERT(false);
    }

    found->second = DART_GPTR_NULL;

    //Local Memory already deallocated so we can remove it from tracked memory
    if (!found->first) {
      _allocated.erase(found);
    }

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
    auto nbytes = num_local_elem * sizeof(value_type);
    auto mem = _alloc.allocate(nbytes);

    if (!mem) return nullptr;

    _allocated.push_back(std::make_pair(mem, DART_GPTR_NULL));

    DASH_LOG_TRACE("DynamicAllocator.allocate_local",
                     "allocated local pointer",
                     mem.ptr);

    return static_cast<local_pointer>(mem.ptr);
  }

  /**
   * Deallocates memory segment in the active unit's local memory.
   *
   * Local operation.
   *
   * \see DashDynamicAllocatorConcept
   */
  void deallocate_local(local_pointer lptr, size_type n)
  {
    block_t pseudoBlock(lptr, n);

    auto const end = std::end(_allocated);
    auto const found =
        std::find_if(std::begin(_allocated), end,
                     [&pseudoBlock](internal_value_type const &val) {
                       return val.first == pseudoBlock;
                     });

    if (found == end) return;

    auto const attached = found->second != DART_GPTR_NULL;

    if (attached) {
      // TODO[rk] detach memory from window...
      DASH_LOG_ERROR("DynamicAllocator.deallocate_local",
                     "deallocating local pointer which is still attached",
                     found->second);
    }

    // TODO[rk] first call the destructor
    _alloc.deallocate(found->first);

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
    local_pointer lmem = allocate_local(num_local_elem);
    pointer gmem = attach(lmem, num_local_elem);
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
    DASH_LOG_DEBUG("DynamicAllocator.deallocate", "deallocate local memory");
    auto const end = std::end(_allocated);
    auto const found = std::find_if(
        std::begin(_allocated), end,
        [&gptr](internal_value_type &val) { return val.second == gptr; });
    if (found != end) {
      // Free local memory:
      _alloc.deallocate(found->first);
      // Unregister from global memory space, removes gptr from _allocated:
      detach(found->second);
    }
    else {
      DASH_LOG_ERROR("DynamicAllocator.deallocate", "cannot deallocate gptr",
                     gptr);
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
    /*
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
        dart_ret_t ret = dart_team_memderegister(e.second);
        assert(ret == DART_OK);
      }
    }
    */
    auto & alloc_capture = _alloc;
    auto const teamId = _team->dart_id();
    std::for_each(std::begin(_allocated), std::end(_allocated),
                  [&alloc_capture, &teamId](internal_value_type &val) {
                    //Deallocate local memory
                    DASH_LOG_DEBUG("DynamicAllocator.clear",
                                   "deallocate local memory block:", val.first.ptr);
                    alloc_capture.deallocate(val.first);
                    //Deregister global memory
                    if (val.second) {
                      DASH_LOG_DEBUG("DynamicAllocator.clear",
                                     "detach global memory:", val.second);
                      // Cannot use DASH_ASSERT due to noexcept qualifier:
                      DASH_ASSERT_RETURNS(
                          dart_team_memderegister(val.second), DART_OK);
                    }
                  });
    _allocated.clear();
    DASH_LOG_DEBUG("DynamicAllocator.clear >");
  }

 private:
  dash::Team *_team;
  size_t _nunits = 0;
  std::vector<std::pair<block_t, pointer>> _allocated;
  LocalAlloc _alloc;

};  // class DynamicAllocator

template <class T, class U>
bool operator==(const DynamicAllocator<T> &lhs, const DynamicAllocator<U> &rhs)
{
  return (sizeof(T) == sizeof(U) &&
          lhs._team->dart_id() == rhs._team->dart_id() &&
          lhs._nunits == rhs._nunits && lhs._alloc == rhs._alloc);
}

template <class T, class U>
bool operator!=(const DynamicAllocator<T> &lhs, const DynamicAllocator<U> &rhs)
{
  return !(lhs == rhs);
}

}  // namespace allocator
}  // namespace dash

#endif  // DASH__ALLOCATOR__DYNAMIC_ALLOCATOR_H__INCLUDED
