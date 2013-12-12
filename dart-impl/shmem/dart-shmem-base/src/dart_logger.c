#include "dart_logger.h"
#include <stdio.h>

char* gptr_to_string(gptr_t ptr)
{
	char* buf;
	asprintf(&buf, "{%d, %d, %u, %llu}", ptr.unitid, ptr.segid, ptr.flags,
			ptr.offset);
	return buf;
}

