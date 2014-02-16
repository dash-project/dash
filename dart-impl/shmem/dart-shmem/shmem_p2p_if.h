#ifndef SHMEM_P2P_IF_H_INCLUDED
#define SHMEM_P2P_IF_H_INCLUDED

#include "dart_types.h"

// P2P send and receive functionality

int dart_shmem_p2p_init(dart_team_t t, size_t tsize, 
			dart_unit_t myid, int key);
int dart_shmem_p2p_destroy(dart_team_t t, size_t tsize, 
			   dart_unit_t myid, int key);

int dart_shmem_send(void *buf, size_t nbytes, 
		    dart_team_t teamid, dart_unit_t dest);

int dart_shmem_recv(void *buf, size_t nbytes,
		    dart_team_t teamid, dart_unit_t source);

#endif /* SHMEM_P2P_IF_H_INCLUDED */
