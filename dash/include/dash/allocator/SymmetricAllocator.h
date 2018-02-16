#ifndef DASH__ALLOCATOR__SYMMETRIC_ALLOCATOR_H__INCLUDED
#define DASH__ALLOCATOR__SYMMETRIC_ALLOCATOR_H__INCLUDED

#include <dash/dart/if/dart.h>

#include <dash/Team.h>
#include <dash/Types.h>

#include <dash/memory/MemorySpace.h>

#include <dash/internal/Logging.h>
#include <dash/internal/StreamConversion.h>

#include <dash/allocator/AllocatorBase.h>
#include <dash/allocator/AllocatorTraits.h>
#include <dash/allocator/internal/Types.h>

#include <algorithm>
#include <cassert>
#include <tuple>
#include <utility>
#include <vector>

namespace dash {

/**
 * Encapsulates a memory allocation and deallocation strategy of local
 * memory regions within a single dash::unit.
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
    global_allocation_policy allocation_policy,
    /// The Local Memory Space (e.g., HostSpace)
    typename LocalMemorySpace = dash::HostSpace,
    /// The Allocator Strategy to allocate from the local memory space
    template <class T, class MemSpace> class LocalAlloc =
        allocator::DefaultAllocator>
class SymmetricAllocator {
  template <class T, class U>
  friend bool operator==(
      const SymmetricAllocator<
          T,
          allocation_policy,
          LocalMemorySpace,
          LocalAlloc>& lhs,
      const SymmetricAllocator<
          U,
          allocation_policy,
          LocalMemorySpace,
          LocalAlloc>& rhs);

  template <class T, class U>
  friend bool operator!=(
      const SymmetricAllocator<
          T,
          allocation_policy,
          LocalMemorySpace,
          LocalAlloc>& lhs,
      const SymmetricAllocator<
          U,
          allocation_policy,
          LocalMemorySpace,
          LocalAlloc>& rhs);

  using memory_traits = typename dash::memory_space_traits<LocalMemorySpace>;

  static_assert(memory_traits::is_local::value, "memory space must be local");

  static_assert(
      std::is_base_of<cpp17::pmr::memory_resource, LocalMemorySpace>::value,
      "Local Memory Space must be a polymorphic memory resource");

  static_assert(
      allocation_policy == global_allocation_policy::collective ||
          allocation_policy == global_allocation_policy::non_collective,
      "only collective or non-collective allocation policies supported");

  using self_t = SymmetricAllocator<
      ElementType,
      allocation_policy,
      LocalMemorySpace,
      LocalAlloc>;

  using allocator_type = LocalAlloc<ElementType, LocalMemorySpace>;

  using allocator_traits = std::allocator_traits<allocator_type>;

  using policy_type = typename std::conditional<
      allocation_policy == global_allocation_policy::collective,
      allocator::CollectiveAllocationPolicy<
          LocalAlloc<ElementType, LocalMemorySpace>>,
      allocator::LocalAllocationPolicy<
          LocalAlloc<ElementType, LocalMemorySpace>>>::type;

public:
  /// Allocator Traits
  using value_type      = ElementType;
  using size_type       = dash::default_size_t;
  using difference_type = dash::default_index_t;

  using pointer            = dart_gptr_t;
  using const_pointer      = dart_gptr_t const;
  using void_pointer       = dart_gptr_t;
  using const_void_pointer = dart_gptr_t const;

  using local_pointer            = typename allocator_traits::pointer;
  using const_local_pointer      = typename allocator_traits::const_pointer;
  using local_void_pointer       = typename allocator_traits::void_pointer;
  using const_local_void_pointer = typename allocator_traits::const_void_pointer;

  // Dash allocator traits...
  using allocator_policy =
      std::integral_constant<global_allocation_policy, allocation_policy>;

public:
  /**
   * Constructor.
   * Creates a new instance of \c dash::SymmetricAllocator for a given team.
   */
  explicit SymmetricAllocator(
      Team const&    team,
      allocator_type a = {static_cast<LocalMemorySpace*>(
          get_default_memory_space<
              void,
              memory_space_local_domain_tag,
              typename memory_traits::
                  memory_space_type_category>())}) noexcept;

  /**
   * Copy constructor.
   *
   * \see DashAllocatorConcept
   */
  SymmetricAllocator(const self_t& other) noexcept;

  /**
   * Copy Assignment operator.
   *
   * \see DashAllocatorConcept
   */
  self_t& operator=(const self_t& other) noexcept;

  /**
   * Move-constructor.
   * Takes ownership of the moved instance's allocation.
   */
  SymmetricAllocator(self_t&& other) noexcept;

  /**
   * Move-assignment operator.
   */
  self_t& operator=(self_t&& other) noexcept;

  /**
   * Swap two collective Allocators
   */
  void swap(self_t& other) noexcept;

  /**
   * Destructor.
   * Frees all global memory regions allocated by this allocator instance.
   */
  ~SymmetricAllocator() noexcept;

