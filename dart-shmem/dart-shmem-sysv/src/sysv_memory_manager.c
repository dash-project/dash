/*
 * dart_shmem.c
 *
 * shared memory based on systemV
 *
 *  Created on: Apr 3, 2013
 *      Author: maierm
 */
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include "sysv_logger.h"

int shmif_mm_create(size_t size)
{
	int key = shmget(IPC_PRIVATE, size, IPC_CREAT | IPC_EXCL | 0600);
	if (key == -1)
	{
		ERRNO("shmget%s", "");
		exit(EXIT_FAILURE);
	}
	return key;
}

void* shmif_mm_attach(int shmem_key)
{
	void* addr = shmat(shmem_key, NULL, 0);
	if (addr == ((void*) -1))
	{
		ERRNO("shmat%s", "");
		exit(EXIT_FAILURE);
	}
	return addr;
}

void shmif_mm_destroy(int key)
{
	if (shmctl(key, IPC_RMID, NULL) == -1)
		ERRNO("shmctl%s", "");
}

void shmif_mm_detach(void* addr)
{
	if (shmdt(addr) == -1)
	{
		ERRNO("shmdt%s", "");
	}
}
