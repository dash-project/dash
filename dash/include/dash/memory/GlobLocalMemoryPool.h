#ifndef DASH__MEMORY__GLOB_LOCAL_MEMORY_H__INCLUDED
#define DASH__MEMORY__GLOB_LOCAL_MEMORY_H__INCLUDED

#include <dash/Exception.h>
#include <dash/allocator/AllocationPolicy.h>
#include <dash/memory/MemorySpaceBase.h>

namespace dash {

/// Forward declarations
template <class MSpaceDomainCategory, class MSpaceTypeCategory>
inline MemorySpace<MSpaceDomainCategory, MSpaceTypeCategory>*
get_default_memory_space();

class HostSpace;

template <class ElementType, class MemorySpace>
class GlobPtr;

template <class LMemSpace>
class GlobLocalMemoryPool : public MemorySpace<
                                memory_domain_global,
                                typename dash::memory_space_traits<
                                    LMemSpace>::memory_space_type_category> {
  static_assert(
      std::is_same<LMemSpace, dash::HostSpace>::value,
      "currently we support only dash::HostSpace for local memory "
      "allocation");

  static constexpr size_t max_align = alignof(dash::max_align_t);

  using local_memory_traits = dash::memory_space_traits<LMemSpace>;

  using base_t = MemorySpace<
      memory_domain_global,
      typename local_memory_traits::memory_space_type_category>;

  using pointer       = dash::GlobPtr<void, GlobLocalMemoryPool>;
  using const_pointer = dash::GlobPtr<const void, GlobLocalMemoryPool>;

public:
  // inherit parent typedefs
  using memory_space_domain_category = memory_domain_global;
  using memory_space_type_category =
      typename base_t::memory_space_type_category;

  using size_type       = dash::default_size_t;
  using index_type      = dash::default_index_t;
  using difference_type = index_type;

  using memory_space_allocation_policy      = allocation_static;
  using memory_space_synchronization_policy = synchronization_independent;
  using memory_space_layout_tag             = memory_space_noncontiguous;

  // We use a polymorhic allocator to obtain memory from the
  // local memory space
  using allocator_type = std::pmr::polymorphic_allocator<std::byte>;

  using void_pointer             = pointer;
  using const_void_pointer       = const_pointer;
  using local_void_pointer       = void*;
  using const_local_void_pointer = void*;

public:
  GlobLocalMemoryPool() = delete;

  explicit GlobLocalMemoryPool(
      size_type         pool_capacity = 0,
      dash::Team const& team          = dash::Team::All());

  GlobLocalMemoryPool(
      LMemSpace*        r,
      size_type         pool_capacity = 0,
      dash::Team const& team          = dash::Team::All());

  ~GlobLocalMemoryPool() override;

  GlobLocalMemoryPool(const GlobLocalMemoryPool&) = delete;
  GlobLocalMemoryPool(GlobLocalMemoryPool&&);

  GlobLocalMemoryPool& operator=(const GlobLocalMemoryPool&) = delete;
  GlobLocalMemoryPool& operator=(GlobLocalMemoryPool&&);

  size_type size() const DASH_ASSERT_NOEXCEPT
  {
    return m_size;
  }

  size_type capacity() const DASH_ASSERT_NOEXCEPT
  {
    return m_capacity;
  }

  dash::Team const& team() const DASH_ASSERT_NOEXCEPT
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

  pointer allocate(size_type nbytes, size_type alignment = max_align)
  {
    return do_allocate(nbytes, alignment);
  }
  void deallocate(
      pointer gptr, size_type nbytes, size_type alignment = max_align)
  {
    return do_deallocate(gptr, nbytes, alignment);
  }
  void release();

  /**
   * Complete all outstanding non-blocking operations to all units.
   */
  void flush(pointer gptr) DASH_ASSERT_NOEXCEPT
  {
    DASH_ASSERT_MSG(gptr, "cannot flush DART_GPTR_NULL");
    dart_flush_all(gptr);
  }

  /**
   * Complete all outstanding non-blocking operations to the specified unit.
   */
  void flush(pointer gptr, dash::team_unit_t target) DASH_ASSERT_NOEXCEPT
  {
    DASH_ASSERT_MSG(gptr, "cannot flush DART_GPTR_NULL");
    gptr.unitid(target);
    dart_flush(gptr);
  }

  /**
   * Locally complete all outstanding non-blocking operations to all units.
   */
  void flush_local(pointer gptr) DASH_ASSERT_NOEXCEPT
  {
    DASH_ASSERT_MSG(gptr, "cannot flush DART_GPTR_NULL");
    dart_flush_local_all(gptr);
  }

  /**
   * Locally complete all outstanding non-blocking operations to the specified
   * unit.
   */
  void flush_local(pointer gptr, dash::team_unit_t target)
      DASH_ASSERT_NOEXCEPT
  {
    DASH_ASSERT_MSG(gptr, "cannot flush DART_GPTR_NULL");
    gptr.unitid(target);
    dart_flush(gptr);
    dart_flush_local(gptr);
  }

private:
  dash::Team const*                       m_team{};
  size_type                               m_size{};
  size_type                               m_capacity{};
  allocator_type                          m_allocator{};
  std::vector<std::pair<pointer, size_t>> m_segments;

private:
  // alignment not used: Pools always allocate with alignof(max_align_t)
  pointer do_allocate(size_type nbytes, size_type /*alignment*/);
  void do_deallocate(pointer gptr, size_type nbytes, size_type /*alignment*/);
  void do_segment_free(
      typename std::vector<std::pair<pointer, size_t>>::iterator it_erase);
};

///////////// Implementation ///////////////////
//
template <class LMemSpace>
inline GlobLocalMemoryPool<LMemSpace>::GlobLocalMemoryPool(
    size_type local_capacity, dash::Team const& team)
  : GlobLocalMemoryPool(nullptr, local_capacity, team)
{
}

template <class LMemSpace>
inline GlobLocalMemoryPool<LMemSpace>::GlobLocalMemoryPool(
    LMemSpace* r, size_type local_capacity, dash::Team const& team)
  : m_team(&team)
  , m_size(0)
  , m_capacity(
        local_capacity ? local_capacity
                       : std::numeric_limits<size_type>::max())
  , m_allocator(
        r ? r
          : get_default_memory_space<
                memory_domain_local,
                typename local_memory_traits::memory_space_type_category>())
{
  DASH_LOG_DEBUG("MemorySpace.MemorySpace >");
}

template <class LMemSpace>
inline GlobLocalMemoryPool<LMemSpace>::GlobLocalMemoryPool(
    GlobLocalMemoryPool&& other)
{
  *this = std::move(other);
}

template <class LMemSpace>
inline GlobLocalMemoryPool<LMemSpace>& GlobLocalMemoryPool<LMemSpace>::
                                       operator=(GlobLocalMemoryPool&& other)
{
  if (this == &other) {
    return *this;
  }
  // deallocate own memory
  release();
  // and swap..
  m_team      = other.m_team;
  m_size      = other.m_size;
  m_capacity  = other.m_capacity;
  m_allocator = allocator_type{other.m_allocator.resource()};
  m_segments  = std::move(other.m_segments);

  other.m_segments.clear();

  return *this;
}

template <class LMemSpace>
inline typename GlobLocalMemoryPool<LMemSpace>::pointer
GlobLocalMemoryPool<LMemSpace>::do_allocate(
    size_type nbytes, size_type /*alignment*/)
{
  DASH_LOG_TRACE(
      "MemorySpace.do_allocate",
      "allocate memory",
      "nbytes: ",
      nbytes,
      "capacity: ",
      m_capacity,
      "size: ",
      m_size);

  if ((m_capacity - m_size) < nbytes) {
    throw std::bad_alloc{};
  }

  dart_gptr_t gptr;

  if (nbytes > 0) {
    dash::dart_storage<uint8_t> ds(nbytes);
    auto const ret = dart_memalloc(ds.nelem, ds.dtype, &gptr);
    if (ret != DART_OK) {
      DASH_LOG_ERROR(
          "LocalAllocationPolicy.do_global_allocate",
          "cannot allocate local memory",
          ret);
      throw std::bad_alloc{};
    }
    DASH_LOG_DEBUG_VAR("LocalAllocator.allocate >", gptr);

    m_segments.emplace_back(std::make_pair(pointer{gptr}, nbytes));
    m_size += nbytes;
    return m_segments.back().first;
  }

  return pointer{DART_GPTR_NULL};
}

template <class LMemSpace>
GlobLocalMemoryPool<LMemSpace>::~GlobLocalMemoryPool()
{
  DASH_LOG_DEBUG("< MemorySpace.~MemorySpace");

  release();

  DASH_LOG_DEBUG("MemorySpace.~MemorySpace >");
}

template <class LMemSpace>
inline void GlobLocalMemoryPool<LMemSpace>::do_deallocate(
    pointer gptr, size_type nbytes, size_type /*alignment*/)
{
  DASH_LOG_DEBUG("< MemorySpace.do_deallocate");

  auto it_seg = std::find_if(
      std::begin(m_segments),
      std::end(m_segments),
      [gptr](const std::pair<pointer, size_t>& item) {
        return item.first == gptr;
      });

  if (it_seg != std::end(m_segments)) {
    do_segment_free(it_seg);
    m_segments.erase(it_seg);
    m_size -= nbytes;
  }

  DASH_LOG_DEBUG("MemorySpace.do_deallocate >");
}
template <class LMemSpace>
inline void GlobLocalMemoryPool<LMemSpace>::release()
{
  for (auto it = std::begin(m_segments); it != std::end(m_segments); ++it) {
    do_segment_free(it);
  }

  m_segments.clear();
}

template <class LMemSpace>
inline void GlobLocalMemoryPool<LMemSpace>::do_segment_free(
    typename std::vector<std::pair<pointer, size_t>>::iterator it_erase)
{
  if (!dart_initialized() || *m_team == dash::Team::Null() ||
      !(it_erase->first)) {
    return;
  }

  auto const dart_gptr = it_erase->first.dart_gptr();
  DASH_ASSERT_RETURNS(dart_memfree(dart_gptr), DART_OK);
}

}  // namespace dash

#endif
