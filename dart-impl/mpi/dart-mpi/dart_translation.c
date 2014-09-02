/** @file dart_translation.c
 *  @date 25 Aug 2014
 *  @brief Implementation for the operations on translation table.
 */

#ifndef ENABLE_LOG
#define ENABLE_LOG
#endif
#include "dart_deb_log.h"
#include "dart_translation.h"
#include "dart_mem.h"

/* Global array: the set of translation table headers for MAX_TEAM_NUMBER teams. */
node_t dart_transtable_globalalloc [DART_MAX_TEAM_NUMBER];

MPI_Win dart_win_local_alloc;
MPI_Win dart_sharedmem_win_local_alloc;


int dart_adapt_transtable_create (int index)
{
	dart_transtable_globalalloc [index] = NULL;
	return 0;
}

int dart_adapt_transtable_add (int index, info_t item)
{
	node_t pre, q;
	node_t p = (node_t) malloc (sizeof (node_info_t));
	p -> trans.offset = item.offset;
	p -> trans.size = item.size;
	p -> trans.handle.win = item.handle.win;
	p -> trans.disp = item.disp;
	p -> next = NULL;
	uint64_t compare = item.offset;

	/* The translation table is empty. */
	if (dart_transtable_globalalloc[index] == NULL)
	{
		dart_transtable_globalalloc [index] = p;
	}

	/* Otherwise, insert into the translation table based upon offset. */
	else
	{
		q = dart_transtable_globalalloc [index];
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

int dart_adapt_transtable_remove (int index, uint64_t offset)
{
	node_t p, pre;
	p = dart_transtable_globalalloc [index];
  	if (offset == (p -> trans.offset))
	{
		dart_transtable_globalalloc [index] = p -> next;
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
			ERROR ("Invalid offset: %llu,can't remove the record from translation table", offset);
			return -1;
		}
	 	pre -> next = p -> next;
	}

	free (p->trans.disp);
	free (p);
	return 0;
}

int dart_adapt_transtable_get_win (int index, uint64_t offset, uint64_t* base, MPI_Win* win)
{
	node_t p, pre;
	p = dart_transtable_globalalloc [index];
	
	while ((p != NULL) && (offset >= ((p -> trans).offset)))
	{
		pre = p;
		p = p -> next;
	}

	if (((pre -> trans).offset + (pre -> trans).size) <= offset)
	{
		ERROR ("Invalid offset: %llu, can not get the related window object", offset);
		return -1;
	}

	if (base != NULL)
	{
		*base = (pre -> trans).offset;/* "base" indicates the base location of the specified global memory region . */
	}

	*win = (pre -> trans).handle.win;
	return 0;;
}

/*
int dart_adapt_transtable_get_addr (int index, int offset, int* base, void **addr)
{
	node_t p, pre;
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


	*base = (pre -> trans).offset;
	*addr = (pre -> trans).addr;
	return 0;
}
*/

int dart_adapt_transtable_get_disp (int index, uint64_t offset, dart_unit_t rel_unitid, uint64_t* base, MPI_Aint *disp_s)
{
	node_t p, pre;
	p = dart_transtable_globalalloc [index];

	while ((p != NULL) && (offset >= ((p -> trans).offset)))
	{
		pre = p;
		p = p -> next;
	}
	if (((pre -> trans).offset + (pre -> trans).size) <= offset)
	{
		ERROR ("Invalid offset : %llu, can not get the related displacement", offset);
		return -1;
	}

	if (base != NULL)
	{
		*base = (pre -> trans).offset;
	}
	*disp_s = (pre -> trans).disp[rel_unitid];
	return 0;
}

int dart_adapt_transtable_destroy (int index)
{
	node_t p, pre;
	p = dart_transtable_globalalloc[index];
	if (p != NULL)
	{
		LOG ("Free up the translation table forcibly as there are still global memory blocks functioning on this team.");
	}
	while (p != NULL)
	{
		pre = p;
		p = p -> next;
		free (pre->trans.disp);
		free (pre);
	}
	dart_transtable_globalalloc[index] = NULL;
	return 0;
}
