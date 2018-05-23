#ifndef DASH__ALLOCATOR__POLICY_H__INCLUDED
#define DASH__ALLOCATOR__POLICY_H__INCLUDED

#include <dash/dart/if/dart.h>

#include <dash/Exception.h>
#include <dash/Types.h>
#include <dash/allocator/internal/Types.h>
#include <dash/memory/MemorySpace.h>

namespace dash {

enum class global_allocation_policy : uint8_t {
  /// All units allocate the same amount of memory
  collective,

  /// only one unit allocates in global memory
  non_collective,

  /// all units allocate invdividually in local memory and synchronize in
  /// epochs
  epoch_synchronized,

};

namespace allocator {

template <typename T>
struct AttachDetachPolicy {
  inline dart_gptr_t do_global_attach(
      dart_team_t teamid, T* ptr, std::size_t nels)
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

  inline bool do_global_detach(dart_gptr_t gptr)
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

namespace detail {

template <class LocalAlloc, typename MemSpaceCategory>
class CollectiveAllocationPolicyImpl
  : public AttachDetachPolicy<
        typename std::allocator_traits<LocalAlloc>::value_type> {
private:
  using allocator_traits = std::allocator_traits<LocalAlloc>;
  using base_t           = AttachDetachPolicy<
      typename std::allocator_traits<LocalAlloc>::value_type>;

public:
  using allocation_rec_t =
      allocator::allocation_rec<typename allocator_traits::pointer>;

  /// Variant to allocate symmetrically in global memory space if we do not
  /// allocate in the default Host Space. In this we have to allocate locally
  /// and subsequently attach the locally allocated memory to the global DART
  /// memory.
  allocation_rec_t do_global_allocate(
      dart_team_t teamid, LocalAlloc& a, std::size_t nels)
  {
    auto lp = allocator_traits::allocate(a, nels);

    if (!lp && nels > 0) {
      DASH_LOG_ERROR(
          "CollectiveAllocationPolicy.global_allocate",
          "cannot allocate local memory segment",
          nels);
      return allocation_rec_t{};
    }

    auto gptr = base_t::do_global_attach(teamid, lp, nels);

    if (DART_GPTR_ISNULL(gptr)) {
      allocator_traits::deallocate(a, lp, nels);
      return allocation_rec_t{};
    }

    return allocation_rec_t(lp, nels, gptr);
  }

  /// Similar to the allocation case above this detaches from global memory
  /// and subsequently releases the local memory.
  bool do_global_deallocate(LocalAlloc& a, allocation_rec_t& rec)
  {
    DASH_LOG_TRACE(
        "SymmatricAllocationPolicy.deallocate",
        "deallocating memory segment (lptr, nelem, gptr)",
        rec.lptr(),
        rec.length(),
        rec.gptr());

    auto ret = base_t::do_global_detach(rec.gptr());

    DASH_LOG_DEBUG("SymmatricAllocationPolicy.deallocate", "_segments.erase");
    allocator_traits::deallocate(a, rec.lptr(), rec.length());

    DASH_ASSERT_RETURNS(dart_barrier(rec.gptr().teamid), DART_OK);

    return ret;
  }
};

template <class LocalAlloc>
class CollectiveAllocationPolicyImpl<LocalAlloc, dash::memory_space_host_tag> {
private:
  using allocator_traits = std::allocator_traits<LocalAlloc>;

public:
  using allocation_rec_t =
      allocator::allocation_rec<typename allocator_traits::pointer>;

