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
