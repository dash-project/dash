/*
 * dart.h - DASH RUNTIME (DART) INTERFACE 
 */

#ifndef DART_GPTR_H_INCLUDED
#define DART_GPTR_H_INCLUDED

#include <stdint.h>

#define GPTR_NULL {.unitid = -1, .segid = 0, .flags = 0, .offset = 0}

typedef struct
{
	int32_t unitid;
	int16_t segid;
	uint16_t flags;
	union
	{
		uint64_t offset;
		void* addr;
	};
} gptr_t;

#endif /* DART_GPTR_H_INCLUDED */

