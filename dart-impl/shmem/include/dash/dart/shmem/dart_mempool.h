#ifndef DART_MEMPOOL_H_INCLUDED
#define DART_MEMPOOL_H_INCLUDED

#include <dash/dart/shmem/extern_c.h>
EXTERN_C_BEGIN

#include <dash/dart/if/dart.h>
#include <dash/dart/shmem/dart_membucket.h>

#define MAXNUM_MEMPOOLS     1024

#define MEMPOOL_NULL        0
#define MEMPOOL_ALIGNED     1
#define MEMPOOL_UNALIGNED   2

struct dart_mempool
{
  unsigned         state;
  void             *base_addr;
  void             *localbase_addr;
  size_t           localsz;
  int              shmem_key;
  dart_team_t      teamid;
  dart_membucket   bucket;
};

typedef struct dart_mempool* dart_mempoolptr;

void dart_mempool_init(dart_mempoolptr pool);

// this is a collective call!
dart_ret_t dart_mempool_create(dart_mempoolptr pool,
			       dart_team_t teamid,
			       size_t teamsize,
			       dart_team_unit_t myid,
			       size_t localsz);




EXTERN_C_END

#endif /* DART_MEMPOOL_H_INCLUDED */
