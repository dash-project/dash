#include<stdio.h>
#include<mpi.h>
#include"dart_types.h"
#include"dart_teamnode.h"
dart_teamnode_t dart_teamnode_create()
{
	int i;
	dart_teamnode_t header = (dart_teamnode_t) malloc (sizeof(dart_teamnode));
	header -> team_id = 0;
	header -> parent = NULL;
	header -> sibling = NULL;
	header -> child = NULL;
	header -> next_team_id[0] = 1;
	for (i = 1; i < MAX_TEAM; i++)
	{
		header -> next_team_id[i] = 0;
	}
	header -> mpi_comm = MPI_COMM_WORLD;
//	printf ("dart_teamnode_t: the teamid is %d\n", header->team_id);
	return header;
}

/*void dart_teamnode_destroy()
{

}
*/
dart_teamnode_t dart_teamnode_query (dart_team_t team)
{
	int32_t parent_id = team.parent_id;
	int32_t key_id = team.team_id;
	int level = team.level;
//	printf ("dart_teamnode_query: parent_id = %d, key_id = %d, level = %d\n", parent_id, key_id, level);
	int i = 0;
	dart_teamnode_t p = dart_header;
	dart_teamnode_t pre;
	if (level == 0)
	{
		return dart_header;
	}
	while (i < level)
	{
		pre = p;
		p = p -> child;
		i ++;
		if (p == NULL)
		{
			break;
		}
	}
	if (p != NULL)
	{
		pre = p;

		while ( !((pre -> team_id == key_id) && (((pre -> parent) -> team_id) == parent_id)))
		{
			p = p -> sibling;
			if (p == NULL)
			{
		//		printf ("break\n");
				break;
			}
			pre = p;
		}
	
		if ((pre -> team_id == key_id) && (((pre -> parent) -> team_id) == parent_id))
		{
		
			return pre;
		}
		p = pre -> parent;
	}
	else 
	{
		p = pre;
	}
			
	while (p != NULL)
	{
		i = i - 1;
		if ((p -> sibling) != NULL)
		{
			p = p -> sibling;
			while (i < level)
		        {
				pre = p;
				p = p -> child;
				i ++;
				if (p == NULL)
				{
					break;
				}
			}
			if (p != NULL)
			{
				pre = p;
				while (!((pre -> team_id == key_id) && (((pre -> parent) -> team_id) == parent_id)))
			        {
					p = p -> sibling;
					if (p == NULL)
					{
						break;
					}
					pre = p;
				}
				if ((pre -> team_id == key_id) && (((pre -> parent) -> team_id) == parent_id))
			 	{
					return pre;
				}
			        p = pre -> parent;
			}
			else 
			{
				p = pre;
			}
		}
		else
		{
			p = p -> parent;
		}
	
	}
	printf ("error: not found yet!\n");
}

void dart_teamnode_add (dart_team_t team, MPI_Comm comm, dart_team_t * newteam)
{
//	printf ("dart_teamnode_add enter\n");
	dart_teamnode_t p = dart_teamnode_query (team);
	dart_teamnode_t current = p;
	dart_teamnode_t pre;
	int id = p -> team_id;
	int i;
	dart_teamnode_t q = (dart_teamnode_t) malloc(sizeof (dart_teamnode));
	for (i = 0; i < MAX_TEAM; i++)
	{
		if (current -> next_team_id [i] == 0)
		{
			break;
		}
	}
	if (current -> child == NULL)
	{
		pre = current;
		pre -> child = q;
	}
	else
	{
		current = current -> child;
		while (current != NULL)
		{
			pre = current;
			current = current -> sibling;
		}
		pre -> sibling =q;

	}
	q -> team_id = i;
	p -> next_team_id [i] = 1;
	q -> parent = p;
	q -> child = NULL;
	q -> sibling = NULL;
	q -> mpi_comm = comm;
	int j;
	for (j = 0; j < MAX_TEAM; j++)
	{
		if (j == i)
			q -> next_team_id [j] = 1;
		else
			q -> next_team_id [j] = 0;
	}
	newteam -> parent_id = p -> team_id;
	newteam -> team_id = q -> team_id;
	newteam -> level = team.level + 1;
}
void dart_teamnode_remove (dart_team_t team)
{

	dart_teamnode_t  p = dart_teamnode_query (team);
	dart_teamnode_t q, p1, pre;
	if (p -> child != NULL)
	{
		printf ("error: not allowed to be removed, there are sub-teams belonging to this removed team!\n");
	}
	else
	{
		q = p -> parent;
		if ((q -> child) == p)
		{
			q -> child = p -> sibling;
		}
		else
		{
			pre = q -> child;
			p1 = pre -> sibling;
			while (p1 != p)
			{
				pre = p1;
				p1 = p1 -> sibling;
			}
			pre -> sibling = p -> sibling;
		}
	}
	q -> next_team_id [p -> team_id] = 0;
	free (p);
}

