#ifndef DASH__ALLOCATOR__BASE_H__INCLUDED
#define DASH__ALLOCATOR__BASE_H__INCLUDED

#include <dash/Exception.h>
#include <dash/Types.h>
#include <dash/memory/MemorySpace.h>

namespace dash {

enum class global_allocation_policy : uint8_t {
  /// All units allocate the same amount of memory
  symmetric,

  /// all units allocate invdividually in local memory and synchronize in
  /// epochs
  epoch_synchronized,

  /// only one unit allocates in global memory
  local
};

namespace allocator {

template <typename T>
using polymorphic_allocator = cpp17::pmr::polymorphic_allocator<T>;

template <typename T>
struct AttachDetachPolicy {
protected:
  dart_gptr_t do_global_attach(dart_team_t teamid, T* ptr, std::size_t nels)
  {
    dart_gptr_t           gptr;
    dash::dart_storage<T> ds(nels);

    if (dart_team_memregister(teamid, ds.nelem, ds.dtype, ptr, &gptr) !=
        DART_OK) {
      gptr = DART_GPTR_NULL;
      DASH_LOG_ERROR(
          "AttachDetachPolicy.global_attach", "cannot attach pointer", ptr);
    }
    return gptr;
  }
  //
  bool do_global_detach(dart_gptr_t gptr)
  {
    if (dart_team_memderegister(gptr) != DART_OK) {
      DASH_LOG_ERROR(
          "AttachDetachPolicy.global_detach",
          "cannot detach global pointer",
          gptr);
      return false;
    }
    return true;
  }
};

template <typename T, typename LocalMemorySpace>
class DefaultAllocator : public polymorphic_allocator<T> {
  using memory_traits = typename dash::memory_space_traits<LocalMemorySpace>;

  static_assert(memory_traits::is_local::value, "memory space must be local");

  static_assert(
      std::is_base_of<cpp17::pmr::memory_resource, LocalMemorySpace>::value,
      "Local Memory Space must be a polymorphic memory resource");

  using base_t = polymorphic_allocator<T>;

public:
  DefaultAllocator() = default;
  DefaultAllocator(memory_resource* r)
    : base_t(r)
  {
  }

public:
  using memory_space = LocalMemorySpace;
};

template <class LocalAlloc>
class SymmetricAllocationPolicy
  : protected AttachDetachPolicy<
        typename std::allocator_traits<LocalAlloc>::value_type> {
  using memory_traits =
      typename dash::memory_space_traits<typename LocalAlloc::memory_space>;
  using is_default_ram_space = typename std::is_same<
      typename memory_traits::memory_space_type_category,
      memory_space_host_tag>::type;

  using allocator_traits = std::allocator_traits<LocalAlloc>;
  using base_t = AttachDetachPolicy<typename allocator_traits::value_type>;

  using local_pointer = typename allocator_traits::pointer;

public:
  // tuple to hold the local pointer
  using allocation_rec = std::tuple<local_pointer, size_t, dart_gptr_t>;

public:
  inline allocation_rec do_global_allocate(
      dart_team_t teamid, LocalAlloc& a, std::size_t nels)
  {
    return this->do_global_allocate(teamid, a, nels, is_default_ram_space{});
  }
  inline bool do_global_deallocate(LocalAlloc& a, allocation_rec& rec)
  {
    return this->do_global_deallocate(a, rec, is_default_ram_space{});
  }

private:
  /// Variant to allocate symmetrically in global memory space if we do not
  /// allocate in the default Host Space. In this we have to allocate locally
  /// and subsequently attach the locally allocated memory to the global DART
  /// memory.
  allocation_rec do_global_allocate(
      dart_team_t teamid, LocalAlloc& a, std::size_t nels, std::false_type);

  /// Variant to allocate symmetrically in global memory space if we
  /// allocate in the default Host Space. In this case DART can allocate
  /// symmatrically.
  allocation_rec do_global_allocate(
      dart_team_t teamid, LocalAlloc& a, std::size_t nels, std::true_type);

  /// Similar to the first case above this variant detaches from global memory
  /// and subsequently releases the local memory.
  bool do_global_deallocate(
      LocalAlloc& a, allocation_rec& rec, std::false_type);

  /// Similar to the second variant memory is deallocated symmetrically in
  /// DART.
  bool do_global_deallocate(
      LocalAlloc& a, allocation_rec& rec, std::true_type);
};

template <class LocalAlloc>
class LocalAllocationPolicy {
  using memory_traits =
      typename dash::memory_space_traits<typename LocalAlloc::memory_space>;

  static_assert(
      std::is_same<
          typename memory_traits::memory_space_type_category,
          memory_space_host_tag>::value,
      "Only Host Space is supported with a local allocation policy");

