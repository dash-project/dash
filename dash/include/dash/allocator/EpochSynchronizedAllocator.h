#ifndef DASH__ALLOCATOR__EPOCH_SYNCHRONIZED_ALLOCATOR_H__INCLUDED
#define DASH__ALLOCATOR__EPOCH_SYNCHRONIZED_ALLOCATOR_H__INCLUDED

#include <dash/dart/if/dart.h>

#include <dash/Team.h>
#include <dash/Types.h>

#include <dash/internal/Logging.h>

#include <dash/GlobPtr.h>
#include <dash/allocator/AllocatorTraits.h>
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
template <
    /// The Element Type
    typename ElementType,
    /// The Memory Space
    typename LMemSpace = dash::HostSpace,
    /// Allocation Policy
    global_allocation_policy AllocationPolicy =
        global_allocation_policy::epoch_synchronized,
    /// The Allocator Strategy to allocate from the local memory space
    template <class T, class MemSpace> class LocalAlloc =
        allocator::DefaultAllocator>
class EpochSynchronizedAllocator {
  template <class T, class U>
  friend bool operator==(
      const EpochSynchronizedAllocator<
          T,
          LMemSpace,
          AllocationPolicy,
          LocalAlloc> &lhs,
      const EpochSynchronizedAllocator<
          U,
          LMemSpace,
          AllocationPolicy,
          LocalAlloc> &rhs);
  template <class T, class U>
  friend bool operator!=(
      const EpochSynchronizedAllocator<
          T,
          LMemSpace,
          AllocationPolicy,
          LocalAlloc> &lhs,
      const EpochSynchronizedAllocator<
          U,
          LMemSpace,
          AllocationPolicy,
          LocalAlloc> &rhs);

  /// Type definitions required for std::allocator concept:
  using memory_traits = typename dash::memory_space_traits<LMemSpace>;

  static_assert(memory_traits::is_local::value, "memory space must be local");

  static_assert(
      AllocationPolicy == global_allocation_policy::epoch_synchronized,
      "currently, only the epoch_synchronized allocation policy is "
      "supported");

  using self_t = EpochSynchronizedAllocator<
      ElementType,
      LMemSpace,
      AllocationPolicy,
      LocalAlloc>;

  static constexpr size_t const alignment = alignof(ElementType);

  using allocator_traits =
      std::allocator_traits<LocalAlloc<ElementType, LMemSpace>>;

  using policy_type = allocator::AttachDetachPolicy<ElementType>;

  // tuple to hold the local pointer
  using allocation_rec_t =
      allocator::allocation_rec<typename allocator_traits::pointer>;

public:
  using local_allocator_type = LocalAlloc<ElementType, LMemSpace>;
  /// Allocator Traits
  using value_type      = ElementType;
  using size_type       = dash::default_size_t;
  using difference_type = dash::default_index_t;

  using pointer            = dart_gptr_t;
  using const_pointer      = dart_gptr_t const;
  using void_pointer       = dart_gptr_t;
  using const_void_pointer = dart_gptr_t const;

  using local_pointer       = typename allocator_traits::pointer;
  using const_local_pointer = typename allocator_traits::const_pointer;
  using local_void_pointer  = typename allocator_traits::void_pointer;
  using const_local_void_pointer =
      typename allocator_traits::const_void_pointer;

  using allocation_policy =
      std::integral_constant<global_allocation_policy, AllocationPolicy>;

  /// Convert EpochSynchronizedAllocator<T> to EpochSynchronizedAllocator<U>.
  template <class U>
  using rebind = dash::
      EpochSynchronizedAllocator<U, LMemSpace, AllocationPolicy, LocalAlloc>;

public:
  /**
   * Constructor.
   * Creates a new instance of \c dash::EpochSynchronizedAllocator for a
   * given team.
   */
  explicit EpochSynchronizedAllocator(
      Team const &         team,
      local_allocator_type a = {static_cast<LMemSpace *>(
          get_default_local_memory_space<
              typename memory_traits::
                  memory_space_type_category>())}) noexcept;

  /**
   * Copy constructor.
   *
   * \see DashAllocatorConcept
   */
  EpochSynchronizedAllocator(const self_t &other) noexcept;

