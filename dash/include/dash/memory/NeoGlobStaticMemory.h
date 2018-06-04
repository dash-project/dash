#ifndef DASH__MEMORY__GLOB_STATIC_MEMORY_H__INCLUDED
#define DASH__MEMORY__GLOB_STATIC_MEMORY_H__INCLUDED

#include <dash/Exception.h>
#include <dash/allocator/AllocationPolicy.h>
#include <dash/memory/MemorySpaceBase.h>

#include <numeric>

namespace dash {

/// Forward declarations
template <class MSpaceDomainCategory, class MSpaceTypeCategory>
inline MemorySpace<MSpaceDomainCategory, MSpaceTypeCategory>*
get_default_memory_space();

template <class ElementType, class LMemSpace, class SynchronizationPolicy>
class MemorySpace<
    /// global memory space
    memory_domain_global,
    /// Element Type
    ElementType,
    /// We are allowed to allocate only once
    allocation_static,
    /// either collective or single synchronization policy
    SynchronizationPolicy,
    /// The local memory space
    LMemSpace>
  : GlobalMemorySpaceBase<
        ElementType,
        allocation_static,
        SynchronizationPolicy,
        LMemSpace> {
  using memory_traits = dash::memory_space_traits<LMemSpace>;

  using base_t = GlobalMemorySpaceBase<
      ElementType,
      allocation_static,
      SynchronizationPolicy,
      LMemSpace>;

  // TODO rko: Move this to base class
  using allocation_policy_t = dash::allocator::GlobalAllocationPolicy<
      ElementType,
      typename base_t::memory_space_allocation_policy,
      typename base_t::memory_space_synchronization_policy,
      typename memory_traits::memory_space_type_category>;

public:
  using value_type      = typename base_t::value_type;
  using size_type       = typename base_t::size_type;
  using index_type      = typename base_t::index_type;
  using difference_type = typename base_t::difference_type;

  // TODO rko: Move this to base class
  using allocator_type = cpp17::pmr::polymorphic_allocator<ElementType>;

  using pointer = dash::GlobPtr<typename base_t::value_type, MemorySpace>;
  using const_pointer = dash::GlobPtr<const value_type, MemorySpace>;
  using local_pointer = value_type *;
  using const_local_pointer = value_type const *;

public:
  MemorySpace() = delete;

  explicit MemorySpace(
      size_type nels, dash::Team const& team = dash::Team::All());
  MemorySpace(size_type nels, LMemSpace* r, dash::Team const& team);
  ~MemorySpace() override;

  MemorySpace(const MemorySpace&) = delete;
  MemorySpace(MemorySpace&&);

  MemorySpace& operator=(const MemorySpace&) = delete;
  MemorySpace& operator                      =(MemorySpace&&);

  bool do_is_equal(base_t const&) const noexcept override;

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
  dash::Team const& team() const noexcept
  {
    return *m_team;
  }

  void barrier()
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
  dash::Team const*      m_team{};
  allocator_type         m_allocator{};
  allocation_policy_t    m_policy{};
  std::vector<size_type> m_local_sizes{};
  // Uniform Initialization does not work in G++ 4.9.x
  // This is why we have to use copy assignment
  dart_gptr_t            m_begin = DART_GPTR_NULL;
  local_pointer          m_lbegin{nullptr};
  local_pointer          m_lend{nullptr};

private:
  void do_allocate(size_type nels);
  void do_deallocate();
};

///////////// Implementation ///////////////////
//
template <class ElementType, class LMemSpace, class SynchronizationPolicy>
inline MemorySpace<
    memory_domain_global,
    ElementType,
    allocation_static,
    SynchronizationPolicy,
    LMemSpace>::MemorySpace(size_type nels, dash::Team const& team)
  : m_team(&team)
  , m_allocator(get_default_memory_space<
                memory_domain_local,
                typename memory_traits::memory_space_type_category>())
  , m_local_sizes(team.size())
{
  DASH_LOG_DEBUG("< MemorySpace.MemorySpace");
  DASH_LOG_DEBUG_VAR("MemorySpace.MemorySpace", team);
  DASH_LOG_DEBUG_VAR("MemorySpace.MemorySpace", nels);

  do_allocate(nels);

  DASH_LOG_DEBUG("MemorySpace.MemorySpace >");
}

template <class ElementType, class LMemSpace, class SynchronizationPolicy>
inline MemorySpace<
    memory_domain_global,
    ElementType,
    allocation_static,
    SynchronizationPolicy,
    LMemSpace>::
    MemorySpace(size_type nels, LMemSpace* r, dash::Team const& team)
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

  do_allocate(nels);

  DASH_LOG_DEBUG("MemorySpace.MemorySpace >");
}

