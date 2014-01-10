#include <stdio.h>
#include <mpi.h>
#include "dart_translation.h"
node_t transtable_globalalloc [MAX_NUMBER];
dart_ret_t dart_transtable_create (int uniqueid)
{
	int i;
	transtable_globalalloc [uniqueid] = NULL;
}

dart_ret_t dart_transtable_add (int uniqueid, info_t item)
{
	int i;
	node_t pre, q;
	node_t p = (node_t) malloc (sizeof (node_info));
	p -> trans.offset = item.offset;
	p -> trans.handle.win = item.handle.win;
	p -> next = NULL;
//	printf ("dart_transtabl_add: item.offset = %d\n", item.offset);
	int compare = item.offset;
	if (transtable_globalalloc[uniqueid] == NULL)
	{
		transtable_globalalloc [uniqueid] = p;
	}
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

dart_ret_t dart_transtable_remove (int uniqueid, int offset)
{
	node_t p, pre;
	p = transtable_globalalloc [uniqueid];
  	if (offset == (p -> trans.offset))
	{
//		printf ("equal\n");
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
}

MPI_Win dart_transtable_query (int uniqueid, int offset, int *begin)
{
	node_t p, pre;
	p = transtable_globalalloc [uniqueid];
	while ((p != NULL) && (offset >= (p -> trans.offset)))
	{
		pre = p;
		p = p -> next;
	}
	MPI_Win win;
	win = (pre -> trans).handle.win;
	*begin = pre -> trans.offset;
	return win;
}