  /**
   * Copy Assignment operator.
   *
   * \see DashAllocatorConcept
   */
  self_t &operator=(const self_t &other) noexcept;

  /**
   * Move-constructor.
   * Takes ownership of the moved instance's allocation.
   */
  EpochSynchronizedAllocator(self_t &&other) noexcept;

  /**
   * Move-assignment operator.
   */
  self_t &operator=(self_t &&other) noexcept;

  /**
   * Swap two collective Allocators
   */
  void swap(self_t &other) noexcept;

  /**
   * Destructor.
   * Frees all global memory regions allocated by this allocator instance.
   */
  ~EpochSynchronizedAllocator() noexcept;

  /**
   * Estimate the largest supported size
   */
  size_type max_size() const noexcept
  {
    return size_type(-1) / sizeof(ElementType);
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
  pointer attach(local_pointer lptr, size_type num_local_elem);

  /**
   * Unregister local memory segment from global memory space.
   * Does not deallocate local memory.
   *
   * Collective operation.
   *
   * \see DashEpochSynchronizedAllocatorConcept
   */
  void detach(pointer gptr, size_type num_local_elem);

  /**
   * Allocates \c num_local_elem local elements in the active unit's local
   * memory.
   *
   * Local operation.
   *
   * \see DashEpochSynchronizedAllocatorConcept
   */
  local_pointer allocate_local(size_type num_local_elem);

  /**
   * Deallocates memory segment in the active unit's local memory.
   *
   * Local operation.
   *
   * \see DashEpochSynchronizedAllocatorConcept
   */
  void deallocate_local(local_pointer lptr, size_type num_local_elem);

  /**
   * Allocates a global memory segment with at least num_local_elem bytes. It
   * is semantically identical to call allocate_local (obtaining a local
   * pointer) and subsequently attaching it to obtain a global pointer. it.
   * Collective operation.
   *
   * \see DashAllocatorConcept
   */
  pointer allocate(size_type num_local_elem);

  /**
   * Deallocates a memory segment from global memory space. It is semantically
   * identical to first call detach on the specified global pointer and
   * subsequently locally deallocate it.
   *
   * Collective operation.
   *
   * \see DashAllocatorConcept
   */
  void deallocate(pointer gptr, size_type num_local_elem);

  /**
   * Returns a copy of the local allocator object associated with the vector.
   */
  inline local_allocator_type get_local_allocator() const noexcept
  {
    return _alloc;
  }

#if 0
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
      _segments.push_back(std::make_tuple(lptr, num_local_elem, gptr));
      return gptr;
    }

