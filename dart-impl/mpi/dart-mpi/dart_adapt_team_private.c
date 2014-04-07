/** @file dart_adapt_team_private.c
 *  @date 25 Mar 2014
 *  @brief Implementations for the operations on teamlist.
 */
#include<stdio.h>
#include"dart_types.h"
#include"dart_adapt_team_private.h"

int32_t teamlist[MAX_TEAM_NUMBER]; 

MPI_Comm teams[MAX_TEAM_NUMBER];

int dart_adapt_teamlist_init ()
{
	int i;
	for (i = 0; i < MAX_TEAM_NUMBER; i++)
	{
		teamlist[i] = -1;

	}
}


int dart_adapt_teamlist_alloc (dart_team_t teamid, int* index)
{
	int i;
	for (i = 0; i < MAX_TEAM_NUMBER; i++)
	{
		if (teamlist[i] == -1)
		{
			break;
		}
	}
	if (i == MAX_TEAM_NUMBER)
	{
		ERROR ("Out of bound: exceed the MAX_TEAM_NUMBER limit");
		return -1;
	}
	teamlist[i] = teamid;
	*index = i;
}

int dart_adapt_teamlist_recycle (int index)
{
	teamlist[index] = -1;
	return 0;	
}

int dart_adapt_teamlist_convert (dart_team_t teamid, int* index)
{
	int i;
	for (i = 0; i <MAX_TEAM_NUMBER; i++)
	{
		if (teamlist[i] == teamid)
		{
			break;
		}
	}
	if (i == MAX_TEAM_NUMBER)
	{
		ERROR ("Invalid teamid input");
		dart_adapt_exit ();
		return -1;
	}
	/* Locate the teamid in the teamlist. */
	*index = i;
}
