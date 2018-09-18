#ifndef DASH__MEMORY__GLOB_STATIC_MEMORY_H__INCLUDED
#define DASH__MEMORY__GLOB_STATIC_MEMORY_H__INCLUDED

#include <dash/Exception.h>
#include <dash/allocator/AllocationPolicy.h>
#include <dash/memory/MemorySpaceBase.h>

#include <numeric>

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

template <class ElementType, class LMemSpace>
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

  using value_type = typename std::remove_cv<
      typename std::remove_reference<ElementType>::type>::type;

  using size_type       = dash::default_size_t;
  using index_type      = dash::default_index_t;
  using difference_type = index_type;

  using memory_space_allocation_policy      = allocation_static;
  using memory_space_synchronization_policy = synchronization_collective;
  using memory_space_layout_tag             = memory_space_contiguous;

  using allocator_type = cpp17::pmr::polymorphic_allocator<ElementType>;

  using pointer             = dash::GlobPtr<value_type, GlobStaticMem>;
  using const_pointer       = dash::GlobPtr<const value_type, GlobStaticMem>;
  using local_pointer       = value_type*;
  using const_local_pointer = value_type const*;

public:
  GlobStaticMem() = delete;

  explicit GlobStaticMem(
      size_type nels, dash::Team const& team = dash::Team::All());
  GlobStaticMem(size_type nels, LMemSpace* r, dash::Team const& team);
  ~GlobStaticMem() override;

  GlobStaticMem(const GlobStaticMem&) = delete;
  GlobStaticMem(GlobStaticMem&&);

  GlobStaticMem& operator=(const GlobStaticMem&) = delete;
  GlobStaticMem& operator                        =(GlobStaticMem&&);

  size_type size() const noexcept;

  size_type local_size(dash::team_unit_t uid) const noexcept
  {
    return m_local_sizes.at(uid);
  }

  local_pointer lbegin() noexcept
  {
    return m_lbegin;
  }
  const_local_pointer lbegin() const noexcept
  {
    return m_lbegin;
  }

  local_pointer lend() noexcept
  {
    return m_lend;
  }

  const_local_pointer lend() const noexcept
  {
    return m_lend;
  }

  pointer begin() noexcept
  {
    return pointer(*this, m_begin);
  }
  const_pointer begin() const noexcept
  {
    return const_pointer(*this, m_begin);
  }

  pointer end() noexcept
  {
    auto soon_to_be_end = m_begin;
    // unitId points one past the last unit
    dart_gptr_setunit(&soon_to_be_end, m_team->size());
    // reset local offset to 0
    soon_to_be_end.addr_or_offs.offset = 0;

    return pointer(*this, soon_to_be_end);
  }
  const_pointer end() const noexcept
  {
    auto soon_to_be_end = m_begin;
    // unitId points one past the last unit
    dart_gptr_setunit(&soon_to_be_end, m_team->size());
    // reset local offset to 0
    soon_to_be_end.addr_or_offs.offset = 0;

    return const_pointer(*this, soon_to_be_end);
  }

  pointer allocate(size_type nels, size_type alignment = alignof(value_type))
  {
    return do_allocate(nels * sizeof(value_type), alignment);
  }
  void deallocate(
      pointer gptr,
      size_type /*nels*/,
      size_type alignment = alignof(value_type))
  {
    DASH_ASSERT_MSG(
        DART_GPTR_EQUAL(gptr.dart_gptr(), m_begin),
        "invalid pointer to deallocate");

    do_deallocate(
        gptr,
        m_local_sizes.at(m_team->myid()) * sizeof(value_type),
        alignment);
  }

  dash::Team const& team() const noexcept
  {
    return *m_team;
  }

  void barrier() const
  {
    m_team->barrier();
  }

  allocator_type get_allocator() const
  {
    // We copy construct an allocator based on the underlying resource of this
    // allocator
    return allocator_type{m_allocator.resource()};
  }

  /**
   * Complete all outstanding non-blocking operations to all units.
   */
  void flush() noexcept
  {
    dart_flush_all(m_begin);
  }

  /**
   * Complete all outstanding non-blocking operations to the specified unit.
   */
  void flush(dash::team_unit_t target) noexcept
  {
    dart_gptr_t gptr = m_begin;
    gptr.unitid      = target.id;
    dart_flush(gptr);
  }

  /**
   * Locally complete all outstanding non-blocking operations to all units.
   */
  void flush_local() noexcept
  {
    dart_flush_local_all(m_begin);
  }

  /**
   * Locally complete all outstanding non-blocking operations to the specified
   * unit.
   */
  void flush_local(dash::team_unit_t target) noexcept
  {
    dart_gptr_t gptr = m_begin;
    gptr.unitid      = target.id;
    dart_flush_local(gptr);
  }

  /**
   * Resolve the global pointer from an element position in a unit's
   * local memory.
   */
  template <typename IndexType>
  pointer at(
      /// The unit id
      team_unit_t unit,
      /// The unit's local address offset
      IndexType local_index) const
  {
    DASH_LOG_DEBUG("MemorySpace.at(unit,l_idx)", unit, local_index);
    if (m_team->size() == 0 || DART_GPTR_ISNULL(m_begin)) {
      DASH_LOG_DEBUG(
          "MemorySpace.at(unit,l_idx) >", "global memory not allocated");
      return pointer(nullptr);
    }
    // Initialize with global pointer to start address:
    dart_gptr_t gptr = m_begin;
    // Resolve global unit id
    DASH_LOG_TRACE_VAR("MemorySpace.at (=g_begptr)", gptr);
    DASH_LOG_TRACE_VAR("MemorySpace.at", gptr.unitid);
    team_unit_t lunit{gptr.unitid};
    DASH_ASSERT_EQ(gptr.unitid, 0, "invalid global begin pointer");
    DASH_ASSERT_RANGE(
        0, lunit.id, m_team->size() - 1, "invalid global begin pointer");
    DASH_LOG_TRACE_VAR("MemorySpace.at", lunit);
    // lunit = (lunit + unit) % _team->size();
    lunit = lunit + unit;

    DASH_LOG_TRACE_VAR("MemorySpace.at", lunit);
    // Apply global unit to global pointer:
    dart_gptr_setunit(&gptr, lunit);
    // increment locally only
    gptr.addr_or_offs.offset += local_index * sizeof(value_type);
    // Apply local offset to global pointer:
    pointer res_gptr(*this, gptr);
    DASH_LOG_DEBUG("MemorySpace.at (+g_unit) >", res_gptr);
    return res_gptr;
  }

