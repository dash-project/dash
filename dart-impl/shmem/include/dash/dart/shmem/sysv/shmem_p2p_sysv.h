#ifndef DASH__DART__SHMEM__MPI__SYSV__SHMEM_P2P_SYSV_H_INCLUDED
#define DASH__DART__SHMEM__MPI__SYSV__SHMEM_P2P_SYSV_H_INCLUDED

#include <dash/dart/if/dart_types.h>
#include <dash/dart/shmem/dart_groups_impl.h> // for MAXSIZE_GROUP
#include <dash/dart/shmem/dart_teams_impl.h>  // for MAXNUM_TEAMS

typedef struct fifo_pair_struct
{
  // the names of the pipes for reading/writing
  char *pname_read;
  char *pname_write;
  
  // the file-descriptors used for reading/writing
  dart_team_unit_t readfrom;
  dart_team_unit_t writeto;
} fifo_pair_t;

fifo_pair_t team2fifos[MAXNUM_TEAMS][MAXSIZE_GROUP];

int dart_shmem_send(
    void *buf,
    size_t nbytes, 
	  dart_team_t teamid,
    dart_team_unit_t dest);

int dart_shmem_isend(
    void *buf,
    size_t nbytes, 
		dart_team_t teamid,
    dart_team_unit_t dest, 
		dart_handle_t *handle);

int dart_shmem_sendevt(
    void *buf,
    size_t nbytes, 
	  dart_team_t teamid,
    dart_team_unit_t dest);

int dart_shmem_recv(
    void *buf,
    size_t nbytes,
		dart_team_t teamid,
    dart_team_unit_t source);

int dart_shmem_irecv(
    void *buf,
    size_t nbytes,
		dart_team_t teamid,
    dart_team_unit_t source,
		dart_handle_t *handle);

int dart_shmem_recvevt(
    void *buf,
    size_t nbytes,
		dart_team_t teamid,
    dart_team_unit_t source);

#endif /* DASH__DART__SHMEM__MPI__SYSV__SHMEM_P2P_SYSV_H_INCLUDED */
