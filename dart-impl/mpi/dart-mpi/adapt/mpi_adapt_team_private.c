/** @file mpi_adapt_team_private.c
 *  @date 22 Nov 2013
 *  @brief Implementations for the operations on convertform.
 */

#include <stdio.h>
#include <mpi.h>
#include "dart_types.h"
#include "dart_adapt_mem.h"
#include "mpi_adapt_team_private.h"

/* The convertform is stored in a static array */
uniqueitem_t convertform [MAX_TEAM_NUMBER];

dart_ret_t dart_adapt_convertform_create ()
{
	/* Convertform[0] is always used to store the infos on DART_TEAM_ALL. */
	convertform[0].team.team_id = 0;

	/* "-1" means no parent. */
	convertform[0].team.parent_id = -1;
	convertform[0].team.level = 0;
	convertform[0].flag = 1;
}

dart_ret_t dart_adapt_team_uniqueid (dart_team_t teamid, int32_t* unique_id)
{
	int i, flags;
	flags = 0;

	/* For DART_TEAM_ALL. */
	if (teamid.level == 0)
	{
		*unique_id = 0;
	}
	else
	{
	 	for (i = 1; i < MAX_TEAM_NUMBER; i++)
		{
			/* "flag == 1" means there is a team matching with the unique_id - 'i'. */
			if (convertform[i].flag == 1)
			{
				/* Comparing on per-member basis */
				if ((convertform[i].team.team_id == teamid.team_id) && (convertform[i].team.parent_id == teamid.parent_id) && (convertform[i].team.level == teamid.level))
		 		{
					flags = 1;
		 		}
	  		}
			if (flags == 1)
				break;
		}

		*unique_id = i;
	}
}

dart_ret_t dart_adapt_convertform_add (dart_team_t teamid)
{
	int i;
	for (i = 0; i < MAX_TEAM_NUMBER; i++)
	{
		if (!(convertform[i].flag))
		{
			break;
		}
	}
	convertform[i].team.team_id = teamid.team_id;
	convertform[i].team.parent_id = teamid.parent_id;
	convertform[i].team.level = teamid.level;
	
	/* Set current flag to '1'. */
	convertform[i].flag = 1;
}

dart_ret_t dart_adapt_convertform_remove (dart_team_t teamid)
{
	int i;
	dart_adapt_team_uniqueid (teamid, &i);

	/* Reset current flag. */
	convertform[i].flag = 0;
}