private:
  dash::Team const*          m_team{};
  allocator_type             m_allocator{};
  global_allocation_strategy m_allocation_policy{};
  std::vector<size_type>     m_local_sizes{};
  // Uniform Initialization does not work in G++ 4.9.x
  // This is why we have to use C-Style Initializer Initialization
  dart_gptr_t       m_begin = DART_GPTR_NULL;
  local_pointer     m_lbegin{nullptr};
  local_pointer     m_lend{nullptr};
  mutable size_type m_size{std::numeric_limits<size_type>::max()};

private:
  pointer do_allocate(size_type nbytes, size_type alignment);
  void    do_deallocate(pointer gptr, size_type nbytes, size_type alignment);
};

///////////// Implementation ///////////////////
//
template <class ElementType, class LMemSpace>
inline GlobStaticMem<ElementType, LMemSpace>::GlobStaticMem(
    size_type nels, dash::Team const& team)
  : m_team(&team)
  , m_allocator(get_default_memory_space<
                memory_domain_local,
                typename memory_traits::memory_space_type_category>())
  , m_local_sizes(team.size())
{
  DASH_LOG_DEBUG("< MemorySpace.MemorySpace");
  DASH_LOG_DEBUG_VAR("MemorySpace.MemorySpace", team);
  DASH_LOG_DEBUG_VAR("MemorySpace.MemorySpace", nels);

  do_allocate(nels * sizeof(value_type), alignof(value_type));

  DASH_LOG_DEBUG("MemorySpace.MemorySpace >");
}

template <class ElementType, class LMemSpace>
inline GlobStaticMem<ElementType, LMemSpace>::GlobStaticMem(
    size_type nels, LMemSpace* r, dash::Team const& team)
  : m_team(&team)
  , m_allocator(
        r ? r
          : get_default_memory_space<
                memory_domain_local,
                typename memory_traits::memory_space_type_category>())
  , m_local_sizes(team.size())
{
  DASH_LOG_DEBUG("< MemorySpace.MemorySpace");

  DASH_LOG_DEBUG_VAR("MemorySpace.MemorySpace", nels);
  DASH_LOG_DEBUG_VAR("MemorySpace.MemorySpace", team);

  do_allocate(nels * sizeof(value_type), alignof(value_type));

  DASH_LOG_DEBUG("MemorySpace.MemorySpace >");
}

template <class ElementType, class LMemSpace>
inline GlobStaticMem<ElementType, LMemSpace>::GlobStaticMem(
    GlobStaticMem&& other)
  : m_team(std::move(other.m_team))
  , m_allocator(std::move(other.m_allocator))
  , m_allocation_policy(std::move(other.m_allocation_policy))
  , m_local_sizes(std::move(other.m_local_sizes))
  , m_begin(std::move(other.m_begin))
  , m_lbegin(std::move(other.m_lbegin))
  , m_lend(std::move(other.m_lend))
{
  other.m_team   = &dash::Team::Null();
  other.m_begin  = DART_GPTR_NULL;
  other.m_lbegin = nullptr;
  other.m_lend   = nullptr;
}

