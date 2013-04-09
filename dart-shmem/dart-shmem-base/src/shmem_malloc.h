#ifndef SHMEM_MALLOC_H_
#define SHMEM_MALLOC_H_

#include "dart/dart_return_codes.h"
#include "dart/dart_gptr.h"
#include "dart/dart_malloc.h"
#include <stddef.h>

typedef struct {
	int key;
	void* value;
} shmid_address;

extern shmid_address shmid2address[MAXNUM_TEAMS];

void putAddress(int teamid, int key, void* value);

void* getAdress(gptr_t gptr);

#endif /* SHMEM_MALLOC_H_ */
