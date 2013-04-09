/*
 * dart_twosided.c
 *
 * based on fifos
 *
 *  Created on: Apr 3, 2013
 *      Author: maierm
 */

#include "dart-shmem-base/shmif_barriers.h"
#include "dart-shmem-base/shmif_memory_manager.h"
#include "dart-shmem-base/shmif_multicast.h"
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "sysv_logger.h"
#include <unistd.h>
#include <fcntl.h>

#define MAXNUM_TEAMS 64
#define MAXSIZE_GROUP 64

typedef struct fifo_pair_struct
{
	// the names of the pipes for reading/writing
	char *pname_read;
	char *pname_write;

	// the file-descriptors used for reading/writing
	int readfrom;
	int writeto;
} fifo_pair_t;

fifo_pair_t team2fifos[MAXNUM_TEAMS][MAXSIZE_GROUP];

static int send(void *buf, size_t nbytes, int teamid, int dest)
{
	int ret;
	if (team2fifos[teamid][dest].writeto < 0)
	{
		ret = team2fifos[teamid][dest].writeto = open(
				team2fifos[teamid][dest].pname_write, O_WRONLY);
		if (ret < 0)
		{
			fprintf(stderr, "Error sending to %d (pipename: '%s') ret=%d\n",
					dest, team2fifos[teamid][dest].pname_write, ret);
			return -1;
		}
	}
	ret = write(team2fifos[teamid][dest].writeto, buf, nbytes);
	return ret;
}

static int recv(void *buf, size_t nbytes, int teamid, int source)
{
	int ret;
	if ((team2fifos[teamid][source].readfrom = open(
			team2fifos[teamid][source].pname_read, O_RDONLY)) < 0)
	{
		fprintf(stderr, "Error opening fifo for reading: '%s'\n",
				team2fifos[teamid][source].pname_read);
		return -999;
	}

	ret = read(team2fifos[teamid][source].readfrom, buf, nbytes);
	return (ret != nbytes) ? -999 : 0;
}

int shmif_multicast_init_multicast_group(int group_id, int id_in_group,
		int size)
{
	const char* key = "sysv";
	int i;
	char buf[128];
	for (i = 0; i < size; i++)
	{
		team2fifos[group_id][i].readfrom = -1;
		team2fifos[group_id][i].writeto = -1;
		team2fifos[group_id][i].pname_read = 0;
		team2fifos[group_id][i].pname_write = 0;
	}

	// the unit 'id_in_group' is responsible for creating all named pipes
	// from any other unit to 'id_in_group' ('i'->'id_in_group' for all i)
	for (i = 0; i < size; i++)
	{
		// pipe for sending from <i> to <id_in_group>
		sprintf(buf, "/tmp/%s-group-%d-pipe-from-%d-to-%d", key, group_id, i,
				id_in_group);
		team2fifos[group_id][i].pname_read = strdup(buf);
		if (mkfifo(buf, 0666) < 0)
		{
			ERROR("Error creating fifo: '%s'\n", buf);
			return -999;
		}

		// pipe for sending from <id_in_group> to <i>
		// mkpipe will be called on the receiver side for those
		sprintf(buf, "/tmp/%s-group-%d-pipe-from-%d-to-%d", key, group_id,
				id_in_group, i);
		team2fifos[group_id][i].pname_write = strdup(buf);
	}
	return 0;
}

int shmif_multicast_release_multicast_group(int group_id, int my_id,
		int group_size)
{
	int i;
	char *pname;

	for (i = 0; i < group_size; i++)
	{
		if ((pname = team2fifos[group_id][i].pname_read))
		{
			unlink(pname);
		}
	}
	return 0;
}

int shmif_multicast_bcast(void* buf, size_t nbytes, int root, int group_id,
		int id_in_group, int group_size)
{
	if (id_in_group == root)
	{
		for (int i = 0; i < group_size; i++)
		{
			if (i != root)
				send(buf, nbytes, group_id, i);
		}
	}
	else
	{
		recv(buf, nbytes, group_id, root);
	}
	return 0;
}

