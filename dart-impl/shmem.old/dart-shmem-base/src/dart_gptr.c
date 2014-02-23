/*
 * dart_gptr.c
 *
 *  Created on: Apr 19, 2013
 *      Author: maierm
 */

#include "dart/dart_gptr.h"
#include "shmem_teams.h"
#include "dart_mempool_private.h"

gptr_t dart_gptr_inc_by(gptr_t ptr, int inc)
{
	gptr_t result = ptr;
	result.offset = result.offset + inc;
	return result;
}

gptr_t dart_gptr_switch_unit(gptr_t ptr, int teamid, int from_unit, int to_unit)
{
	dart_mempool mempool = dart_team_mempool_aligned(teamid);
	gptr_t result = ptr;
	result.offset -= from_unit * mempool->size; // switch to unit 0
	result.offset += to_unit * mempool->size; // switch to other unit
	return result;
}
