
#include "dart_types.h"
#include "dart_globmem.h"
#include "dart_communication.h"
#include "dart_memarea.h"
#include "dart_mempool.h"
#include "shmem_mm_if.h"
#include "shmem_logger.h"


void dart_memarea_init(dart_memarea_t *memarea)
{
  DEBUG("in dart_memarea_init %p", memarea);

  if(!memarea) return;
  
  int i;
  for( i=0; i<MAXNUM_MEMPOOLS; i++ ) {
    (memarea->mempools[i]).key_aligned=-1;
    (memarea->mempools[i]).key_unaligned=-1;
    (memarea->mempools[i]).aligned=DART_MEMPOOL_NULL;
    (memarea->mempools[i]).unaligned=DART_MEMPOOL_NULL;
    (memarea->mempools[i]).in_use=0;
  }
}

dart_mempool 
dart_memarea_get_mempool_aligned(dart_memarea_t *memarea,
				 int id)
{
  dart_mempool ret = DART_MEMPOOL_NULL;

  if( memarea && 0<=id && id<MAXNUM_MEMPOOLS ) {
    ret = (memarea->mempools[id]).aligned;
  }
  
  return ret;
}

dart_mempool 
dart_memarea_get_mempool_unaligned(dart_memarea_t *memarea,
				   int id)
{
  dart_mempool ret = DART_MEMPOOL_NULL;

  if( memarea && 0<=id && id<MAXNUM_MEMPOOLS ) {
    ret = (memarea->mempools[id]).unaligned;
  }
  
  return ret;
}

dart_ret_t 
dart_memarea_create_mempool(dart_memarea_t *memarea,
			    int id,
			    dart_team_t teamid,
			    dart_unit_t myid,
			    size_t teamsize,
			    size_t localsize)
{
  if(! (memarea && 0<=id && id<MAXNUM_MEMPOOLS) ) {
    return DART_ERR_INVAL;
  }
  
  size_t totalsize = localsize * teamsize;
  int attach_keys[2];
  if( myid == 0 )
    {
      attach_keys[0] = shmem_mm_create(totalsize);
      attach_keys[1] = shmem_mm_create(totalsize);
    }
  dart_bcast(attach_keys, 2*sizeof(int), 0, teamid );

  (memarea->mempools[id]).key_aligned   = attach_keys[0];
  (memarea->mempools[id]).key_unaligned = attach_keys[1];

  int myoffset = myid * localsize;

  void *addr[2];
  addr[0] = shmem_mm_attach(attach_keys[0]);
  addr[1] = shmem_mm_attach(attach_keys[1]);
  
  dart_mempool pools[2];
  pools[0] = dart_mempool_create(((char*)addr[0]) + myoffset, localsize);
  pools[1] = dart_mempool_create(((char*)addr[1]) + myoffset, localsize);

  pools[0]->shm_address = addr[0];
  pools[1]->shm_address = addr[1];
  pools[0]->localsize = localsize;
  pools[1]->localsize = localsize;

  (memarea->mempools[id]).aligned   = pools[0];
  (memarea->mempools[id]).unaligned = pools[1];

  (memarea->mempools[id]).in_use = 1;

  // todo: check return values of all of the above
  return DART_OK;
}


dart_ret_t 
dart_memarea_destroy_mempool(dart_memarea_t *memarea,
			     int id,
			     dart_team_t teamid,
			     dart_unit_t myid )
{
  if(! (memarea && 0<=id && id<MAXNUM_MEMPOOLS) ) {
    return DART_ERR_INVAL;
  }

  if( !((memarea->mempools[id]).in_use) ) {
    return DART_ERR_INVAL;
  }


  dart_mempool pools[2];
  pools[0] = (memarea->mempools[id]).aligned;
  pools[1] = (memarea->mempools[id]).unaligned;
  
  void *addr[2];
  addr[0] = pools[0]?pools[0]->shm_address:0;
  addr[1] = pools[1]?pools[1]->shm_address:0;
  
  int shm_ids[2];
  shm_ids[0] = (memarea->mempools[id]).key_aligned;
  shm_ids[1] = (memarea->mempools[id]).key_unaligned;

  //DEBUG("destroy_mempool: at %p  (shm_id: %d)", addr, shm_id);
 
  dart_mempool_destroy(pools[0]);
  dart_mempool_destroy(pools[1]);

  shmem_mm_detach(addr[0]);
  shmem_mm_detach(addr[1]);

  dart_barrier(teamid);

  if (myid == 0) {
    shmem_mm_destroy(shm_ids[0]);
    shmem_mm_destroy(shm_ids[1]);
  }

  (memarea->mempools[id]).in_use = 1;

  // todo: check return values of all of the above
  return DART_OK;
}
