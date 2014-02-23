/*
 * dart_communication.c
 *
 *  Created on: Apr 12, 2013
 *      Author: maierm
 */

#include "dart/dart_communication.h"
#include "shmif_multicast.h"
#include "shmem_teams.h"
#include "dart_logger.h"

int dart_bcast(void* buf, size_t nbytes, int root, int team)
{
	int unique_id = dart_team_unique_id(team);
	DEBUG("bcast: multicast_group %d, root %d, team %d", unique_id, root, team);
	return shmif_multicast_bcast(buf, nbytes, root, unique_id,
			dart_team_myid(team), dart_team_size(team));
}

int dart_gather(void *sendbuf, void *recvbuf, size_t nbytes, int root, int team)
{
	int unique_id = dart_team_unique_id(team);
	DEBUG("gather: multicast_group %d, root %d, team %d",
			unique_id, root, team);
	int result = shmif_multicast_gather(sendbuf, recvbuf, nbytes, root,
			unique_id, dart_team_myid(team), dart_team_size(team));
	// TODO: do i need this? There seems to be a problem when i remove the following line!?
	dart_barrier(team);
	return result;
}

int dart_all_gather(void *sendbuf, void *recvbuf, size_t nbytes, int team)
{
	int ts = dart_team_size(team);
	int result = DART_OK;
	for (int i = 0; i < ts; i++)
	{
		result |= dart_gather(sendbuf, recvbuf, nbytes, i, team);
		// TODO: do i need this? There seems to be a problem when i remove the following line!?
		dart_barrier(team);
	}
	return result;
}
