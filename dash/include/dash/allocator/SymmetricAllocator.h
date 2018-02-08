#ifndef DASH__ALLOCATOR__SYMMETRIC_ALLOCATOR_H__INCLUDED
#define DASH__ALLOCATOR__SYMMETRIC_ALLOCATOR_H__INCLUDED

#include <dash/dart/if/dart.h>

#include <dash/Team.h>
#include <dash/Types.h>

#include <dash/memory/MemorySpace.h>

#include <dash/internal/Logging.h>
#include <dash/internal/StreamConversion.h>

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
template <typename ElementType, typename LocalMemorySpace>
class SymmetricAllocator {
  template <class T, class U>
  friend bool operator==(
      const SymmetricAllocator<T, LocalMemorySpace>& lhs,
      const SymmetricAllocator<U, LocalMemorySpace>& rhs);

  template <class T, class U>
  friend bool operator!=(
      const SymmetricAllocator<T, LocalMemorySpace>& lhs,
      const SymmetricAllocator<U, LocalMemorySpace>& rhs);

  using memory_traits = typename dash::memory_space_traits<LocalMemorySpace>;

  static_assert(memory_traits::is_local::value, "memory space must be local");

  static_assert(
      std::is_base_of<cpp17::pmr::memory_resource, LocalMemorySpace>::value,
      "Local Memory Space must be a polymorphic memory resource");

  static constexpr size_t const alignment = alignof(ElementType);

  using self_t           = SymmetricAllocator<ElementType, LocalMemorySpace>;
  using allocator_type   = cpp17::pmr::polymorphic_allocator<byte>;
  using allocator_traits = std::allocator_traits<allocator_type>;

public:
  /// Allocator Traits
  using value_type      = ElementType;
  using size_type       = dash::default_size_t;
  using difference_type = dash::default_index_t;

  using pointer            = dart_gptr_t;
  using const_pointer      = dart_gptr_t const;
  using void_pointer       = dart_gptr_t;
  using const_void_pointer = dart_gptr_t const;

  using local_pointer            = value_type*;
  using const_local_pointer      = value_type const*;
  using local_void_pointer       = void*;
  using const_local_void_pointer = void const*;

  // Dash allocator traits...
  using allocator_category = dash::collective_allocator_tag;

private:
  // tuple to hold the local pointer
  using allocation_rec = std::tuple<local_pointer, size_t, pointer>;

private:
  // Private Members
  dart_team_t                 _team_id = DART_TEAM_NULL;
  allocator_type              _alloc;
  std::vector<allocation_rec> _segments;

public:
  /**
   * Constructor.
   * Creates a new instance of \c dash::SymmetricAllocator for a given team.
   */
  explicit SymmetricAllocator(
      Team&          team,
      allocator_type a = {get_default_memory_space<
          void,
          typename memory_traits::memory_space_domain_category,
          typename memory_traits::memory_space_type_category>()}) noexcept;

  /**
   * Move-constructor.
   * Takes ownership of the moved instance's allocation.
   */
  SymmetricAllocator(self_t&& other) = delete;

  /**
   * Copy constructor.
   *
   * \see DashAllocatorConcept
   */
  SymmetricAllocator(const self_t& other) = delete;

  /**
   * Copy Assignment operator.
   *
   * \see DashAllocatorConcept
   */
  self_t& operator=(const self_t& other) = delete;

  /**
   * Move-assignment operator.
   */
  self_t& operator=(self_t&& other) = delete;

