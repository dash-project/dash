
#include <dash/dart/shmem/dart_memarea.h>

dart_memarea_t memarea;

void dart_memarea_init()
{
  int i;
  memarea.next_free = 0;
  for (i = 0; i < MAXNUM_MEMPOOLS; i++) {
    dart_mempool_init(
      &((memarea.mempools)[i])
    );
  }
}

dart_mempoolptr 
dart_memarea_get_mempool_by_id(int id)
{
  dart_mempoolptr res = 0;
  if (0 <= id && id < MAXNUM_MEMPOOLS) {
    res = &((memarea.mempools)[id]);
  }
  return res;
}

int dart_memarea_create_mempool(
  dart_team_t teamid,
  size_t teamsize,
  dart_team_unit_t myid,
  size_t localsize,
  int is_aligned)
{
  dart_ret_t ret;
  int res =- 1; 
  if (0 <= memarea.next_free && 
      memarea.next_free < MAXNUM_MEMPOOLS) {
    dart_mempoolptr pool = 
      &((memarea.mempools)[memarea.next_free]);
    ret = dart_mempool_create(
            pool,
            teamid,
            teamsize, 
            myid,
            localsize);
    if (ret == DART_OK) {
      res = memarea.next_free;
      pool->state = (is_aligned?MEMPOOL_ALIGNED:MEMPOOL_UNALIGNED);
      pool->teamid = teamid;
      memarea.next_free++;
    }
  }
  return res;
}
