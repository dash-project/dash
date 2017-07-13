
#ifndef DART_MEMAREA_H_INCLUDED
#define DART_MEMAREA_H_INCLUDED

#include <dash/dart/shmem/extern_c.h>
EXTERN_C_BEGIN

#include <dash/dart/shmem/dart_mempool.h>

struct dart_memarea
{
  int next_free;
  
  struct dart_mempool mempools[MAXNUM_MEMPOOLS];
};

typedef struct dart_memarea dart_memarea_t;

void dart_memarea_init();

dart_mempoolptr 
dart_memarea_get_mempool_by_id(int id); 


// create a new mempool and return its id
int dart_memarea_create_mempool(dart_team_t teamid,
				size_t teamsize,
				dart_team_unit_t myid,
				size_t localsize,
				int is_aligned);
			      



EXTERN_C_END

#endif /* DART_MEMAREA_H_INCLUDED */
