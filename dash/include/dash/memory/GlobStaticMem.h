#ifndef DASH__MEMORY__GLOB_STATIC_MEMORY_H__INCLUDED
#define DASH__MEMORY__GLOB_STATIC_MEMORY_H__INCLUDED

#include <algorithm>
#include <numeric>

#include <dash/Exception.h>
#include <dash/allocator/AllocationPolicy.h>
#include <dash/memory/MemorySpaceBase.h>
#include <dash/internal/Macro.h>

#include <cpp17/type_traits.h>

// clang-format off

/**
 * \defgroup  DashGlobalMemoryConcept  Global Memory Concept
 * Concept of distributed global memory space shared by units in a specified
 * team.
 *
 * \ingroup DashConcept
 * \{
 * \par Description
 *
 * An abstraction of global memory that provides sequential iteration and
 * random access to local and global elements to units in a specified team.
 * The C++ STL does not specify a counterpart of this concept as it only
 * considers local memory that is implicitly described by the random access
 * pointer interface.
 *
 * The model of global memory represents a single, virtual global address
 * space partitioned into the local memory spaces of its associated units.
 * The global memory concept depends on the allocator concept that specifies
 * allocation of physical memory.
 *
 * Local pointers are usually, but not necessarily represented as raw native
 * pointers as returned by \c malloc.
 *
 * \see DashAllocatorConcept
 *
 * \par Types
 *
 * Type Name            | Description                                            |
 * -------------------- | ------------------------------------------------------ |
 * \c GlobalRAI         | Random access iterator on global address space         |
 * \c LocalRAI          | Random access iterator on a single local address space |
 *
 *
 * \par Methods
 *
 * Return Type          | Method             | Parameters                         | Description                                                                                                |
 * -------------------- | ------------------ | ---------------------------------- | ---------------------------------------------------------------------------------------------------------- |
 * <tt>GlobalRAI</tt>   | <tt>begin</tt>     | &nbsp;                             | Global pointer to the initial address of the global memory space                                           |
 * <tt>GlobalRAI</tt>   | <tt>end</tt>       | &nbsp;                             | Global pointer past the final element in the global memory space                                           |
 * <tt>LocalRAI</tt>    | <tt>lbegin</tt>    | &nbsp;                             | Local pointer to the initial address in the local segment of the global memory space                       |
 * <tt>LocalRAI</tt>    | <tt>lbegin</tt>    | <tt>unit u</tt>                    | Local pointer to the initial address in the local segment at unit \c u of the global memory space          |
 * <tt>LocalRAI</tt>    | <tt>lend</tt>      | &nbsp;                             | Local pointer past the final element in the local segment of the global memory space                       |
 * <tt>LocalRAI</tt>    | <tt>lend</tt>      | <tt>unit u</tt>                    | Local pointer past the final element in the local segment at unit \c u of the global memory space          |
 * <tt>GlobalRAI</tt>   | <tt>at</tt>        | <tt>index gidx</tt>                | Global pointer to the element at canonical global offset \c gidx in the global memory space                |
 * <tt>void</tt>        | <tt>put_value</tt> | <tt>value & v_in, index gidx</tt>  | Stores value specified in parameter \c v_in to address in global memory at canonical global offset \c gidx |
 * <tt>void</tt>        | <tt>get_value</tt> | <tt>value * v_out, index gidx</tt> | Loads value from address in global memory at canonical global offset \c gidx into local address \c v_out   |
 * <tt>void</tt>        | <tt>barrier</tt>   | &nbsp;                             | Blocking synchronization of all units associated with the global memory instance                           |
 *
 * \}
 */

// clang-format on