  /**
   * Allocates \c num_local_elem local elements at every unit in global
   * memory space.
   *
   * \note As allocation is collective, each unit has to allocate
   *       an equal number of local elements.
   *
   * \return  Global pointer to allocated memory range, or \c DART_GPTR_NULL
   *          if \c num_local_elem is 0 or less.
   *
   * \see DashAllocatorConcept
   */
  pointer allocate(size_type num_local_elem);

  /**
   * Deallocates memory in global memory space previously allocated across
   * local memory of all units in the team.
   *
   * \note collective operation
   *
   * \see DashAllocatorConcept
   */
  void deallocate(pointer gptr);

private:
  /**
   * Frees all global memory regions allocated by this allocator instance.
   */
  void clear() noexcept;

  /**
   * Deallocates memory in global memory space previously allocated in the
   * active unit's local memory.
   */
  void do_deallocate(
      /// gptr to be deallocated
      pointer gptr,
      /// if true, only free memory but keep gptr in vector
      bool keep_reference);

private:
  // Private Members
  dart_team_t                                       _team_id;
  allocator_type                                    _alloc;
  std::vector<typename policy_type::allocation_rec> _segments;
  policy_type                                       _policy;
};  // class SymmetricAllocator

///////////// Implementation ///////////////////

template <
    typename ElementType,
    global_allocation_policy allocation_policy,
    typename LocalMemorySpace,
    template <class, class> class LocalAlloc>
SymmetricAllocator<
    ElementType,
    allocation_policy,
    LocalMemorySpace,
    LocalAlloc>::
    SymmetricAllocator(Team const& team, allocator_type a) noexcept
  : _team_id(team.dart_id())
  , _alloc(a)
  , _policy{}
{
  DASH_LOG_DEBUG("SymmatricAllocator.SymmetricAllocator(team, alloc) >");
  _segments.reserve(1);
}

template <
    typename ElementType,
    global_allocation_policy allocation_policy,
    typename LocalMemorySpace,
    template <class, class> class LocalAlloc>
SymmetricAllocator<
    ElementType,
    allocation_policy,
    LocalMemorySpace,
    LocalAlloc>::SymmetricAllocator(self_t const& other) noexcept
{
  operator=(other);
}

template <
    typename ElementType,
    global_allocation_policy allocation_policy,
    typename LocalMemorySpace,
    template <class, class> class LocalAlloc>
SymmetricAllocator<
    ElementType,
    allocation_policy,
    LocalMemorySpace,
    LocalAlloc>::SymmetricAllocator(self_t&& other) noexcept
  : _team_id(other._team_id)
  , _alloc(std::move(other._alloc))
  , _segments(std::move(other._segments))
  , _policy(other._policy)
{
}

template <
    typename ElementType,
    global_allocation_policy allocation_policy,
    typename LocalMemorySpace,
    template <class, class> class LocalAlloc>
typename SymmetricAllocator<
    ElementType,
    allocation_policy,
    LocalMemorySpace,
    LocalAlloc>::self_t&
                 SymmetricAllocator<
                     ElementType,
                     allocation_policy,
                     LocalMemorySpace,
                     LocalAlloc>::operator=(self_t const& other) noexcept
{
  if (&other == this) return *this;

  clear();

  _alloc = other._alloc;

  // TODO: Should we really copy the team, or keep the originally team passed
  // to the constructor
  _team_id = other._team_id;

  _segments.reserve(1);

  _policy = other._policy;

  return *this;
}

template <
    typename ElementType,
    global_allocation_policy allocation_policy,
    typename LocalMemorySpace,
    template <class, class> class LocalAlloc>
typename SymmetricAllocator<
    ElementType,
    allocation_policy,
    LocalMemorySpace,
    LocalAlloc>::self_t&
                 SymmetricAllocator<
                     ElementType,
                     allocation_policy,
                     LocalMemorySpace,
                     LocalAlloc>::operator=(self_t&& other) noexcept
{
  if (&other == this) return *this;

  if (_alloc == other._alloc) {
    clear();
    swap(other);
  }
  else {
    operator=(other);  // Copy Assignment
  }

  return *this;
}

template <
    typename ElementType,
    global_allocation_policy allocation_policy,
    typename LocalMemorySpace,
    template <class, class> class LocalAlloc>
SymmetricAllocator<
    ElementType,
    allocation_policy,
    LocalMemorySpace,
    LocalAlloc>::~SymmetricAllocator() noexcept
{
  clear();
}

template <
    typename ElementType,
    global_allocation_policy allocation_policy,
    typename LocalMemorySpace,
    template <class, class> class LocalAlloc>
void SymmetricAllocator<
    ElementType,
    allocation_policy,
    LocalMemorySpace,
    LocalAlloc>::swap(self_t& other) noexcept
{
  std::swap(_team_id, other._team_id);
  std::swap(_alloc, other._alloc);
  std::swap(_segments, other._segments);
  std::swap(_policy, other._policy);
}

template <
    typename ElementType,
    global_allocation_policy allocation_policy,
    typename LocalMemorySpace,
    template <class, class> class LocalAlloc>
