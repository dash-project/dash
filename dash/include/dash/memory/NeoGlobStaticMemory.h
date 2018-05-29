#ifndef DASH__GLOB_STATIC_MEMORY_H__INCLUDED
#define DASH__GLOB_STATIC_MEMORY_H__INCLUDED

#include <dash/allocator/AllocationPolicy.h>
#include <dash/memory/MemorySpaceBase.h>

#include <numeric>

namespace dash {

/// Forward declarations
template <class MSpaceDomainCategory, class MSpaceTypeCategory>
inline MemorySpace<MSpaceDomainCategory, MSpaceTypeCategory>*
get_default_memory_space();

namespace experimental {
template <class ElementType, class LMemSpace, class SynchronizationPolicy>
class GlobalStaticMemory
  : public MemorySpace<
        /// global memory space
        memory_domain_global,
        /// Element Type
        ElementType,
        /// We are allowed to allocate only once
        allocation_static,
        /// All Units participate in global memory allocation
        SynchronizationPolicy,
        /// The local memory space
        LMemSpace> {
  using memory_traits = dash::memory_space_traits<LMemSpace>;

  using base_t = MemorySpace<
      memory_domain_global,
      ElementType,
      allocation_static,
      SynchronizationPolicy,
      LMemSpace>;

  static constexpr size_t alignment = alignof(ElementType);

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

  using allocator_type = cpp17::pmr::polymorphic_allocator<byte>;

  using pointer =
      dash::GlobPtr<typename base_t::value_type, GlobalStaticMemory>;
  using const_pointer = dash::GlobPtr<const value_type, GlobalStaticMemory>;
  using local_pointer = typename std::add_pointer<value_type>::type;

public:
  GlobalStaticMemory() = delete;

  explicit GlobalStaticMemory(
      size_type nels, dash::Team const& team = dash::Team::All());
  GlobalStaticMemory(size_type nels, LMemSpace* r, dash::Team const& team);
  ~GlobalStaticMemory() override;

  GlobalStaticMemory(const GlobalStaticMemory&) = delete;
  GlobalStaticMemory(GlobalStaticMemory&&);

  GlobalStaticMemory& operator=(const GlobalStaticMemory&) = delete;
  GlobalStaticMemory& operator=(GlobalStaticMemory&&);

  bool do_is_equal(typename base_t::global_memory_space_base_t const&) const
      noexcept override;

  size_type     local_size(dash::team_unit_t) const noexcept;
  size_type     size() const noexcept;
  local_pointer lbegin() noexcept;
  local_pointer lend() const noexcept;
  pointer       begin() noexcept
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

  void barrier();

private:
  dash::Team const*      m_team{};
  allocator_type         m_allocator{};
  allocation_policy_t    m_policy{};
  std::vector<size_type> m_local_sizes{};
  dart_gptr_t            m_begin{DART_GPTR_NULL};
  local_pointer          m_lbegin{nullptr};
  local_pointer          m_lend{nullptr};

private:
  void do_allocate(size_type nels);
  void do_deallocate();
};  // namespace experimental

///////////// Implementation ///////////////////
//
template <class ElementType, class LMemSpace, class SynchronizationPolicy>
inline GlobalStaticMemory<ElementType, LMemSpace, SynchronizationPolicy>::
    GlobalStaticMemory(size_type nels, dash::Team const& team)
  : m_team(&team)
  , m_allocator(get_default_memory_space<
                memory_domain_local,
                typename memory_traits::memory_space_type_category>())
  , m_local_sizes(team.size())
{
  DASH_LOG_DEBUG("< GlobalStaticMemory.GlobalStaticMemory");
  DASH_LOG_DEBUG_VAR("GlobalStaticMemory.GlobalStaticMemory", team);
  DASH_LOG_DEBUG_VAR("GlobalStaticMemory.GlobalStaticMemory", nels);

  do_allocate(nels);

  DASH_LOG_DEBUG("GlobalStaticMemory.GlobalStaticMemory >");
}

template <class ElementType, class LMemSpace, class SynchronizationPolicy>
inline GlobalStaticMemory<ElementType, LMemSpace, SynchronizationPolicy>::
    GlobalStaticMemory(size_type nels, LMemSpace* r, dash::Team const& team)
  : m_team(&team)
  , m_allocator(
        r ? r
          : get_default_memory_space<
                memory_domain_local,
                typename memory_traits::memory_space_type_category>())
  , m_local_sizes(team.size())
{
  DASH_LOG_DEBUG("< GlobalStaticMemory.GlobalStaticMemory");

  DASH_LOG_DEBUG_VAR("GlobalStaticMemory.GlobalStaticMemory", nels);
  DASH_LOG_DEBUG_VAR("GlobalStaticMemory.GlobalStaticMemory", team);

  do_allocate(nels);

  DASH_LOG_DEBUG("GlobalStaticMemory.GlobalStaticMemory >");
}

template <class ElementType, class LMemSpace, class SynchronizationPolicy>
inline GlobalStaticMemory<ElementType, LMemSpace, SynchronizationPolicy>::
    GlobalStaticMemory(GlobalStaticMemory&& other)
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
inline GlobalStaticMemory<ElementType, LMemSpace, SynchronizationPolicy>&
GlobalStaticMemory<ElementType, LMemSpace, SynchronizationPolicy>::operator=(
    GlobalStaticMemory&& other)
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
inline void
GlobalStaticMemory<ElementType, LMemSpace, SynchronizationPolicy>::
    do_allocate(size_type nels)
{
  auto alloc_rec = m_policy.do_global_allocate(
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
inline GlobalStaticMemory<ElementType, LMemSpace, SynchronizationPolicy>::
    ~GlobalStaticMemory()
{
  DASH_LOG_DEBUG("< GlobalStaticMemory.~GlobalStaticMemory");

  do_deallocate();

  DASH_LOG_DEBUG("GlobalStaticMemory.~GlobalStaticMemory >");
}

template <class ElementType, class LMemSpace, class SynchronizationPolicy>
inline void
GlobalStaticMemory<ElementType, LMemSpace, SynchronizationPolicy>::
    do_deallocate()
{
  DASH_LOG_DEBUG("< GlobalStaticMemory.do_deallocate");

  DASH_LOG_DEBUG_VAR("GlobStaticMemory.do_deallocate", m_lbegin);
  DASH_LOG_DEBUG_VAR("GlobStaticMemory.do_deallocate", m_lend);
  DASH_LOG_DEBUG_VAR("GlobStaticMemory.do_deallocate", m_begin);
  DASH_LOG_DEBUG_VAR("GlobStaticMemory.do_deallocate", m_local_sizes.size());

  if (*m_team != dash::Team::Null() && !DART_GPTR_ISNULL(m_begin)) {
    m_policy.do_global_deallocate(
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

  DASH_LOG_DEBUG("GlobalStaticMemory.do_deallocate >");
}

template <class ElementType, class LMemSpace, class SynchronizationPolicy>
inline void
GlobalStaticMemory<ElementType, LMemSpace, SynchronizationPolicy>::barrier()
{
  m_team->barrier();
}

template <class ElementType, class LMemSpace, class SynchronizationPolicy>
inline typename GlobalStaticMemory<
    ElementType,
    LMemSpace,
    SynchronizationPolicy>::local_pointer
GlobalStaticMemory<ElementType, LMemSpace, SynchronizationPolicy>::
    lbegin() noexcept
{
  return m_lbegin;
}

template <class ElementType, class LMemSpace, class SynchronizationPolicy>
inline typename GlobalStaticMemory<
    ElementType,
    LMemSpace,
    SynchronizationPolicy>::local_pointer
GlobalStaticMemory<ElementType, LMemSpace, SynchronizationPolicy>::lend()
    const noexcept
{
  return m_lend;
}

template <class ElementType, class LMemSpace, class SynchronizationPolicy>
inline typename GlobalStaticMemory<
    ElementType,
    LMemSpace,
    SynchronizationPolicy>::size_type
GlobalStaticMemory<ElementType, LMemSpace, SynchronizationPolicy>::size()
    const noexcept
{
  return std::accumulate(
      std::begin(m_local_sizes),
      std::end(m_local_sizes),
      0,
      std::plus<size_type>());
}

template <class ElementType, class LMemSpace, class SynchronizationPolicy>
inline typename GlobalStaticMemory<
    ElementType,
    LMemSpace,
    SynchronizationPolicy>::size_type
GlobalStaticMemory<ElementType, LMemSpace, SynchronizationPolicy>::local_size(
    dash::team_unit_t id) const noexcept
{
  return m_local_sizes[id];
}

template <class ElementType, class LMemSpace, class SynchronizationPolicy>
inline bool
GlobalStaticMemory<ElementType, LMemSpace, SynchronizationPolicy>::
    do_is_equal(typename base_t::global_memory_space_base_t const& other)
        const noexcept
{
  return true;
}

}  // namespace experimental

}  // namespace dash

#endif
