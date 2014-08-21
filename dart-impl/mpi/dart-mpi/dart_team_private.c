/** @file dart_adapt_team_private.c
 *  @date 25 Mar 2014
 *  @brief Implementations for the operations on teamlist.
 */
#include<stdio.h>
#include "dart_types.h"
#include "dart_team_private.h"
#include "dart_team_group.h"

/* @brief Storing all the teamIDs that are already used. 
 * 
 *  Each element of this array has the possibility of indicating a specific teamID that is already used.
 *  if teamlist[i] = -1, which indicates that ith element is an empty slot and can be allocated.
 *  else teamlist[i] will equal to certain teamID and can't be rewritten.i
 */ 

MPI_Comm teams[MAX_TEAM_NUMBER];
MPI_Comm sharedmem_comm_list[MAX_TEAM_NUMBER];

//MPI_Win numa_win_list[MAX_TEAM_NUMBER];
MPI_Win win_lists[MAX_TEAM_NUMBER];
//int* dart_unit_mapping[MAX_TEAM_NUMBER];
int* dart_sharedmem_table[MAX_TEAM_NUMBER];
int dart_sharedmemnode_size[MAX_TEAM_NUMBER];

int dart_allocated_teamlist_size;
struct dart_free_teamlist_entry
{
	int index;
	struct dart_free_teamlist_entry* next;
};
typedef struct dart_free_teamlist_entry dart_free_entry;
typedef dart_free_entry* dart_free_teamlist_ptr;
dart_free_teamlist_ptr dart_free_teamlist_header;

struct dart_allocated_teamlist_entry
{
	int index;
	dart_team_t allocated_teamid;
};
typedef struct dart_allocated_teamlist_entry dart_allocated_entry;

dart_allocated_entry dart_allocated_teamlist_array[MAX_TEAM_NUMBER];

int dart_adapt_teamlist_init ()
{
	int i;
	dart_free_teamlist_ptr pre = NULL;
	dart_free_teamlist_ptr newAllocateEntry;
	for (i = 0; i < MAX_TEAM_NUMBER; i++)
	{
		newAllocateEntry = (dart_free_teamlist_ptr)malloc(sizeof (dart_free_entry));
		newAllocateEntry -> index = i;
		
		if (pre != NULL)
		{
			pre -> next = newAllocateEntry;
		}
		else
		{
			dart_free_teamlist_header = newAllocateEntry;
		}
		pre = newAllocateEntry;
	}
	newAllocateEntry -> next = NULL;

	dart_allocated_teamlist_size = 0;
	return 0;
}

int dart_adapt_teamlist_destroy ()
{
	int i;
	dart_free_teamlist_ptr pre, p = dart_free_teamlist_header;
	while (p)
	{
		pre = p;
		p = p -> next;
		free (pre);
	}
	dart_allocated_teamlist_size = 0;
	return 0;
}

int dart_adapt_teamlist_alloc (dart_team_t teamid, int* index)
{
	/*
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
	*/
	int i, j;
	dart_free_teamlist_ptr p;
	if (dart_free_teamlist_header != NULL)
	{
		*index = dart_free_teamlist_header -> index;
		p = dart_free_teamlist_header;
		
		dart_free_teamlist_header = dart_free_teamlist_header -> next;
		free (p);
		/*
		for (j = dart_allocated_teamlist_size - 1; (j>=0)&&(dart_allocated_teamlist_array[j].allocated_teamid > teamid); j--)
		{
			dart_allocated_teamlist_array[j + 1].index = dart_allocated_teamlist_array[j].index;
			dart_allocated_teamlist_array[j + 1].allocated_teamid = dart_allocated_teamlist_array[j].allocated_teamid;
		}
		dart_allocated_teamlist_array[j+1].index = *index;
		dart_allocated_teamlist_array[j+1].allocated_teamid = teamid;
		*/

		dart_allocated_teamlist_array[dart_allocated_teamlist_size].index = *index;
		dart_allocated_teamlist_array[dart_allocated_teamlist_size].allocated_teamid = teamid;
		dart_allocated_teamlist_size ++;
		return (dart_allocated_teamlist_size - 1);

	}
	else 
	{
		ERROR ("Out of bound: exceed the MAX_TEAM_NUMBER limit");
		return -1;
	}
}

int dart_adapt_teamlist_recycle (int index, int pos)
{
	int i;
	int unitid;
	dart_myid (&unitid);
//	printf ("unitid %d: recycle, index is %d, pos is %d\n", unitid, index, pos);
//	teamlist[index] = -1;
	dart_free_teamlist_ptr newAllocateEntry = (dart_free_teamlist_ptr)malloc (sizeof (dart_free_entry));
	newAllocateEntry -> index = index;
	newAllocateEntry -> next = dart_free_teamlist_header;
	dart_free_teamlist_header = newAllocateEntry;

	for (i = pos; i < dart_allocated_teamlist_size - 1; i ++)
	{
		dart_allocated_teamlist_array[i].allocated_teamid = dart_allocated_teamlist_array[i + 1].allocated_teamid;
		dart_allocated_teamlist_array[i].index = dart_allocated_teamlist_array[i + 1].index;
	}
	dart_allocated_teamlist_size --;
	return 0;	
}

int dart_adapt_teamlist_convert (dart_team_t teamid, int* index)
{
#if 0
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
		ERROR ("Invalid teamid input: %d", teamid);
		return -1;
	}
	/* Locate the teamid in the teamlist. */
	*index = i;
#endif
	if (teamid == DART_TEAM_ALL)
	{
		*index = 0;
		return 0;
	}
	int i, imin, imax;
	imin = 0;
	imax = dart_allocated_teamlist_size - 1;
	while (imin < imax)
	{
		int imid = (imin + imax) >> 1;
		if (dart_allocated_teamlist_array[imid].allocated_teamid < teamid)
		{
			imin = imid + 1;
		}
		else
		{
			imax = imid;
		}
	}
	if ((imax == imin) && (dart_allocated_teamlist_array[imin].allocated_teamid == teamid))
	{
		*index = dart_allocated_teamlist_array[imin].index;
		return imin;
	}
	else
	{
		ERROR ("Invalid teamid input: %d", teamid);
		return -1;	
	}
}
