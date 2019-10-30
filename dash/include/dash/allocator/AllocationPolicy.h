#ifndef DASH__ALLOCATOR__POLICY_H__INCLUDED
#define DASH__ALLOCATOR__POLICY_H__INCLUDED

#include <dash/dart/if/dart.h>

#include <dash/Types.h>
#include <dash/Team.h>

#include <dash/allocator/internal/Types.h>
#include <dash/internal/Logging.h>
#include <dash/memory/MemorySpaceBase.h>

std::ostream& operator<<(std::ostream& os, const dart_gptr_t& dartptr);

namespace dash {

enum class global_allocation_policy : uint8_t {
  /// All units collectively allocate global memory
  collective,

  /// only one unit allocates in global memory
  non_collective,

  /// all units allocate invdividually in local memory and synchronize in
  /// epochs
  epoch_synchronized,
};

namespace allocator {

struct AttachDetachPolicy {
  inline dart_gptr_t do_global_attach(
      dart_team_t teamid, void* ptr, std::size_t nbytes)
  {
    dart_gptr_t gptr;

    if (dart_team_memregister(teamid, nbytes, DART_TYPE_BYTE, ptr, &gptr) !=
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

/**
 * Collective AllocationPolicy: Implements the mechanisms to allocate
 * symmetrically from the global memory space. This means that all
 * units allocate collectively the same number of blocks. This is suitable for
 * static containers such as Arrays where all units allocate collectively a
 * local portion to global memory.
 * Note: All gobal memory allocations and deallocations are collective
 */
template <
    class AllocationPolicy,
    class SynchronizationPolicy,
    class LMemSpaceTag>
class GlobalAllocationPolicy;

template <class LMemSpaceTag>
class GlobalAllocationPolicy<
    allocation_static,
    synchronization_collective,
    LMemSpaceTag> : AttachDetachPolicy {
  using memory_space_tag = LMemSpaceTag;

public:
  /// Variant to allocate symmetrically in global memory space if we
  /// allocate in the default Host Space. In this case DART can allocate
  /// symmatrically.
  dart_gptr_t allocate_segment(
      dart_team_t                             teamid,
      LocalMemorySpaceBase<memory_space_tag>* res,
      std::size_t                             nbytes,
      std::size_t                             alignment)
  {
    DASH_LOG_DEBUG(
        "GlobalAllocationPolicy.do_global_allocate(nlocal)",
        "number of local values:",
        nbytes);

    auto lp = res->allocate(nbytes, alignment);

    if (!lp && nbytes > 0) {
      throw std::bad_alloc{};
    }

    DASH_LOG_DEBUG_VAR(
        "GlobalAllocationPolicy.do_global_allocate(nlocal)", lp);

    // Here, we attach the allocated global pointer. DART internally stores
    // this pointer in such a way that we can retrieve it by looking up the
    // local address (teamid <- my_id, addr_or_offset.offset <- 0)
    auto gptr = AttachDetachPolicy::do_global_attach(
        teamid, lp, nbytes);

    if (DART_GPTR_ISNULL(gptr)) {
      res->deallocate(lp, nbytes, alignment);
      throw std::bad_alloc{};
    }

    DASH_LOG_DEBUG_VAR(
        "GlobalAllocationPolicy.do_global_allocate(nlocal)", gptr);

    DASH_LOG_DEBUG("GlobalAllocationPolicy.do_global_allocate(nlocal) >");

    return gptr;
  }

  /// Similar to the allocation case above global memory is deallocated
  /// symmetrically in DART.
  bool deallocate_segment(
      dart_gptr_t                             gptr,
      LocalMemorySpaceBase<memory_space_tag>* res,
      void*                                   lptr,
      size_t                                  nbytes,
      size_t                                  alignment)
  {
    DASH_LOG_DEBUG("< GlobalAllocationPolicy.do_global_deallocate");
    DASH_LOG_DEBUG_VAR("GlobalAllocationPolicy.do_global_deallocate", gptr);
    DASH_LOG_DEBUG_VAR("GlobalAllocationPolicy.do_global_deallocate", lptr);
    DASH_LOG_DEBUG_VAR("GlobalAllocationPolicy.do_global_deallocate", nbytes);

    DASH_ASSERT_RETURNS(AttachDetachPolicy::do_global_detach(gptr), true);

    res->deallocate(lptr, nbytes, alignment);

    DASH_LOG_DEBUG("GlobalAllocationPolicy.do_global_deallocate >");
    return true;
  }
};

template <>
class GlobalAllocationPolicy<
    allocation_static,
    synchronization_collective,
    memory_space_host_tag> {
  using memory_space_tag = memory_space_host_tag;

public:
  /// Variant to allocate symmetrically in global memory space if we
  /// allocate in the default Host Space. In this case DART can allocate
  /// symmatrically.
  dart_gptr_t allocate_segment(
      dart_team_t teamid,
      LocalMemorySpaceBase<memory_space_tag>* /* res */,
      std::size_t nbytes,
      std::size_t /*alignment*/)
  {
    DASH_LOG_DEBUG(
        "GlobalAllocationPolicy.do_global_allocate(nlocal)",
        "number of local values:",
        nbytes);

    dart_gptr_t gptr;

    dash::dart_storage<uint8_t> ds(nbytes);
    if (dart_team_memalloc_aligned(teamid, ds.nelem, ds.dtype, &gptr) !=
        DART_OK) {
      DASH_LOG_ERROR(
          "GlobalAllocationPolicy.do_global_allocate(nlocal)",
          "cannot allocate global memory segment",
          nbytes);
      gptr = DART_GPTR_NULL;
    }

    if (DART_GPTR_ISNULL(gptr)) {
      throw std::bad_alloc{};
    }

    return gptr;
  }

  /// Similar to the allocation case above global memory is deallocated
  /// symmetrically in DART.
  bool deallocate_segment(
      dart_gptr_t gptr,
      LocalMemorySpaceBase<memory_space_tag>* /* unused */,
      void* /* unused */,
      size_t /* unused */,
      size_t /*unused*/)
  {
    DASH_LOG_TRACE(
        "GlobalAllocationPolicy.do_global_deallocate",
        "deallocating memory segment",
        gptr);

    if (DART_GPTR_ISNULL(gptr)) {
      return true;
    }

    auto ret = dart_team_memfree(gptr) == DART_OK;

    return ret;
  }
};

/**
 * LocalAllocationPolicy: Implements a mechanism to allocate locally,
 * independent of the other units. This local memory portion is allocated from
 * a memory pool which is already attached to global memory. This variant is a
 * good fit in cases where only one units needs to allocate memory while the
 * other units do not contribute any local portion. It is used for example
 * used to implement dash::Shared.
 *
 * Note: Both allocation and deallocation are
 * non-collective. The user has to ensure that the owning unit does not
 * release the memory while other units are still operating on it.
 */
template <>
class GlobalAllocationPolicy<
    allocation_static,
    synchronization_single,
    memory_space_host_tag> {
  using memory_space_tag = memory_space_host_tag;

public:
  /// Variant to allocate only locally in global memory space if we
  /// allocate in the default Host Space. In this case DART allocates from the
  /// internal buddy allocator.
  dart_gptr_t allocate_segment(
      /// The local memory resource to allocated from
      LocalMemorySpaceBase<memory_space_tag>* /* res */,
      /// the number of bytes to allocate
      std::size_t nbytes,
      /// The alignment
      std::size_t /*alignment*/)
  {
    dart_gptr_t gptr = DART_GPTR_NULL;

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
    }
    return gptr;
  }

  /// Similar to the allocation case above global memory is deallocated
  /// symmetrically in DART.
  bool deallocate_segment(
      dart_gptr_t gptr,
      LocalMemorySpaceBase<memory_space_tag>* /* unused */,
      void* /* unused */,
      size_t /* unused */,
      size_t /*unused*/)
  {
    auto const ret = dart_memfree(gptr) == DART_OK;
    return ret;
  }
};

}  // namespace allocator
}  // namespace dash
#endif
