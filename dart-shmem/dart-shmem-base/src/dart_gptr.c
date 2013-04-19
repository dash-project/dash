/*
 * dart_gptr.c
 *
 *  Created on: Apr 19, 2013
 *      Author: maierm
 */

#include "dart/dart_gptr.h"

gptr_t dart_gptr_inc_by(gptr_t ptr, int inc)
{
	gptr_t result = ptr;
	result.offset = result.offset + inc;
	return result;
}
