#ifndef DASH__ALLOCATOR__EPOCH_SYNCHRONIZED_ALLOCATOR_H__INCLUDED
#define DASH__ALLOCATOR__EPOCH_SYNCHRONIZED_ALLOCATOR_H__INCLUDED

#include <dash/dart/if/dart.h>

#include <dash/Team.h>
#include <dash/Types.h>

#include <dash/internal/Logging.h>

#include <dash/GlobPtr.h>
#include <dash/allocator/AllocatorTraits.h>
#include <dash/allocator/AllocatorBase.h>
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
    template <class T> class LocalAlloc = allocator::DefaultAllocator>
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

  using allocator_traits = std::allocator_traits<LocalAlloc<ElementType>>;

  using policy_type = allocator::AttachDetachPolicy;

  // tuple to hold the local pointer
  using allocation_rec_t =
      allocator::allocation_rec<typename allocator_traits::pointer>;

public:
  using local_allocator_type = LocalAlloc<ElementType>;
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
      Team const &team, LMemSpace *r = nullptr) noexcept;

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

private:
  struct BinaryCmpLocalPointer {
    bool operator()(const allocation_rec_t &a, const allocation_rec_t &b)
    {
      return reinterpret_cast<const uintptr_t>(a.lptr()) <
             reinterpret_cast<const uintptr_t>(b.lptr());
    }
  };

private:
  /**
   * Frees and detaches all global memory regions allocated by this allocator
   * instance.
   */
  void clear() DASH_ASSERT_NOEXCEPT
  {
    DASH_LOG_DEBUG("EpochSynchronizedAllocator.clear()");

    for (auto const &segment : _segments) {
      deallocate(segment.gptr(), segment.length());
    }
    _segments.clear();
    DASH_LOG_DEBUG("EpochSynchronizedAllocator.clear >");
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

  typename std::vector<allocation_rec_t>::iterator do_local_allocate(
      size_type num_local_elem)
  {
    auto lp = allocator_traits::allocate(_alloc, num_local_elem);

    if (!lp && num_local_elem > 0) {
      DASH_LOG_ERROR(
          "EpochSynchronizedAllocator.allocate_local",
          "cannot allocate local memory segment",
          num_local_elem);
      throw std::bad_alloc{};
    }

    allocation_rec_t rec{lp, num_local_elem, DART_GPTR_NULL};

    auto pos = std::lower_bound(
        std::begin(_segments),
        std::end(_segments),
        rec,
        BinaryCmpLocalPointer{});

    return _segments.emplace(pos, std::move(rec));
  }

  typename std::vector<allocation_rec_t>::iterator lookup_segment_by_lptr(
      local_pointer lptr)
  {
    allocation_rec_t rec{lptr, 0, DART_GPTR_NULL};

    return binary_find(
        std::begin(_segments),
        std::end(_segments),
        rec,
        BinaryCmpLocalPointer{});
  }

  typename std::vector<allocation_rec_t>::iterator lookup_segment_by_gptr(
      pointer gptr)
  {
    return std::find_if(
        std::begin(_segments),
        std::end(_segments),
        [gptr](const allocation_rec_t &a) {
          return DART_GPTR_EQUAL(a.gptr(), gptr);
        });
  }

private:
  dash::Team const *            _team{};
  std::vector<allocation_rec_t> _segments;
  local_allocator_type          _alloc;
  policy_type                   _policy;

};  // class EpochSynchronizedAllocator

///////////// Implementation ///////////////////

template <
    typename ElementType,
    typename LMemSpace,
    global_allocation_policy AllocationPolicy,
    template <class> class LocalAlloc>
EpochSynchronizedAllocator<
    ElementType,
    LMemSpace,
    AllocationPolicy,
    LocalAlloc>::
    EpochSynchronizedAllocator(Team const &team, LMemSpace *r) noexcept
  : _team(&team)
  , _alloc(
        r ? r
          : static_cast<LMemSpace *>(
                get_default_memory_space<
                    memory_domain_local,
                    typename memory_traits::memory_space_type_category>()))
  , _policy{}
{
  DASH_LOG_DEBUG(
      "EpochSynchronizedAllocator.SymmetricAllocator(team, alloc) >");
  _segments.reserve(1);
}