    // Search for corresponding memory block
    auto const end   = std::end(_segments);
    auto const found = std::find_if(
        std::begin(_segments), end, [&lptr](allocation_rec const &val) {
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

    auto const end = std::end(_segments);
    // Look up if we can
    auto const found = std::find_if(
        std::begin(_segments),
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

    _segments.push_back(
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
    auto const end   = std::end(_segments);
    auto const found = std::find_if(
        std::begin(_segments), end, [lptr](allocation_rec const &val) {
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

    if (!attached) _segments.erase(found);
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
    auto const end   = std::end(_segments);
    auto const found = std::find_if(
        std::begin(_segments),
        end,
        [gptr, num_local_elem](allocation_rec &val) {
          return DART_GPTR_EQUAL(std::get<2>(val), gptr) &&
                 std::get<1>(val) == num_local_elem;
        });
    if (found != end) {
      // Unregister from global memory space, removes gptr from _segments:
      do_detach(gptr);
      // Free local memory:
      allocator_traits::deallocate(
          _alloc, std::get<0>(*found), num_local_elem);

      // erase from locally tracked blocks
      _segments.erase(found);
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

#endif

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
        std::begin(_segments),
        std::end(_segments),
        [](allocation_rec_t const &block) {

        });
    _segments.clear();
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

  template <class ForwardIt, class Compare>
  ForwardIt binary_find(
      ForwardIt               first,
      ForwardIt               last,
      const allocation_rec_t &value,
      Compare                 comp)
  {
    first = std::lower_bound(first, last, value, comp);
    return first != last && !comp(value, *first) ? first : last;
  }

private:
  dash::Team const *_team;
  // size_t                        _nunits = 0;
  std::vector<allocation_rec_t> _segments;
  local_allocator_type          _alloc;
  policy_type                   _policy;

};  // class EpochSynchronizedAllocator

///////////// Implementation ///////////////////

template <
    typename ElementType,
    typename LMemSpace,
    global_allocation_policy AllocationPolicy,
    template <class, class> class LocalAlloc>
EpochSynchronizedAllocator<
    ElementType,
    LMemSpace,
    AllocationPolicy,
    LocalAlloc>::
    EpochSynchronizedAllocator(
        Team const &team, local_allocator_type a) noexcept
  : _team(&team)
  , _alloc(a)
  , _policy{}
{
  DASH_LOG_DEBUG("SymmatricAllocator.SymmetricAllocator(team, alloc) >");
  _segments.reserve(1);
}

template <
    typename ElementType,
    typename LMemSpace,
    global_allocation_policy AllocationPolicy,
    template <class, class> class LocalAlloc>
EpochSynchronizedAllocator<
    ElementType,
    LMemSpace,
    AllocationPolicy,
    LocalAlloc>::EpochSynchronizedAllocator(self_t const &other) noexcept
{
  operator=(other);
}

template <
    typename ElementType,
    typename LMemSpace,
    global_allocation_policy AllocationPolicy,
    template <class, class> class LocalAlloc>
EpochSynchronizedAllocator<
    ElementType,
    LMemSpace,
    AllocationPolicy,
    LocalAlloc>::EpochSynchronizedAllocator(self_t &&other) noexcept
  : _team(other._team)
  , _alloc(std::move(other._alloc))
  , _segments(std::move(other._segments))
  , _policy(other._policy)
{
  other.clear();
}

template <
    typename ElementType,
    typename LMemSpace,
    global_allocation_policy AllocationPolicy,
    template <class, class> class LocalAlloc>
typename EpochSynchronizedAllocator<
    ElementType,
    LMemSpace,
    AllocationPolicy,
    LocalAlloc>::self_t &
                 EpochSynchronizedAllocator<
                     ElementType,
                     LMemSpace,
                     AllocationPolicy,
                     LocalAlloc>::operator=(self_t const &other) noexcept
{
  if (&other == this) return *this;

  _alloc = other._alloc;

  _team = other._team;

  _policy = other._policy;

  return *this;
}

template <
    typename ElementType,
    typename LMemSpace,
    global_allocation_policy AllocationPolicy,
    template <class, class> class LocalAlloc>
typename EpochSynchronizedAllocator<
    ElementType,
    LMemSpace,
    AllocationPolicy,
    LocalAlloc>::self_t &
                 EpochSynchronizedAllocator<
                     ElementType,
                     LMemSpace,
                     AllocationPolicy,
                     LocalAlloc>::operator=(self_t &&other) noexcept
{
  if (&other == this) return *this;

  if (_alloc == other._alloc) {
    // If the local allocators equal each other we can move everything
    clear();
    swap(other);
  }
  else {
    // otherwise we do not touch any data and copy assign it
    operator=(other);
  }

  return *this;
}

template <
    typename ElementType,
    typename LMemSpace,
    global_allocation_policy AllocationPolicy,
    template <class, class> class LocalAlloc>
EpochSynchronizedAllocator<
    ElementType,
    LMemSpace,
    AllocationPolicy,
    LocalAlloc>::~EpochSynchronizedAllocator() noexcept
{
  clear();
}

template <
    typename ElementType,
    typename LMemSpace,
    global_allocation_policy AllocationPolicy,
    template <class, class> class LocalAlloc>
void EpochSynchronizedAllocator<
    ElementType,
    LMemSpace,
    AllocationPolicy,
    LocalAlloc>::swap(self_t &other) noexcept
{
  std::swap(_team, other._team);
  std::swap(_alloc, other._alloc);
  std::swap(_segments, other._segments);
  std::swap(_policy, other._policy);
}

template <
    typename ElementType,
    typename LMemSpace,
    global_allocation_policy AllocationPolicy,
    template <class, class> class LocalAlloc>
typename EpochSynchronizedAllocator<
    ElementType,
    LMemSpace,
    AllocationPolicy,
    LocalAlloc>::local_pointer
EpochSynchronizedAllocator<
    ElementType,
    LMemSpace,
    AllocationPolicy,
    LocalAlloc>::allocate_local(size_type num_local_elem)
{
  DASH_LOG_DEBUG(
      "SymmetricAllocator.allocate_local(n)",
      "number of local values:",
      num_local_elem);

  auto lp = allocator_traits::allocate(_alloc, num_local_elem);

  if (!lp && num_local_elem > 0) {
    DASH_LOG_ERROR(
        "EpochSynchronizedAllocator.allocate_local",
        "cannot allocate local memory segment",
        num_local_elem);
    throw std::bad_alloc{};
  }

  auto const comp = [](const allocation_rec_t &a, const allocation_rec_t &b) {
    return reinterpret_cast<const uintptr_t>(a.lptr()) <
           reinterpret_cast<const uintptr_t>(b.lptr());
  };

  allocation_rec_t rec{lp, num_local_elem, DART_GPTR_NULL};

  auto pos =
      std::lower_bound(std::begin(_segments), std::end(_segments), rec, comp);

  _segments.insert(pos, rec);

  return lp;
}

template <
    typename ElementType,
    typename LMemSpace,
    global_allocation_policy AllocationPolicy,
    template <class, class> class LocalAlloc>
void EpochSynchronizedAllocator<
    ElementType,
    LMemSpace,
    AllocationPolicy,
    LocalAlloc>::
    deallocate_local(local_pointer lptr, size_type num_local_elem)
{
  DASH_LOG_DEBUG(
      "EpochSynchronizedAllocator.deallocate_local(n)",
      "local pointer",
      lptr,
      "length:",
      num_local_elem);

  auto const comp = [](const allocation_rec_t &a, const allocation_rec_t &b) {
    return reinterpret_cast<const uintptr_t>(a.lptr()) <
           reinterpret_cast<const uintptr_t>(b.lptr());
  };

  allocation_rec_t rec{lptr, num_local_elem, DART_GPTR_NULL};

  auto pos =
      binary_find(std::begin(_segments), std::end(_segments), rec, comp);

  if (pos == std::end(_segments)) {
    DASH_LOG_ERROR(
        "EpochSynchronizedAllocator.deallocate_local",
        "invalid local pointer",
        lptr);
    return;
  }

  if (!DART_GPTR_ISNULL(pos->gptr())) {
    DASH_LOG_ERROR(
        "EpochSynchronizedAllocator.deallocate_local",
        "local pointer must not be attach to global memory",
        lptr);
    return;
  }

  DASH_ASSERT_EQ(pos->length(), num_local_elem, "Invalid block length");

  allocator_traits::deallocate(_alloc, pos->lptr(), pos->length());

  _segments.erase(pos);
}

template <
    typename ElementType,
    typename LMemSpace,
    global_allocation_policy AllocationPolicy,
    template <class, class> class LocalAlloc>
typename EpochSynchronizedAllocator<
    ElementType,
    LMemSpace,
    AllocationPolicy,
    LocalAlloc>::pointer
EpochSynchronizedAllocator<
    ElementType,
    LMemSpace,
    AllocationPolicy,
    LocalAlloc>::attach(local_pointer const lptr, size_type num_local_elem)
{
  DASH_LOG_DEBUG(
      "EpochSynchronizedAllocator.attach",
      "local_pointer:",
      lptr,
      "length:",
      num_local_elem);

  auto const comp = [](const allocation_rec_t &a, const allocation_rec_t &b) {
    return reinterpret_cast<const uintptr_t>(a.lptr()) <
           reinterpret_cast<const uintptr_t>(b.lptr());
  };

  allocation_rec_t rec{lptr, num_local_elem, DART_GPTR_NULL};

  auto pos =
      binary_find(std::begin(_segments), std::end(_segments), rec, comp);

  if (pos == std::end(_segments)) {
    DASH_LOG_ERROR(
        "EpochSynchronizedAllocator.attach", "Invalid local pointer", lptr);
    return DART_GPTR_NULL;
  }

  DASH_ASSERT_EQ(num_local_elem, pos->length(), "invalid block length");

  auto gptr =
      _policy.do_global_attach(_team->dart_id(), pos->lptr(), pos->length());

  DASH_ASSERT_MSG(!DART_GPTR_ISNULL(gptr), "gptr must not be null");

  pos->gptr() = gptr;

  return gptr;
}

template <
    typename ElementType,
    typename LMemSpace,
    global_allocation_policy AllocationPolicy,
    template <class, class> class LocalAlloc>
void EpochSynchronizedAllocator<
    ElementType,
    LMemSpace,
    AllocationPolicy,
    LocalAlloc>::detach(pointer const gptr, size_type num_local_elem)
{
  DASH_LOG_DEBUG(
      "EpochSynchronizedAllocator.detach",
      "global pointer:",
      gptr,
      "length:",
      num_local_elem);

  if (DART_GPTR_ISNULL(gptr)) {
    return;
  }

  auto pos = std::find_if(
      std::begin(_segments),
      std::end(_segments),
      [&gptr](const allocation_rec_t &a) {
        return DART_GPTR_EQUAL(a.gptr(), gptr);
      });

  if (pos == std::end(_segments)) {
    DASH_LOG_ERROR(
        "EpochSynchronizedAllocator.detach", "Invalid global pointer", gptr);
    return;
  }

  DASH_ASSERT_EQ(num_local_elem, pos->length(), "invalid block length");

  _policy.do_global_detach(pos->gptr());

  pos->gptr() = DART_GPTR_NULL;
}

template <
    typename ElementType,
    typename LMemSpace,
    global_allocation_policy AllocationPolicy,
    template <class, class> class LocalAlloc>
void EpochSynchronizedAllocator<
    ElementType,
    LMemSpace,
    AllocationPolicy,
    LocalAlloc>::deallocate(pointer gptr, size_type num_local_elem)
{
  DASH_LOG_DEBUG(
      "EpochSynchronizedAllocator.deallocate", "deallocate local memory");

  auto pos = std::find_if(
      std::begin(_segments),
      std::end(_segments),
      [gptr](const allocation_rec_t &a) {
        return DART_GPTR_EQUAL(a.gptr(), gptr);
      });

  if (pos == std::end(_segments)) {
    DASH_LOG_ERROR(
        "EpochSynchronizedAllocator.detach", "Invalid global pointer", gptr);
    return;
  }

  DASH_ASSERT_EQ(pos->length(), num_local_elem, "invalid block length");

  detach(gptr, num_local_elem);

  deallocate_local(pos->lptr(), num_local_elem);
}

template <
    typename ElementType,
    typename LMemSpace,
    global_allocation_policy AllocationPolicy,
    template <class, class> class LocalAlloc>
typename EpochSynchronizedAllocator<
    ElementType,
    LMemSpace,
    AllocationPolicy,
    LocalAlloc>::pointer
EpochSynchronizedAllocator<
    ElementType,
    LMemSpace,
    AllocationPolicy,
    LocalAlloc>::allocate(size_type num_local_elem)
{
  DASH_LOG_DEBUG(
      "EpochSynchronizedAllocator.allocate",
      "num_local_elem",
      num_local_elem);

  local_pointer lptr = allocate_local(num_local_elem);

  pointer gptr = attach(lptr, num_local_elem);

  return gptr;
}

template <
    class T,
    class U,
    class LocalMemSpace,
    global_allocation_policy AllocationPolicy,
    template <class, class> class LocalAlloc>
bool operator==(
    const EpochSynchronizedAllocator<
        T,
        LocalMemSpace,
        AllocationPolicy,
        LocalAlloc> &lhs,
    const EpochSynchronizedAllocator<
        U,
        LocalMemSpace,
        AllocationPolicy,
        LocalAlloc> &rhs)
{
  return (
      sizeof(T) == sizeof(U) && lhs._team == rhs._team &&
      lhs._alloc == rhs._alloc);
}

template <
    class T,
    class U,
    class LocalMemSpace,
    global_allocation_policy AllocationPolicy,
    template <class, class> class LocalAlloc>
bool operator!=(
    const EpochSynchronizedAllocator<
        T,
        LocalMemSpace,
        AllocationPolicy,
        LocalAlloc> &lhs,
    const EpochSynchronizedAllocator<
        U,
        LocalMemSpace,
        AllocationPolicy,
        LocalAlloc> &rhs)
{
  return !(lhs == rhs);
}

}  // namespace dash

#endif  // DASH__ALLOCATOR__EPOCH_SYNCHRONIZED_ALLOCATOR_H__INCLUDED
