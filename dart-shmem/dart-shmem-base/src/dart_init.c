/*
 * dart_init.c
 *
 *  Created on: Apr 3, 2013
 *      Author: maierm
 */

#include <stddef.h>
#include "dart_return_codes.h"
#include "dart_teams.h"
#include "dart_logger.h"
#include "shmif_barriers.h"
#include "shmif_memory_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

extern int _glob_myid;
const int numDartArgs = 4;

pid_t dart_spawn(int id, int nprocs, int shm_id, size_t sync_area_size,
		char *exec, int argc, char **argv);

int dart_start(int argc, char* argv[])
{
	DEBUG("start %s", "called");

	int nprocs; // number of procs to start
	char *dashapp; // path to app executable

	if (argc < 3)
		return 1;

	nprocs = atoi(argv[1]);
	if (nprocs <= 0)
	{
		fprintf(stderr, "Error: Enter a valid number '%s'\n", argv[1]);
		return 1;
	}

	dashapp = argv[2];
	if (access(dashapp, X_OK))
	{
		fprintf(stderr, "Error: Can't open executable '%s'\n", dashapp);
		return 1;
	}

	size_t sync_area_size = 256000; // TODO
	int shm_id = shmif_mm_create(sync_area_size);
	void* shm_addr = shmif_mm_attach(shm_id);
	shmif_barriers_prolog(nprocs, shm_addr);

	for (int i = 0; i < nprocs; i++)
	{
		dart_spawn(i, nprocs, shm_id, sync_area_size, dashapp, argc, argv);
	}

	for (int i = 0; i < nprocs; i++)
	{
		int status;
		pid_t pid = waitpid(-1, &status, 0);
		LOG("child process %d terminated\n", pid);
	}

	shmif_barriers_epilog(nprocs, shm_addr);
	shmif_mm_detach(shm_addr);
	shmif_mm_destroy(shm_id);
	return 0;
}

pid_t dart_spawn(int id, int nprocs, int shm_id, size_t sync_area_size,
		char *exec, int argc, char **argv)
{
	const int maxArgLen = 256;
	pid_t pid;
	int i = 0;
	int dartc;    // new argc for spawned process
	dartc = argc - 2; // don't need initial executable and <n> Parameter
	dartc += numDartArgs + 1; // dartArgs and NULL-Pointer

	char* dartv[dartc];
	for (i = 2; i < argc; i++)
		dartv[i - 2] = strdup(argv[i]);
	for (i = argc - 2; i < dartc - 1; i++)
		dartv[i] = (char*) malloc(maxArgLen + 1);
	dartv[dartc - 1] = NULL;

	i = argc - 2;
	sprintf(dartv[i++], "--dart-id=%d", id);
	sprintf(dartv[i++], "--dart-size=%d", nprocs);
	sprintf(dartv[i++], "--dart-sync_area_id=%d", shm_id);
	sprintf(dartv[i++], "--dart-sync_area_size=%d", sync_area_size);

	pid = fork();
	if (pid == 0)
	{
		int result = execv(exec, dartv);
		if (result == -1)
		{
			char* s = strerror(errno);
			fprintf(stderr, "execv failed: %s\n", s);
		}
		exit(result);
	}
	return pid;
}

void dart_usage(char *s)
{
	fprintf(stderr, "Usage: %s <n> <executable> <args> \n"
			"   Runs n copies of executable\n\n", s);
}

int dart_init(int *argc, char ***argv)
{
	int i;
	int shm_id = -1;
	size_t sync_area_size, stmp;
	int team_size = -1;
	int myid = -1;
	int itmp;

	DEBUG("dart_init %s", "parsing args...");
	for (i = 0; i < (*argc); i++)
	{
		if (sscanf((*argv)[i], "--dart-id=%d", &itmp) > 0)
			myid = itmp;

		if (sscanf((*argv)[i], "--dart-size=%d", &itmp) > 0)
			team_size = itmp;

		if (sscanf((*argv)[i], "--dart-sync_area_id=%d", &itmp) > 0)
			shm_id = itmp;

		if (sscanf((*argv)[i], "--dart-sync_area_size=%d", &stmp) > 0)
			sync_area_size = stmp;
	}

	*argc -= numDartArgs;
	(*argv)[*argc] = NULL;

	if (myid < 0 || team_size < 1)
	{
		return DART_ERR_OTHER;
	}

	DEBUG("dart_init attaching shm %d", shm_id);
	void* sync_area = shmif_mm_attach(shm_id);
	DEBUG("dart_init init internal sync %p", sync_area);
	shmif_barriers_init(team_size, sync_area);
	DEBUG("dart_init %s", "teams_init");
	dart_teams_init(myid, team_size);
	DEBUG("dart_init %s", "calling barrier");
	DART_SAFE(dart_barrier(DART_TEAM_ALL));
	DEBUG("dart_init %s", "done");
	return DART_OK;
}

void dart_exit(int exitcode)
{
	int size = dart_team_size(DART_TEAM_ALL);
	dart_teams_cleanup(_glob_myid, size);
	shmif_barriers_destroy();
}