template <
    typename ElementType,
    typename LMemSpace,
    global_allocation_policy AllocationPolicy,
    template <class> class LocalAlloc>
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
    template <class> class LocalAlloc>
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
    template <class> class LocalAlloc>
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
  if (&other == this) {
    return *this;
  }

  _alloc = other._alloc;

  _team = other._team;

  _policy = other._policy;

  return *this;
}

template <
    typename ElementType,
    typename LMemSpace,
    global_allocation_policy AllocationPolicy,
    template <class> class LocalAlloc>
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
  if (&other == this) {
    return *this;
  }

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
    template <class> class LocalAlloc>
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
    template <class> class LocalAlloc>
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
    template <class> class LocalAlloc>
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

  auto it = do_local_allocate(num_local_elem);

  DASH_ASSERT_MSG(it->lptr() != nullptr, "invalid pointer");

  return it->lptr();
}

template <
    typename ElementType,
    typename LMemSpace,
    global_allocation_policy AllocationPolicy,
    template <class> class LocalAlloc>
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

  auto pos = lookup_segment_by_lptr(lptr);

  if (pos == std::end(_segments)) {
    DASH_LOG_ERROR(
        "EpochSynchronizedAllocator.deallocate_local",
        "invalid local pointer",
        lptr);
    return;
  }

  DASH_ASSERT_EQ(pos->length(), num_local_elem, "Invalid block length");

  if (!DART_GPTR_ISNULL(pos->gptr())) {
    DASH_LOG_ERROR(
        "EpochSynchronizedAllocator.deallocate_local",
        "local pointer must not be attached to global memory",
        lptr);
    return;
  }

  allocator_traits::deallocate(_alloc, pos->lptr(), pos->length());

  _segments.erase(pos);
}

template <
    typename ElementType,
    typename LMemSpace,
    global_allocation_policy AllocationPolicy,
    template <class> class LocalAlloc>
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

  auto pos = lookup_segment_by_lptr(lptr);

  if (pos == std::end(_segments)) {
    DASH_LOG_ERROR(
        "EpochSynchronizedAllocator.attach", "Invalid local pointer", lptr);
    return DART_GPTR_NULL;
  }

  DASH_ASSERT_EQ(num_local_elem, pos->length(), "invalid block length");

  auto gptr = _policy.do_global_attach(
      _team->dart_id(), pos->lptr(), pos->length() * sizeof(ElementType));

  DASH_ASSERT_MSG(!DART_GPTR_ISNULL(gptr), "gptr must not be null");

  pos->gptr() = gptr;

  return gptr;
}

template <
    typename ElementType,
    typename LMemSpace,
    global_allocation_policy AllocationPolicy,
    template <class> class LocalAlloc>
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

  auto pos = lookup_segment_by_gptr(gptr);

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
    template <class> class LocalAlloc>
void EpochSynchronizedAllocator<
    ElementType,
    LMemSpace,
    AllocationPolicy,
    LocalAlloc>::deallocate(pointer gptr, size_type num_local_elem)
{
  DASH_LOG_DEBUG(
      "EpochSynchronizedAllocator.deallocate", "deallocate local memory");

  if (DART_GPTR_ISNULL(gptr)) {
    return;
  }

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

  _policy.do_global_detach(pos->gptr());

  allocator_traits::deallocate(_alloc, pos->lptr(), pos->length());

  _segments.erase(pos);
}

template <
    typename ElementType,
    typename LMemSpace,
    global_allocation_policy AllocationPolicy,
    template <class> class LocalAlloc>
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

  auto it = do_local_allocate(num_local_elem);

  DASH_ASSERT_MSG(it->lptr() != nullptr, "local pointer must not be NULL");

  auto gptr = _policy.do_global_attach(
      _team->dart_id(), it->lptr(), num_local_elem * sizeof(ElementType));

  DASH_ASSERT_MSG(!DART_GPTR_ISNULL(gptr), "gptr must not be null");

  it->gptr() = gptr;

  return gptr;
}

template <
    class T,
    class U,
    class LocalMemSpace,
    global_allocation_policy AllocationPolicy,
    template <class> class LocalAlloc>
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
      sizeof(T) == sizeof(U) && *(lhs._team) == *(rhs._team) &&
      lhs._alloc == rhs._alloc);
}

template <
    class T,
    class U,
    class LocalMemSpace,
    global_allocation_policy AllocationPolicy,
    template <class> class LocalAlloc>
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
