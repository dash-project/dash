/*
 * dart_malloc.h
 *
 *  Created on: Apr 3, 2013
 *      Author: maierm
 */

#ifndef DART_MALLOC_H_
#define DART_MALLOC_H_

#include "dart_teams.h"
#include "dart_gptr.h"
#include <stddef.h>

typedef struct
{
	int key;
	void* value;
} shmid_address;

extern shmid_address shmid2address[MAXNUM_TEAMS];

void putAddress(int teamid, int key, void* value);

void* getAdress(gptr_t gptr);

gptr_t dart_alloc_aligned(int teamid, size_t nbytes);

#define DART_ADDRESSOF(ptr) (getAdress(ptr))

#endif /* DART_MALLOC_H_ */
