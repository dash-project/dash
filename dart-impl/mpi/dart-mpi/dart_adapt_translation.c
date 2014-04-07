/** @file dart_adapt_translation.c
 *  @date 25 Mar 2014
 *  @brief Implementation for the operations on translation table.
 */

#ifndef ENABLE_LOG
#define ENABLE_LOG
#endif
#include "dart_adapt_translation.h"
#include "dart_adapt_mem.h"

/* Global array: the set of translation table headers for MAX_TEAM_NUMBER teams. */
node_t transtable_globalalloc [MAX_TEAM_NUMBER];

MPI_Win win_local_alloc;

int dart_adapt_transtable_create (int index)
{
	int i;
	transtable_globalalloc [index] = NULL;
	return 0;
}

int dart_adapt_transtable_add (int index, info_t item)
{
	int i;
	node_t pre, q;
	node_t p = (node_t) malloc (sizeof (node_info_t));
	p -> trans.offset = item.offset;
	p -> trans.size = item.size;
	p -> trans.handle.win = item.handle.win;
	p -> next = NULL;
	int compare = item.offset;

	/* The translation table is empty. */
	if (transtable_globalalloc[index] == NULL)
	{
		transtable_globalalloc [index] = p;
	}

	/* Otherwise, insert into the translation table based upon offset. */
	else
	{
		q = transtable_globalalloc [index];
		while ((q != NULL) && (compare > (q->trans.offset)))
		{
			pre = q;
			q = q -> next;
		}
		p -> next = pre -> next;
		pre -> next = p;
	}
	return 0;
}

int dart_adapt_transtable_remove (int index, int offset)
{
	node_t p, pre;
	p = transtable_globalalloc [index];
  	if (offset == (p -> trans.offset))
	{
		transtable_globalalloc [index] = p -> next;
	}
        else
	{
	 	while ((p != NULL ) && (offset != (p->trans.offset)))
		{
			pre = p;
			p = p -> next;
		}
		if (p == NULL)
		{
			ERROR ("Can't remove the record from translation table");
			return -1;
		}
	 	pre -> next = p -> next;
	}

	free (p);
	return 0;
}

int dart_adapt_transtable_query (int index, int offset, int *begin, MPI_Win* win)
{
	node_t p, pre;
	MPI_Win result_win;
	p = transtable_globalalloc [index];
	while ((p != NULL) && (offset >= (p -> trans.offset)))
	{
		pre = p;
		p = p -> next;
	}

	if (pre -> trans.offset + pre -> trans.size <= offset)
	{
		return -1;
	}
	
	result_win = (pre -> trans).handle.win;
	*begin = pre -> trans.offset;/* "begin" indicates the base location of the memory region for the specified team. */
	*win = result_win;
	return 0;
}

int dart_adapt_transtable_destroy (int index)
{
	node_t p, pre;
	p = transtable_globalalloc[index];
	if (p != NULL)
	{
		LOG ("Free up the translation table forcibly as there are still global memory blocks functioning on this team.");
	}
	while (p != NULL)
	{
		pre = p;
		p = p -> next;
		free (pre);
	}
	transtable_globalalloc[index] = NULL;
	return 0;
}
