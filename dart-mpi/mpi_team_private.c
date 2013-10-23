#include <stdio.h>
#include <mpi.h>
#include "dart_types.h"
#include "dart_mem.h"
#include "mpi_team_private.h"


uniqueitem convertform [MAX_TEAM_NUMBER];


dart_ret_t dart_team_uniqueid (dart_team_t team, int32_t* unique_id)
{
	int i, flags;
	flags = 0;
	if (team.team_id == 0)
	{
		*unique_id = 0;
	}
	else
	{
	 	for (i = 1; i < MAX_TEAM_NUMBER; i++)
		{
			if (convertform[i].flag == 1)
			{
				if ((convertform[i].team.team_id == team.team_id) && (convertform[i].team.parent_id == team.parent_id) && (convertform[i].team.level == team.level))
		 		{
					flags = 1;
		 		}
	  		}
			if (flags == 1)
				break;
		}

		*unique_id = i;
//		printf ("unique: *unique_id is %d\n", *unique_id);
	}
}

dart_ret_t dart_convertform_add (dart_team_t team)
{
	int i;
	for (i = 0; i < MAX_TEAM_NUMBER; i++)
	{
		if (!(convertform[i].flag))
		{
			break;
		}
	}
	convertform[i].team.team_id = team.team_id;
	convertform[i].team.parent_id = team.parent_id;
	convertform[i].team.level = team.level;
	convertform[i].flag = 1;
}

dart_ret_t dart_convertform_remove (dart_team_t team)
{
	int i;
	dart_team_uniqueid (team, &i);
	convertform[i].flag = 0;
}