  /**
   * Destructor.
   * Frees all global memory regions allocated by this allocator instance.
   */
  ~SymmetricAllocator() noexcept
  {
    clear();
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
  bool operator==(const self_t& rhs) const noexcept
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
  bool operator!=(const self_t& rhs) const noexcept
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
  pointer allocate(size_type num_local_elem);

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
    for (auto& tuple : _segments) {
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
      bool keep_reference = false);

};  // class SymmetricAllocator

///////////// Implementation ///////////////////

template <typename ElementType, typename LocalMemorySpace>
SymmetricAllocator<ElementType, LocalMemorySpace>::SymmetricAllocator(
    Team& team, allocator_type a) noexcept
  : _team_id(team.dart_id())
  , _alloc(a)
{
  _segments.reserve(1);
}

template <typename ElementType, typename LocalMemorySpace>
typename SymmetricAllocator<ElementType, LocalMemorySpace>::pointer
SymmetricAllocator<ElementType, LocalMemorySpace>::allocate(
    size_type num_local_elem)
{
  DASH_LOG_DEBUG(
      "SymmetricAllocator.allocate(nlocal)",
      "number of local values:", num_local_elem);

  DASH_ASSERT_EQ(
      _segments.size(), 0, "Number of allocated _segments must be 0");

  local_pointer lp =
      reinterpret_cast<local_pointer>(allocator_traits::allocate(
          _alloc, num_local_elem * sizeof(value_type)));

  if (!lp && num_local_elem > 0) {
    std::stringstream ss;
    ss << "Allocating local segment (nelem: " << num_local_elem
       << ") failed!";
    DASH_LOG_ERROR("SymmetricAllocator.allocate", ss.str());
    DASH_THROW(dash::exception::RuntimeError, ss.str());
  }

  pointer                         gptr;
  dash::dart_storage<ElementType> ds(num_local_elem);

  if (dart_team_memregister(_team_id, ds.nelem, ds.dtype, lp, &gptr) !=
      DART_OK) {
    gptr = DART_GPTR_NULL;
    allocator_traits::deallocate(
        _alloc, reinterpret_cast<allocator_traits::pointer>(lp),
        num_local_elem * sizeof(value_type));

    DASH_LOG_ERROR(
        "SymmetricAllocator.attach", "cannot attach local memory", gptr);
  }
  else {
    DASH_LOG_TRACE(
        "SymmetricAllocator.allocate(nlocal)",
        "allocated memory segment (lp, nelem, gptr)", lp, num_local_elem,
        gptr);
    _segments.push_back(std::make_tuple(lp, num_local_elem, gptr));
  }
  DASH_LOG_DEBUG_VAR("SymmetricAllocator.allocate >", gptr);
  return gptr;
}

template <typename ElementType, typename LocalMemorySpace>
void SymmetricAllocator<ElementType, LocalMemorySpace>::_deallocate(
    /// gptr to be deallocated
    pointer gptr,
    /// if true, only free memory but keep gptr in vector
    bool keep_reference)
{
  DASH_ASSERT_RANGE(
      0, _segments.size(), 1,
      "SymmatricAllocator supports only 1 or 0 memory _segments");

  if (!dash::is_initialized()) {
    // If a DASH container is deleted after dash::finalize(), global
    // memory has already been freed by dart_exit() and must not be
    // deallocated again.
    DASH_LOG_DEBUG(
        "SymmetricAllocator.deallocate >", "DASH not initialized, abort");
    return;
  }

  if (_segments.size() == 0) {
    DASH_LOG_WARN(
        "SymmetricAllocator.deallocate >", "cannot free gptr", gptr);
    return;
  }

  auto& tuple = *_segments.begin();

  auto& seg_lptr  = std::get<0>(tuple);
  auto& seg_nelem = std::get<1>(tuple);
  auto& seg_gptr  = std::get<2>(tuple);

  DASH_ASSERT(DART_GPTR_EQUAL(gptr, seg_gptr));

  DASH_LOG_TRACE(
      "SymmetricAllocator.deallocate",
      "allocating memory segment (lptr, nelem, gptr)", seg_lptr, seg_nelem,
      seg_gptr);

  DASH_LOG_DEBUG("SymmetricAllocator.deallocate", "dart_team_memderegister");
  DASH_ASSERT_RETURNS(dart_team_memderegister(gptr), DART_OK);
  DASH_LOG_DEBUG("SymmetricAllocator.deallocate", "barrier");
  DASH_ASSERT_RETURNS(dart_barrier(_team_id), DART_OK);

  DASH_LOG_DEBUG("SymmetricAllocator.deallocate", "_segments.erase");
  allocator_traits::deallocate(
      _alloc, reinterpret_cast<allocator_traits::pointer>(seg_lptr),
      seg_nelem * sizeof(value_type));
  seg_lptr = nullptr;

  if (!keep_reference) {
    _segments.erase(_segments.begin());
  }

  DASH_LOG_DEBUG("SymmetricAllocator.deallocate >");
}

template <class T, class U, class LocalMemSpace>
bool operator==(
    const SymmetricAllocator<T, LocalMemSpace>& lhs,
    const SymmetricAllocator<U, LocalMemSpace>& rhs)
{
  return (sizeof(T) == sizeof(U) && lhs._team_id == rhs._team_id);
}

template <class T, class U, class LocalMemSpace>
bool operator!=(
    const SymmetricAllocator<T, LocalMemSpace>& lhs,
    const SymmetricAllocator<U, LocalMemSpace>& rhs)
{
  return !(lhs == rhs);
}

}  // namespace dash

#endif  // DASH__ALLOCATOR__SYMMETRIC_ALLOCATOR_H__INCLUDED
