
#include <dash/dart/if/dart_globmem.h>
#include <dash/dart/if/dart_team_group.h>

#include <dash/dart/base/logging.h>
#include <dash/dart/mpi/dart_mem.h>
#include <dash/dart/mpi/dart_communication_priv.h>

#include <mpi.h>

struct dart_allocator_struct {
  dart_gptr_t           base_gptr;
  struct dart_buddy  *  buddy_allocator;
};

dart_ret_t
dart_allocator_new(
  size_t             pool_size,
  dart_team_t        team,
  dart_allocator_t * new_allocator)
{
  int ret;

  // dart_buddy_new will round up to next power of 2
  struct dart_buddy * buddy_allocator = dart_buddy_new(pool_size);
  if (buddy_allocator == NULL) {
    return DART_ERR_INVAL;
  }

  dart_gptr_t base_gptr;
  ret = dart_team_memalloc_aligned(team, pool_size, DART_TYPE_BYTE, &base_gptr);

  if (ret != DART_OK) {
    DART_LOG_ERROR("%s: Failed to allocate global memory pool!", __func__);
    dart_buddy_delete(buddy_allocator);
    return ret;
  }

  // set the base_gptr unit ID to my ID
  dart_team_unit_t myid;
  dart_team_myid(team, &myid);
  base_gptr.unitid = myid.id;

  struct dart_allocator_struct *allocator = malloc(sizeof(*allocator));
  allocator->buddy_allocator = buddy_allocator;
  allocator->base_gptr       = base_gptr;

  *new_allocator = allocator;

  return DART_OK;
}


dart_ret_t
dart_allocator_alloc(
  size_t             nelem,
  dart_datatype_t    dtype,
  dart_gptr_t      * gptr,
  dart_allocator_t   allocator)
{
  size_t      nbytes   = nelem * dart__mpi__datatype_sizeof(dtype);
  dart_gptr_t res_gptr = allocator->base_gptr;
  ssize_t     offset   = dart_buddy_alloc(allocator->buddy_allocator, nbytes);
  if (offset < 0) {
    DART_LOG_WARN("dart_allocator_alloc(%zu): allocator %p out of memory",
                  nbytes, allocator);
    *gptr = DART_GPTR_NULL;
    return DART_ERR_NOMEM;
  }
  res_gptr.addr_or_offs.offset += offset;
  *gptr = res_gptr;
  DART_LOG_DEBUG("dart_memalloc: local alloc nbytes:%lu offset:%"PRIu64"",
                 nbytes, gptr->addr_or_offs.offset);
  return DART_OK;
}


dart_ret_t
dart_allocator_free(
  dart_gptr_t      * gptr,
  dart_allocator_t   allocator)
{
  struct dart_allocator_struct *alloc = allocator;
  if (gptr == NULL || alloc == NULL) {
    return DART_ERR_INVAL;
  }
  dart_gptr_t g = *gptr;
  if (gptr->segid != alloc->base_gptr.segid) {
    DART_LOG_ERROR("dart_allocator_free: invalid segment id:%d (expected %d)",
                   g.segid, alloc->base_gptr.segid);
    return DART_ERR_INVAL;
  }
  uint64_t offset = gptr->addr_or_offs.offset - allocator->base_gptr.addr_or_offs.offset;
  if (dart_buddy_free(alloc->buddy_allocator, offset) == -1) {
    DART_LOG_ERROR("dart_allocator_free: invalid local global pointer: "
                   "invalid offset: %"PRIu64"",
                   g.addr_or_offs.offset);
    return DART_ERR_INVAL;
  }
  *gptr = DART_GPTR_NULL;
  DART_LOG_DEBUG("dart_memfree: local free, gptr.unitid:%2d offset:%"PRIu64"",
                 g.unitid, g.addr_or_offs.offset);
  return DART_OK;
}


dart_ret_t
dart_allocator_destroy(dart_allocator_t *allocator)
{
  int ret;
  struct dart_allocator_struct *alloc = *allocator;
  dart_buddy_delete(alloc->buddy_allocator);
  dart_gptr_t base_gptr = alloc->base_gptr;
  base_gptr.unitid = 0; // reset unit ID to root of the team
  ret = dart_team_memfree(base_gptr);
  if (ret != DART_OK) {
    DART_LOG_ERROR("Failed to deallocate memory pool!");
  }
  free(alloc);
  *allocator = NULL;

  return DART_OK;
}