  /// Variant to allocate symmetrically in global memory space if we
  /// allocate in the default Host Space. In this case DART can allocate
  /// symmatrically.
  allocation_rec_t do_global_allocate(
      dart_team_t teamid, LocalAlloc& /*a*/, std::size_t nels)
  {
    DASH_LOG_DEBUG(
        "CollectiveAllocationPolicyImpl.do_global_allocate(nlocal)",
        "number of local values:",
        nels);

    dart_gptr_t gptr;

    dash::dart_storage<typename allocator_traits::value_type> ds(nels);
    if (dart_team_memalloc_aligned(teamid, ds.nelem, ds.dtype, &gptr) !=
        DART_OK) {
      DASH_LOG_ERROR(
          "CollectiveAllocationPolicyImpl.do_global_allocate(nlocal)",
          "cannot allocate global memory segment",
          nels);
      gptr = DART_GPTR_NULL;
    }
    DASH_LOG_DEBUG_VAR("CollectiveAllocationPolicyImpl.do_global_allocate >", gptr);
    typename allocator_traits::pointer addr;
    dart_gptr_getaddr(
        gptr,
        reinterpret_cast<typename allocator_traits::void_pointer*>(&addr));
    return allocation_rec_t(addr, nels, gptr);
  }

  /// Similar to the allocation case above global memory is deallocated
  /// symmetrically in DART.
  bool do_global_deallocate(LocalAlloc& /*a*/, allocation_rec_t& rec)
  {
    DASH_LOG_TRACE(
        "CollectiveAllocationPolicyImpl.do_global_deallocate",
        "deallocating memory segment (lptr, nelem, gptr)",
        rec.lptr(),
        rec.length(),
        rec.gptr());

    // We have to wait for the other since dart_team_memfree is non-collective
    // This is required for symmetric allocation policy as the memory may be
    // detached while other units are still operting on the unit's global
    // memory
    DASH_ASSERT_RETURNS(dart_barrier(rec.gptr().teamid), DART_OK);

    auto ret = dart_team_memfree(rec.gptr()) == DART_OK;

    return ret;
  }
};
}  // namespace detail

/**
 * CollectiveAllocationPolicy: Implements the mechanisms to allocate
 * symmetrically from the global memory space. This means that all
 * units allocate collectively the same number of blocks. This is suitable for
 * static containers such as Arrays where all units allocate collectively a
 * local portion to global memory.
 * Note: All gobal memory allocations and deallocations are collective
 */
template <class LocalAlloc>
using CollectiveAllocationPolicy = detail::CollectiveAllocationPolicyImpl<
    LocalAlloc,
    typename dash::memory_space_traits<
        typename LocalAlloc::memory_space>::memory_space_type_category>;

/**
 * LocalAllocationPolicy: Implements a mechanism to allocate locally,
 * independent of the other units. This local memory portion is allocated from
 * a memory pool which is already attached to global memory. This variant is a
 * good fit in cases where only one units needs to allocate memory while the
 * other units do not contribute any local portion. It is used for example
 * used to implement dash::Shared. Note: Both allocation and deallocation are
 * non-collective. The user has to ensure that the owning unit does not
 * release the memory while other units are still operating on it.
 */
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
  using allocation_rec_t =
      allocator::allocation_rec<local_pointer, dart_gptr_t>;

public:
  inline allocation_rec_t do_global_allocate(
      dart_team_t /*teamid*/, LocalAlloc& /*a*/, std::size_t nels) const
  {
    dart_gptr_t   gptr;
    local_pointer addr;

    if (nels > 0) {
      dash::dart_storage<value_type> ds(nels);
      auto const ret = dart_memalloc(ds.nelem, ds.dtype, &gptr);
      if (ret != DART_OK) {
        DASH_LOG_ERROR(
            "LocalAllocationPolicy.do_global_allocate",
            "cannot allocate local memory",
            ret);
        return allocation_rec_t{};
      }
      dart_gptr_getaddr(
          gptr,
          reinterpret_cast<typename allocator_traits::void_pointer*>(&addr));
      DASH_LOG_DEBUG_VAR("LocalAllocator.allocate >", gptr);
    }

    return allocation_rec_t{addr, nels, gptr};
  };

  inline bool do_global_deallocate(
      LocalAlloc& /*a*/, allocation_rec_t& rec) const
  {
    DASH_LOG_DEBUG_VAR(
        "LocalAllocationPolicy.do_global_deallocate", rec.gptr());
    auto const ret = dart_memfree(rec.gptr()) == DART_OK;
    rec.gptr()     = DART_GPTR_NULL;
    return ret;
  };
};

}  // namespace allocator
}  // namespace dash
#endif