template <class ElementType, class LMemSpace>
inline GlobStaticMem<ElementType, LMemSpace>&
GlobStaticMem<ElementType, LMemSpace>::operator=(GlobStaticMem&& other)
{
  // deallocate own memory
  if (!DART_GPTR_ISNULL(m_begin)) {
    do_deallocate(
        begin(),
        m_local_sizes.at(m_team->myid()) * sizeof(value_type),
        alignof(value_type));
  }
  // and swap..
  std::swap(m_team, other.m_team);
  std::swap(m_allocator, other.m_allocator);
  std::swap(m_allocation_policy, other.m_allocation_policy);
  std::swap(m_local_sizes, other.m_local_sizes);
  std::swap(m_begin, other.m_begin);
  std::swap(m_lbegin, other.m_lbegin);
  std::swap(m_lend, other.m_lend);

  return *this;
}

template <class ElementType, class LMemSpace>
inline typename GlobStaticMem<ElementType, LMemSpace>::pointer
GlobStaticMem<ElementType, LMemSpace>::do_allocate(
    size_type nbytes, size_type alignment)
{
  if (!DART_GPTR_ISNULL(m_begin)) {
    DASH_THROW(
        dash::exception::RuntimeError,
        "only one allocated segment is allowed");
  }

  auto alloc_rec = m_allocation_policy.allocate_segment(
      m_team->dart_id(),
      static_cast<LocalMemorySpaceBase<
          typename memory_traits::memory_space_type_category>*>(
          m_allocator.resource()),
      nbytes,
      alignment);

  m_begin = alloc_rec.second;

  DASH_ASSERT_MSG(
      !DART_GPTR_ISNULL(m_begin), "global memory allocation failed");

  auto const nels = nbytes / sizeof(value_type);

  DASH_ASSERT_RETURNS(
      dart_allgather(
          // source buffer
          &nels,
          // target buffer
          m_local_sizes.data(),
          // nels
          1,
          // dart type
          dash::dart_datatype<size_type>::value,
          // dart team
          m_team->dart_id()),
      DART_OK);

  m_lbegin = static_cast<local_pointer>(alloc_rec.first);
  m_lend   = std::next(m_lbegin, nels);

  return pointer(*this, m_begin);
}

template <class ElementType, class LMemSpace>
inline GlobStaticMem<ElementType, LMemSpace>::~GlobStaticMem()
{
  DASH_LOG_DEBUG("< MemorySpace.~MemorySpace");

  if (!DART_GPTR_ISNULL(m_begin)) {
    do_deallocate(
        begin(),
        m_local_sizes.at(m_team->myid()) * sizeof(value_type),
        alignof(value_type));
  }

  DASH_LOG_DEBUG("MemorySpace.~MemorySpace >");
}

template <class ElementType, class LMemSpace>
inline void GlobStaticMem<ElementType, LMemSpace>::do_deallocate(
    pointer gptr, size_type nbytes, size_type alignment)
{
  DASH_LOG_DEBUG("< MemorySpace.do_deallocate");

  DASH_ASSERT_MSG(
      DART_GPTR_EQUAL(gptr.dart_gptr(), m_begin),
      "Invalid global pointer to deallocate");

  DASH_LOG_DEBUG_VAR("GlobStaticMemory.do_deallocate", m_lbegin);
  DASH_LOG_DEBUG_VAR("GlobStaticMemory.do_deallocate", m_lend);
  DASH_LOG_DEBUG_VAR("GlobStaticMemory.do_deallocate", m_begin);
  DASH_LOG_DEBUG_VAR("GlobStaticMemory.do_deallocate", m_local_sizes.size());

  if (*m_team != dash::Team::Null() && !DART_GPTR_ISNULL(m_begin)) {
    DASH_ASSERT_RETURNS(dart_barrier(m_team->dart_id()), DART_OK);

    m_allocation_policy.deallocate_segment(
        m_begin,
        static_cast<LocalMemorySpaceBase<
            typename memory_traits::memory_space_type_category>*>(
            m_allocator.resource()),
        m_lbegin,
        nbytes,
        alignment);
  }

  m_begin  = DART_GPTR_NULL;
  m_lbegin = nullptr;
  m_lend   = nullptr;
  m_local_sizes.clear();

  DASH_LOG_DEBUG("MemorySpace.do_deallocate >");
}

template <class ElementType, class LMemSpace>
inline typename GlobStaticMem<ElementType, LMemSpace>::size_type
GlobStaticMem<ElementType, LMemSpace>::size() const noexcept
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
