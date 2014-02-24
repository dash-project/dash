/** @file dart_adapt_teamnode.c
 *  @date 22 Nov 2013
 *  @brief Implementations for the operations on team hierarchical tree.
 */
#include<stdio.h>
#include<mpi.h>
#include"dart_types.h"
#include"dart_adapt_teamnode.h"

dart_teamnode_t dart_header; /* The header of team hierarchical tree. */

/* Initialize team hierarchy tree. */
dart_ret_t dart_adapt_teamnode_create ()
{
	int i;
	dart_header = (dart_teamnode_t) malloc (sizeof(struct dart_teamnode_struct));
	dart_header -> team_id = 0;
	dart_header -> parent = NULL;
	dart_header -> sibling = NULL;
	dart_header -> child = NULL;
	dart_header -> next_team_id[0] = 1;
	for (i = 1; i < MAX_TEAM; i++)
	{
		dart_header -> next_team_id[i] = 0;
	}
	dart_header -> mpi_comm = MPI_COMM_WORLD;
}

/* Destroy the team tree recursively. */

void dart_adapt_destroy (dart_teamnode_t node)
{
	if (node == NULL)
	{
		return;
	}
	dart_adapt_destroy (node -> child);
	dart_adapt_destroy (node -> sibling);
	free (node);
}

dart_ret_t dart_adapt_teamnode_destroy ()
{
	dart_adapt_destroy (dart_header);
	return DART_OK;
}

dart_ret_t dart_adapt_teamnode_query (dart_team_t teamid, dart_teamnode_t* node)
{
	int32_t parent_id = teamid.parent_id;
	int32_t team_id = teamid.team_id;
	int level = teamid.level;
	int i = 0;
	dart_teamnode_t p = dart_header;
	dart_teamnode_t pre;
	if (level == 0)
	{
		*node = dart_header;
		return DART_OK;
	}

	/* First retrieve the teamid with 'level'. */
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

	/* Continue searching along the sibling link. */
	if (p != NULL)
	{
		pre = p;

		while ( !((pre -> team_id == team_id) && (((pre -> parent) -> team_id) == parent_id)))
		{
			p = p -> sibling;
			if (p == NULL)
			{
				break;
			}
			pre = p;
		}
	
		if ((pre -> team_id == team_id) && (((pre -> parent) -> team_id) == parent_id))
		{
			*node = pre;
			return DART_OK; /* If finds, this function returns */
		}
		p = pre -> parent; /* Otherwise, return back to its parent node */
	}
	else 
	{
		p = pre; /* Return back to its parent node. */
	}
	
        /* Retrieve from node p (parent node) */	
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
				while (!((pre -> team_id == team_id) && (((pre -> parent) -> team_id) == parent_id)))
			        {
					p = p -> sibling;
					if (p == NULL)
					{
						break;
					}
					pre = p;
				}
				if ((pre -> team_id == team_id) && (((pre -> parent) -> team_id) == parent_id))
			 	{
					*node = pre;
					return DART_OK;
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
	printf ("ERROR! TEAM %2d: TEAMNODE_QUERY	- failed, no matched item\n", team_id);
}

dart_ret_t dart_adapt_teamnode_add (dart_team_t teamid, MPI_Comm comm, dart_team_t* newteam)
{
	int i, j;
	dart_teamnode_t p; 
	dart_adapt_teamnode_query (teamid, &p);
	dart_teamnode_t current = p;
	dart_teamnode_t pre;

	int id = p -> team_id;

	dart_teamnode_t q = (dart_teamnode_t) malloc(sizeof (struct dart_teamnode_struct));
	
	/* Find the nearest available team ID for the newly added team with parent team 
	 * specified by teamid */
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
	/* Set ith element in next_next_id to 1. */
	p -> next_team_id [i] = 1;
	q -> parent = p;
	q -> child = NULL;
	q -> sibling = NULL;
	q -> mpi_comm = comm;

	/* Initialize "next_team_id" array for the newly added team node. */
	for (j = 0; j < MAX_TEAM; j++)
	{
		if (j == i)
			q -> next_team_id [j] = 1;
		else
			q -> next_team_id [j] = 0;
	}

	newteam -> parent_id = p -> team_id;
	newteam -> team_id = q -> team_id;
	newteam -> level = teamid.level + 1;
	return DART_OK;
}

dart_ret_t dart_adapt_teamnode_remove (dart_team_t teamid)
{
	dart_teamnode_t  p;
	dart_teamnode_t q, p1, pre;

	dart_adapt_teamnode_query (teamid, &p);
	if (p -> child != NULL)
	{
		printf ("ERROR! TEAM %2d: TEAMNODEREMOVE	- not allowed as there are sub-teams under it!\n", teamid.team_id);
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
        
	/* Reset (p->team_id)th element of next_team_id. */
	q -> next_team_id [p -> team_id] = 0;
	free (p);
	return DART_OK;
}

