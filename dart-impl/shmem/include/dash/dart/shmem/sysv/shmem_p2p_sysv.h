#ifndef SHMEM_P2P_SYSV_H_INCLUDED
#define SHMEM_P2P_SYSV_H_INCLUDED

#include <dash/dart/if/dart_types.h>
#include <dash/dart/shmem/dart_groups_impl.h> // for MAXSIZE_GROUP
#include <dash/dart/shmem/dart_teams_impl.h>  // for MAXNUM_TEAMS

typedef struct fifo_pair_struct
{
  // the names of the pipes for reading/writing
  char *pname_read;
  char *pname_write;
  
  // the file-descriptors used for reading/writing
  dart_unit_t readfrom;
  dart_unit_t writeto;
} fifo_pair_t;

fifo_pair_t team2fifos[MAXNUM_TEAMS][MAXSIZE_GROUP];

#endif /* SHMEM_P2P_SYSV_H_INCLUDED */