template <class ElementType, class LMemSpace, class SynchronizationPolicy>
inline MemorySpace<
    memory_domain_global,
    ElementType,
    allocation_static,
    SynchronizationPolicy,
    LMemSpace>::MemorySpace(MemorySpace&& other)
  : m_team(std::move(other.m_team))
  , m_allocator(std::move(other.m_allocator))
  , m_policy(std::move(other.m_policy))
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

template <class ElementType, class LMemSpace, class SynchronizationPolicy>
inline MemorySpace<
    memory_domain_global,
    ElementType,
    allocation_static,
    SynchronizationPolicy,
    LMemSpace>&
                MemorySpace<
                    memory_domain_global,
                    ElementType,
                    allocation_static,
                    SynchronizationPolicy,
                    LMemSpace>::operator=(MemorySpace&& other)
{
  // deallocate own memory
  do_deallocate();
  // and swap..
  std::swap(m_team, other.m_team);
  std::swap(m_allocator, other.m_allocator);
  std::swap(m_policy, other.m_policy);
  std::swap(m_local_sizes, other.m_local_sizes);
  std::swap(m_begin, other.m_begin);
  std::swap(m_lbegin, other.m_lbegin);
  std::swap(m_lend, other.m_lend);

  return *this;
}

template <class ElementType, class LMemSpace, class SynchronizationPolicy>
inline void MemorySpace<
    memory_domain_global,
    ElementType,
    allocation_static,
    SynchronizationPolicy,
    LMemSpace>::do_allocate(size_type nels)
{
  auto alloc_rec = m_policy.allocate_segment(
      m_team->dart_id(),
      static_cast<LocalMemorySpaceBase<memory_space_host_tag>*>(
          m_allocator.resource()),
      nels);

  m_begin = alloc_rec.second;

  DASH_ASSERT_MSG(
      !DART_GPTR_ISNULL(m_begin), "global memory allocation failed");

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
          // dart teamk
          m_team->dart_id()),
      DART_OK);

  m_lbegin = static_cast<local_pointer>(alloc_rec.first);
  m_lend   = std::next(m_lbegin, nels);
}

template <class ElementType, class LMemSpace, class SynchronizationPolicy>
inline MemorySpace<
    memory_domain_global,
    ElementType,
    allocation_static,
    SynchronizationPolicy,
    LMemSpace>::~MemorySpace()
{
  DASH_LOG_DEBUG("< MemorySpace.~MemorySpace");

  do_deallocate();

  DASH_LOG_DEBUG("MemorySpace.~MemorySpace >");
}

template <class ElementType, class LMemSpace, class SynchronizationPolicy>
inline void MemorySpace<
    memory_domain_global,
    ElementType,
    allocation_static,
    SynchronizationPolicy,
    LMemSpace>::do_deallocate()
{
  DASH_LOG_DEBUG("< MemorySpace.do_deallocate");

  DASH_LOG_DEBUG_VAR("GlobStaticMemory.do_deallocate", m_lbegin);
  DASH_LOG_DEBUG_VAR("GlobStaticMemory.do_deallocate", m_lend);
  DASH_LOG_DEBUG_VAR("GlobStaticMemory.do_deallocate", m_begin);
  DASH_LOG_DEBUG_VAR("GlobStaticMemory.do_deallocate", m_local_sizes.size());

  if (*m_team != dash::Team::Null() && !DART_GPTR_ISNULL(m_begin)) {
    m_policy.deallocate_segment(
        m_begin,
        static_cast<LocalMemorySpaceBase<memory_space_host_tag>*>(
            m_allocator.resource()),
        m_lbegin,
        m_local_sizes.at(m_team->myid()));
  }

  m_begin  = DART_GPTR_NULL;
  m_lbegin = nullptr;
  m_lend   = nullptr;
  m_local_sizes.clear();

  DASH_LOG_DEBUG("MemorySpace.do_deallocate >");
}

template <class ElementType, class LMemSpace, class SynchronizationPolicy>
inline typename MemorySpace<
    memory_domain_global,
    ElementType,
    allocation_static,
    SynchronizationPolicy,
    LMemSpace>::size_type
MemorySpace<
    memory_domain_global,
    ElementType,
    allocation_static,
    SynchronizationPolicy,
    LMemSpace>::size() const noexcept
{
  return std::accumulate(
      std::begin(m_local_sizes),
      std::end(m_local_sizes),
      0,
      std::plus<size_type>());
}

template <class ElementType, class LMemSpace, class SynchronizationPolicy>
inline bool MemorySpace<
    memory_domain_global,
    ElementType,
    allocation_static,
    SynchronizationPolicy,
    LMemSpace>::do_is_equal(base_t const& other) const noexcept
{
  return true;
}

}  // namespace dash

#endif