typename SymmetricAllocator<
    ElementType,
    allocation_policy,
    LocalMemorySpace,
    LocalAlloc>::pointer
SymmetricAllocator<
    ElementType,
    allocation_policy,
    LocalMemorySpace,
    LocalAlloc>::allocate(size_type num_local_elem)
{
  DASH_LOG_DEBUG(
      "SymmetricAllocator.allocate(nlocal)",
      "number of local values:",
      num_local_elem);

  DASH_ASSERT_EQ(
      _segments.size(), 0, "Number of allocated _segments must be 0");

  auto const rec =
      _policy.do_global_allocate(_team_id, _alloc, num_local_elem);

  auto& lp   = std::get<0>(rec);
  auto& gptr = std::get<2>(rec);

  if (lp == nullptr && DART_GPTR_ISNULL(gptr)) {
    return DART_GPTR_NULL;
  }

  DASH_LOG_TRACE(
      "SymmetricAllocator.allocate(nlocal)",
      "allocated memory global segment (lp, nelem, gptr)",
      lp,
      num_local_elem,
      gptr);

  _segments.push_back(rec);

  DASH_LOG_DEBUG_VAR("SymmetricAllocator.allocate >", gptr);
  return gptr;
}

template <
    typename ElementType,
    global_allocation_policy allocation_policy,
    typename LocalMemorySpace,
    template <class, class> class LocalAlloc>
void SymmetricAllocator<
    ElementType,
    allocation_policy,
    LocalMemorySpace,
    LocalAlloc>::deallocate(pointer gptr)
{
  do_deallocate(gptr, false);
}

template <
    typename ElementType,
    global_allocation_policy allocation_policy,
    typename LocalMemorySpace,
    template <class, class> class LocalAlloc>
void SymmetricAllocator<
    ElementType,
    allocation_policy,
    LocalMemorySpace,
    LocalAlloc>::
    do_deallocate(
        /// gptr to be deallocated
        pointer gptr,
        /// if true, only free memory but keep gptr in vector
        bool keep_reference)
{
  if (_segments.size() == 0) {
    DASH_LOG_ERROR(
        "SymmetricAllocator.deallocate >",
        "cannot free gptr, maybe a double free?",
        gptr);
    return;
  }

  DASH_ASSERT_EQ(
      1,
      _segments.size(),
      "SymmatricAllocator allows only 1 global memory segment");

  if (!dash::is_initialized()) {
    // If a DASH container is deleted after dash::finalize(), global
    // memory has already been freed by dart_exit() and must not be
    // deallocated again.
    DASH_LOG_DEBUG(
        "SymmetricAllocator.deallocate >", "DASH not initialized, abort");
    return;
  }

  auto& tuple = *_segments.begin();

  auto& seg_lptr  = std::get<0>(tuple);
  auto& seg_nelem = std::get<1>(tuple);
  auto& seg_gptr  = std::get<2>(tuple);

  DASH_ASSERT(DART_GPTR_EQUAL(gptr, seg_gptr));

  DASH_LOG_TRACE(
      "SymmetricAllocator.deallocate",
      "deallocating memory segment (lptr, nelem, gptr)",
      seg_lptr,
      seg_nelem,
      seg_gptr);

  auto const ret = _policy.do_global_deallocate(_alloc, tuple);

  if (!keep_reference) {
    _segments.erase(_segments.begin());
  }

  DASH_LOG_DEBUG("SymmetricAllocator.deallocate >");
}

template <
    typename ElementType,
    global_allocation_policy allocation_policy,
    typename LocalMemorySpace,
    template <class, class> class LocalAlloc>
void SymmetricAllocator<
    ElementType,
    allocation_policy,
    LocalMemorySpace,
    LocalAlloc>::clear() noexcept
{
  for (auto& tuple : _segments) {
    do_deallocate(std::get<2>(tuple), true);
  }
  _segments.clear();
}

template <
    class T,
    class U,
    global_allocation_policy allocation_policy,
    class LocalMemSpace,
    template <class, class> class LocalAlloc>
bool operator==(
    const SymmetricAllocator<T, allocation_policy, LocalMemSpace, LocalAlloc>&
        lhs,
    const SymmetricAllocator<U, allocation_policy, LocalMemSpace, LocalAlloc>&
        rhs)
{
  return (
      sizeof(T) == sizeof(U) && lhs._team_id == rhs._team_id &&
      lhs._alloc == rhs._alloc);
}

template <
    class T,
    class U,
    global_allocation_policy allocation_policy,
    class LocalMemSpace,
    template <class, class> class LocalAlloc>
bool operator!=(
    const SymmetricAllocator<T, allocation_policy, LocalMemSpace, LocalAlloc>&
        lhs,
    const SymmetricAllocator<U, allocation_policy, LocalMemSpace, LocalAlloc>&
        rhs)
{
  return !(lhs == rhs);
}

}  // namespace dash

#endif  // DASH__ALLOCATOR__SYMMETRIC_ALLOCATOR_H__INCLUDED