namespace dash {

/// Forward declarations
template <class MSpaceDomainCategory, class MSpaceTypeCategory>
inline MemorySpace<MSpaceDomainCategory, MSpaceTypeCategory>*
get_default_memory_space();

/**
 * Global memory with address space of static size.
 *
 * For global memory spaces with support for resizing, see
 * \c dash::GlobHeapMem.
 *
 * \see dash::GlobHeapMem
 *
 * \concept{DashMemorySpaceConcept}
 */

template <class LMemSpace>
class GlobStaticMem : public MemorySpace<
                          /// global memory space
                          memory_domain_global,
                          /// The local memory space
                          typename dash::memory_space_traits<
                              LMemSpace>::memory_space_type_category> {
  static constexpr size_t max_align = alignof(max_align_t);

  using memory_traits = dash::memory_space_traits<LMemSpace>;

  using base_t = MemorySpace<
      /// global memory space
      memory_domain_global,
      /// The local memory space
      typename memory_traits::memory_space_type_category>;

  using global_allocation_strategy = dash::allocator::GlobalAllocationPolicy<
      allocation_static,
      synchronization_collective,
      typename memory_traits::memory_space_type_category>;

public:
  using memory_space_domain_category =
      typename base_t::memory_space_domain_category;
  using memory_space_type_category =
      typename base_t::memory_space_type_category;

  using size_type       = dash::default_size_t;
  using index_type      = dash::default_index_t;
  using difference_type = index_type;

  using memory_space_allocation_policy      = allocation_static;
  using memory_space_synchronization_policy = synchronization_collective;
  using memory_space_layout_tag             = memory_space_contiguous;

  using allocator_type = std::pmr::polymorphic_allocator<std::byte>;

  using void_pointer             = dash::GlobPtr<void, GlobStaticMem>;
  using const_void_pointer       = dash::GlobPtr<const void, GlobStaticMem>;
  using local_void_pointer       = void*;
  using const_local_void_pointer = const void*;

public:
  constexpr GlobStaticMem() = default;
  explicit constexpr GlobStaticMem(dash::Team const& team);
  GlobStaticMem(LMemSpace* r, dash::Team const& team);
  ~GlobStaticMem() override = default;

  GlobStaticMem(const GlobStaticMem&) = delete;
  GlobStaticMem& operator=(const GlobStaticMem&) = delete;

  constexpr GlobStaticMem(GlobStaticMem&& other) noexcept {
    *this = std::move(other);
  }

  GlobStaticMem& operator=(GlobStaticMem&&) noexcept = default;

  constexpr size_type capacity() const noexcept;

  constexpr size_type capacity(dash::team_unit_t uid) const noexcept
  {
    return m_local_sizes.at(uid);
  }

  constexpr void_pointer begin() noexcept
  {
    return void_pointer(m_begin);
  }

  constexpr const_void_pointer begin() const noexcept
  {
    return const_void_pointer(m_begin);
  }

  constexpr void_pointer end() noexcept
  {
    auto soon_to_be_end = m_begin;
    // reset local offset to 0
    soon_to_be_end.set_unit(dash::team_unit_t{static_cast<dart_team_unit_t>(m_team->size())});

    return void_pointer(soon_to_be_end);
  }
  constexpr const_void_pointer end() const noexcept
  {
    auto soon_to_be_end = m_begin;
    // reset local offset to 0
    soon_to_be_end.set_unit(
        dash::team_unit_t{static_cast<dart_team_unit_t>(m_team->size())});

    return void_pointer(soon_to_be_end);
  }

  void_pointer allocate(size_type nbytes, size_type alignment = max_align)
  {
    m_local_sizes.resize(m_team->size());
    return do_allocate(nbytes, alignment);
  }

  void deallocate(
      void_pointer gptr, size_type nbytes, size_type alignment = max_align)
  {
    do_deallocate(gptr, nbytes, alignment);
  }

  constexpr dash::Team const& team() const noexcept
  {
    return *m_team;
  }

  constexpr void barrier() const
  {
    m_team->barrier();
  }

  allocator_type get_allocator() const
  {
    // We copy construct an allocator based on the underlying resource of this
    // allocator
    return allocator_type{m_local_allocator.resource()};
  }

  /**
   * Complete all outstanding non-blocking operations to either all units or a
   * specified unit
   */
  constexpr void flush(
      void_pointer ptr, dash::team_unit_t unit = dash::team_unit_t{}) const
  {
    if (unit == dash::team_unit_t{}) {
      dart_flush_all(static_cast<dart_gptr_t>(ptr));
    }
    else {
      auto gptr = static_cast<dart_gptr_t>(ptr);
      // DASH_ASSERT_EQ(gptr.teamid, m_team->dart_id(), "invalid pointer to
      // flush");
      gptr.unitid = unit.id;
      dart_flush(gptr);
    }
  }

  /**
   * Locally complete all outstanding non-blocking operations to all units.
   */
  DASH_CONSTEXPR void flush_local(
      void_pointer ptr, dash::team_unit_t unit = dash::team_unit_t{}) const
  {
    if (unit == dash::team_unit_t{}) {
      dart_flush_local_all(static_cast<dart_gptr_t>(ptr));
    }
    else {
      auto gptr = static_cast<dart_gptr_t>(ptr);
      DASH_ASSERT_EQ(
          gptr.teamid, m_team->dart_id(), "invalid pointer to flush");
      gptr.unitid = unit.id;
      dart_flush_local(gptr);
    }
  }

private:
  /// The team which is necessary for global allocation
  dash::Team const* m_team{};
  /// The local allocator
  allocator_type m_local_allocator{};
  /// The capacities of all units withing the team
  std::vector<size_type> m_local_sizes{};
  /// A global pointer pointing to the first element in the global segment
  void_pointer m_begin{nullptr};

  // global size across all units in bytes
  mutable size_type m_size{std::numeric_limits<size_type>::max()};

private:
  void_pointer do_allocate(size_type nbytes, size_type alignment);

  void do_deallocate(
      void_pointer gptr, size_type nbytes, size_type alignment);
};

///////////// Implementation ///////////////////
//
template <class LMemSpace>
inline constexpr GlobStaticMem<LMemSpace>::GlobStaticMem(dash::Team const& team)
  : GlobStaticMem(nullptr, team)
{
}

template <class LMemSpace>
inline GlobStaticMem<LMemSpace>::GlobStaticMem(
    LMemSpace* r, dash::Team const& team)
  : m_team(&team)
  , m_local_allocator(
        r ? r
          : get_default_memory_space<
                memory_domain_local,
                typename memory_traits::memory_space_type_category>())
  , m_local_sizes(std::max(team.size(), std::size_t(1)))
{
  DASH_LOG_DEBUG("< MemorySpace.MemorySpace");

  DASH_LOG_DEBUG_VAR("MemorySpace.MemorySpace", team);

  DASH_LOG_DEBUG("MemorySpace.MemorySpace >");
}

template <class LMemSpace>
inline typename GlobStaticMem<LMemSpace>::void_pointer
GlobStaticMem<LMemSpace>::do_allocate(size_type nbytes, size_type alignment)
{
  global_allocation_strategy strategy{};
  auto                       gptr = strategy.allocate_segment(
      m_team->dart_id(),
      static_cast<LocalMemorySpaceBase<
          typename memory_traits::memory_space_type_category>*>(
          m_local_allocator.resource()),
      nbytes,
      alignment);

  DASH_ASSERT(!DART_GPTR_ISNULL(gptr));
  DASH_ASSERT_EQ(m_team->size(), m_local_sizes.size(), "invalid setting");

  m_begin = static_cast<void_pointer>(gptr);

  DASH_ASSERT_RETURNS(
      dart_allgather(
          // source buffer
          &nbytes,
          // target buffer
          m_local_sizes.data(),
          // nels
          1,
          // dart type
          dash::dart_datatype<size_type>::value,
          // dart team
          m_team->dart_id()),
      DART_OK);

  return void_pointer(gptr);
}

template <class LMemSpace>
inline void GlobStaticMem<LMemSpace>::do_deallocate(
    void_pointer gptr, size_type nbytes, size_type alignment)
{
  DASH_LOG_DEBUG("< MemorySpace.do_deallocate");

  if (*m_team != dash::Team::Null()) {
    DASH_ASSERT_RETURNS(dart_barrier(m_team->dart_id()), DART_OK);

    auto soon_to_be_lbegin = gptr;
    gptr.set_unit(m_team->myid());

    auto* lbegin = gptr.local();

    global_allocation_strategy strategy{};
    strategy.deallocate_segment(
        static_cast<dart_gptr_t>(gptr),
        static_cast<LocalMemorySpaceBase<
            typename memory_traits::memory_space_type_category>*>(
            m_local_allocator.resource()),
        lbegin,
        nbytes,
        alignment);
  }

  m_begin = void_pointer{};
  m_size = 0;

  DASH_LOG_DEBUG("MemorySpace.do_deallocate >");
}

template <class LMemSpace>
constexpr inline typename GlobStaticMem<LMemSpace>::size_type
GlobStaticMem<LMemSpace>::capacity() const noexcept
{
  if (m_size == std::numeric_limits<size_type>::max()) {
    m_size = std::accumulate(
        std::begin(m_local_sizes),
        std::end(m_local_sizes),
        0,
        std::plus<size_type>());
  }
  return m_size;
}

}  // namespace dash

#endif