  // tuple to hold the local pointer
  using allocator_traits = std::allocator_traits<LocalAlloc>;
  using local_pointer    = typename allocator_traits::pointer;
  using value_type       = typename allocator_traits::value_type;

public:
  using allocation_rec = std::tuple<local_pointer, size_t, dart_gptr_t>;

public:
  inline allocation_rec do_global_allocate(
      dart_team_t teamid, LocalAlloc& a, std::size_t nels) const
  {
    dart_gptr_t   gptr = DART_GPTR_NULL;
    local_pointer addr = nullptr;

    if (nels > 0) {
      dash::dart_storage<value_type> ds(nels);
      auto const ret = dart_memalloc(ds.nelem, ds.dtype, &gptr);
      if (ret != DART_OK) {
        DASH_LOG_ERROR(
            "LocalAllocationPolicy.do_global_allocate",
            "cannot allocate local memory",
            ret);
        gptr = DART_GPTR_NULL;
        return std::make_tuple(addr, nels, gptr);
      }
      dart_gptr_getaddr(
          gptr,
          reinterpret_cast<typename allocator_traits::void_pointer*>(&addr));
      DASH_LOG_DEBUG_VAR("LocalAllocator.allocate >", gptr);
    }

    return std::make_tuple(addr, nels, gptr);
  };

  inline bool do_global_deallocate(LocalAlloc& a, allocation_rec& rec) const
  {
    auto& gptr = std::get<2>(rec);
    DASH_LOG_DEBUG_VAR("LocalAllocationPolicy.do_global_deallocate", gptr);
    auto const ret = dart_memfree(gptr) == DART_OK;
    gptr           = DART_GPTR_NULL;
    return ret;
  };
};

template <class LocalAlloc>
typename SymmetricAllocationPolicy<LocalAlloc>::allocation_rec
SymmetricAllocationPolicy<LocalAlloc>::do_global_allocate(
    dart_team_t teamid, LocalAlloc& a, std::size_t nels, std::false_type)
{
  auto ret = std::make_tuple(nullptr, nels, DART_GPTR_NULL);

  auto lp = allocator_traits::allocate(a, nels);

  if (!lp && nels > 0) {
    DASH_LOG_ERROR(
        "SymmetricAllocationPolicy.global_allocate",
        "cannot allocate local memory segment",
        nels);
    return ret;
  }

  auto gptr = base_t::do_global_attach(teamid, lp, nels);

  if (DART_GPTR_ISNULL(gptr)) {
    allocator_traits::deallocate(a, lp, nels);
    return ret;
  }

  return std::make_tuple(lp, nels, gptr);
}

template <class LocalAlloc>
typename SymmetricAllocationPolicy<LocalAlloc>::allocation_rec
SymmetricAllocationPolicy<LocalAlloc>::do_global_allocate(
    dart_team_t teamid, LocalAlloc& a, std::size_t nels, std::true_type)
{
  DASH_LOG_DEBUG(
      "SymmetricAllocationPolicy.global_allocate(nlocal)",
      "number of local values:",
      nels);

  dart_gptr_t gptr;

  dash::dart_storage<typename allocator_traits::value_type> ds(nels);
  if (dart_team_memalloc_aligned(teamid, ds.nelem, ds.dtype, &gptr) !=
      DART_OK) {
    DASH_LOG_ERROR(
        "SymmetricAllocationPolicy.global_allocate(nlocal)",
        "cannot allocate global memory segment",
        nels);
    gptr = DART_GPTR_NULL;
  }
  DASH_LOG_DEBUG_VAR("SymmetricAllocationPolicy.global_allocate >", gptr);
  local_pointer addr;
  dart_gptr_getaddr(
      gptr,
      reinterpret_cast<typename allocator_traits::void_pointer*>(&addr));
  return std::make_tuple(addr, nels, gptr);
}

template <class LocalAlloc>
bool SymmetricAllocationPolicy<LocalAlloc>::do_global_deallocate(
    LocalAlloc& a, allocation_rec& rec, std::false_type)
{
  auto& seg_lptr  = std::get<0>(rec);
  auto& seg_nelem = std::get<1>(rec);
  auto& seg_gptr  = std::get<2>(rec);

  DASH_LOG_TRACE(
      "SymmatricAllocationPolicy.deallocate",
      "deallocating memory segment (lptr, nelem, gptr)",
      seg_lptr,
      seg_nelem,
      seg_gptr);

  auto ret = base_t::do_global_detach(seg_gptr);

  DASH_LOG_DEBUG("SymmatricAllocationPolicy.deallocate", "_segments.erase");
  allocator_traits::deallocate(a, seg_lptr, seg_nelem);

  DASH_ASSERT_RETURNS(dart_barrier(seg_gptr.teamid), DART_OK);

  return ret;
}

template <class LocalAlloc>
bool SymmetricAllocationPolicy<LocalAlloc>::do_global_deallocate(
    LocalAlloc& a, allocation_rec& rec, std::true_type)
{
  auto& seg_lptr  = std::get<0>(rec);
  auto& seg_nelem = std::get<1>(rec);
  auto& seg_gptr  = std::get<2>(rec);

  DASH_LOG_TRACE(
      "SymmatricAllocationPolicy.deallocate",
      "deallocating memory segment (lptr, nelem, gptr)",
      seg_lptr,
      seg_nelem,
      seg_gptr);

  auto ret = dart_team_memfree(seg_gptr) == DART_OK;

  DASH_ASSERT_RETURNS(dart_barrier(seg_gptr.teamid), DART_OK);

  return ret;
}
}  // namespace allocator

}  // namespace dash
#endif
