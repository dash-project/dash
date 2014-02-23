
#include <dart.h>

#include "dart_malloc.h"
#include "dart_memarea.h"
#include "dart_mempool.h"
#include "dart_teams_impl.h"
#include "shmem_logger.h"


/* TO IMPLEMENT */
/*
dart_ret_t dart_gptr_getaddr(const dart_gptr_t gptr, void *addr);
dart_ret_t dart_gptr_setaddr(dart_gptr_t *gptr, void *addr);
dart_ret_t dart_gptr_setunit(dart_gptr_t *gptr, dart_unit_t);

dart_ret_t dart_memfree(dart_gptr_t gptr);

dart_ret_t dart_team_memalloc_aligned(dart_team_t teamid, 
				      size_t nbytes, dart_gptr_t *gptr);
dart_ret_t dart_team_memfree(dart_team_t teamid, dart_gptr_t gptr);
*/


dart_ret_t dart_gptr_getaddr(const dart_gptr_t gptr, void **addr)
{
  void *ptr; 
  dart_mempool pool;
  dart_memarea_t *mem;
  
  mem = dart_shmem_team_get_memarea(DART_TEAM_ALL);
  if(!mem) return DART_ERR_OTHER;

  pool = dart_memarea_get_mempool_unaligned(mem, 0);
  if(!pool) return DART_ERR_OTHER;
  
  ptr = gptr.addr_or_offs.offset + ((char*)pool->shm_address);
  
  (*addr) = ptr; 
  return DART_OK;
}


dart_ret_t dart_memalloc(size_t nbytes,
			 dart_gptr_t *gptr)
{
  dart_mempool pool;
  dart_memarea_t *mem;
  dart_unit_t myid;

  if(!gptr) return DART_ERR_INVAL;

  mem = dart_shmem_team_get_memarea(DART_TEAM_ALL);
  if(!mem) return DART_ERR_OTHER;

  pool = dart_memarea_get_mempool_unaligned(mem, 0);
  if(!pool) return DART_ERR_OTHER;

  void *addr;
  addr = dart_mempool_alloc(pool, nbytes);
  if( addr==((void*)0) )
    {
      ERROR("Could not alloc memory in mempool%s", "");
      return DART_ERR_OTHER;
    }
  
  dart_myid(&myid);

  gptr->unitid = myid;
  
  // abusing segment id
  gptr->segid = DART_TEAM_ALL * MAXNUM_MEMPOOLS + 0;
  gptr->addr_or_offs.offset = 
    ((char*)addr)-((char*)pool->shm_address);

  //fprintf(stderr, "got %p for addr\n", addr);
  /*
    result.segid = dart_team_unique_id(DART_TEAM_ALL);
    result.offset = ((char*) addr) - ((char*) mempool->shm_address);
    */
  return DART_OK;
}
