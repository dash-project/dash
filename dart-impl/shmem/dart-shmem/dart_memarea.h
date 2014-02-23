#ifndef DART_MEMAREA_H_INCLUDED
#define DART_MEMAREA_H_INCLUDED

#include "dart_types.h"
#include "dart_mempool.h"

#include "extern_c.h"
EXTERN_C_BEGIN


#define MAXNUM_MEMPOOLS 10

struct dart_mempools
{
  int in_use;
  int key_aligned;
  int key_unaligned;

  dart_mempool aligned;
  dart_mempool unaligned;
};


typedef struct dart_memarea_struct
{
  struct dart_mempools mempools[MAXNUM_MEMPOOLS];
} dart_memarea_t;

void dart_memarea_init(dart_memarea_t *memarea);

dart_mempool 
dart_memarea_get_mempool_aligned(dart_memarea_t *memarea,
				 int id);

dart_mempool 
dart_memarea_get_mempool_unaligned(dart_memarea_t *memarea,
				   int id);

dart_ret_t 
dart_memarea_create_mempool(dart_memarea_t *memarea,
			    int id,
			    dart_team_t teamid,
			    dart_unit_t myid,
			    size_t teamsize,
			    size_t localsize);


dart_ret_t 
dart_memarea_destroy_mempool(dart_memarea_t *memarea,
			     int id,
			     dart_team_t teamid,
			     dart_unit_t myid);



EXTERN_C_END

#endif /* DART_MEMAREA_H_INCLUDED */
