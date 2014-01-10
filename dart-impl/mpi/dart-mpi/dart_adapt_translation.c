/** @file dart_adapt_translation.c
 *  @date 21 Nov 2013
 *  @brief Implementation for the operations on translation table.
 */

#include "dart_adapt_translation.h"

/* Global array: the set of translation table headers for MAX_NUMBER teams. */
node_t transtable_globalalloc [MAX_NUMBER];

dart_ret_t dart_adapt_transtable_create (int uniqueid)
{
	int i;
	transtable_globalalloc [uniqueid] = NULL;
}

dart_ret_t dart_adapt_transtable_add (int uniqueid, info_t item)
{
	int i;
	node_t pre, q;
	node_t p = (node_t) malloc (sizeof (node_info_t));
	p -> trans.offset = item.offset;
	p -> trans.handle.win = item.handle.win;
	p -> next = NULL;
	int compare = item.offset;

	/* The translation table is empty. */
	if (transtable_globalalloc[uniqueid] == NULL)
	{
		transtable_globalalloc [uniqueid] = p;
	}

	/* Otherwise, insert into the translation table based upon offset. */
	else
	{
		q = transtable_globalalloc [uniqueid];
		while ((q != NULL) && (compare > (q->trans.offset)))
		{
			pre = q;
			q = q -> next;
		}
		p -> next = pre -> next;
		pre -> next = p;
	}
}

dart_ret_t dart_adapt_transtable_remove (int uniqueid, int offset)
{
	node_t p, pre;
	p = transtable_globalalloc [uniqueid];
  	if (offset == (p -> trans.offset))
	{
		transtable_globalalloc [uniqueid] = p -> next;
	}
        else
	{
	 	while ((p != NULL ) && (offset != (p->trans.offset)))
		{
			pre = p;
			p = p -> next;
		}
	 	pre -> next = p -> next;
	}
	free (p);
	return DART_OK;
}

dart_ret_t dart_adapt_transtable_query (int uniqueid, int offset, int *begin, MPI_Win* win)
{
	node_t p, pre;
	MPI_Win result_win;
	p = transtable_globalalloc [uniqueid];
	while ((p != NULL) && (offset >= (p -> trans.offset)))
	{
		pre = p;
		p = p -> next;
	}

	result_win = (pre -> trans).handle.win;
	*begin = pre -> trans.offset;/* "begin" indicates the base location of the memory region for the specified team. */
	*win = result_win;
	return DART_OK;
}
