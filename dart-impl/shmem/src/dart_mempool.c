
#include <dash/dart/if/dart.h>

#include <dash/dart/shmem/dart_membucket.h>
#include <dash/dart/shmem/dart_mempool.h>
#include <dash/dart/shmem/shmem_mm_if.h>
#include <dash/dart/shmem/shmem_logger.h>

void dart_mempool_init(dart_mempoolptr pool)
{
  if( !pool) return;

  pool->state     = MEMPOOL_NULL;
  pool->base_addr = 0;
  pool->localbase_addr = 0;
  pool->shmem_key = -1;
  pool->teamid    = -1;
  pool->bucket    = DART_MEMBUCKET_NULL;
}

dart_ret_t dart_mempool_create(dart_mempoolptr pool,
			       dart_team_t teamid,
			       size_t teamsize,
			       dart_team_unit_t myid,
			       size_t localsz)
{
  if( !pool ) return DART_ERR_INVAL;

  size_t totalsz = teamsize * localsz;
  int   attach_key;
  void *attach_addr;
  dart_team_unit_t root = DART_TEAM_UNIT_ID(0);

  // TODO: maybe a barrier here...

  if( myid.id == 0 ) {
    attach_key = shmem_mm_create(totalsz);
  }
  
  dart_bcast(&attach_key, 1, DART_TYPE_INT, root, teamid );

  attach_addr = shmem_mm_attach(attach_key);

  dart_membucket membucket;
  int myoffset = myid.id * localsz;

  membucket = 
    dart_membucket_create( ((char*)attach_addr)+myoffset, 
			   localsz );

  // todo: check if everything is ok
    
  pool->bucket    = membucket;
  pool->base_addr = attach_addr;
  pool->localbase_addr = ((char*)attach_addr)+myoffset;
  pool->localsz   = localsz;
  pool->shmem_key = attach_key;

  //pool->state = ...
  
  // TODO: maybe a barrier here...

  return DART_OK;
}
