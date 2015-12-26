/**
 *  \file dart_translation.c
 *
 *  Implementation for the operations on translation table.
 */

#include <dash/dart/base/logging.h>
#include <dash/dart/mpi/dart_translation.h>
#include <dash/dart/mpi/dart_mem.h>

/* Global array: the header for the global translation table. */
node_t dart_transtable_globalalloc;

MPI_Win dart_win_local_alloc;
#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
MPI_Win dart_sharedmem_win_local_alloc;
#endif

int dart_adapt_transtable_create ()
{
  dart_transtable_globalalloc = NULL;
  return 0;
}

int dart_adapt_transtable_add (info_t item)
{
  node_t pre, q;
  node_t p = (node_t) malloc (sizeof (node_info_t));
  DART_LOG_TRACE(
      "dart_adapt_transtable_add() item: "
      "seg_id:%d size:%lld disp:%lld win:%lld",
      item.seg_id, item.size, item.disp, item.win);
  //	printf ("item.seg_id is %d\n", item.seg_id);
  p -> trans.seg_id  = item.seg_id;
  p -> trans.size    = item.size;
  p -> trans.disp    = item.disp;
#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
  p -> trans.win     = item.win;
  p -> trans.baseptr = item.baseptr;
#endif
  p -> trans.selfbaseptr = item.selfbaseptr;
  p -> next = NULL;

  /* The translation table is empty. */
  if (dart_transtable_globalalloc == NULL) {
    dart_transtable_globalalloc = p;
  }

  /* Otherwise, insert the added item into the translation table. */
  else {
    q = dart_transtable_globalalloc;
    while (q != NULL) {
      pre = q;
      q = q -> next;
    }
    pre -> next = p;
  }
  return 0;
}

int dart_adapt_transtable_remove(int16_t seg_id)
{
  node_t pre, p;
  p = dart_transtable_globalalloc;
  if (seg_id == (p -> trans.seg_id)) {
    dart_transtable_globalalloc = p -> next;
  } else {
    while ((p != NULL ) && (seg_id != (p->trans.seg_id))) {
      pre = p;
      p = p -> next;
    }
    if (!p) {
      DART_LOG_ERROR(
        "Invalid seg_id: %d, can't remove the record from translation table",
        seg_id);
      return -1;
    }
    pre -> next = p -> next;
  }

  free (p->trans.disp);
#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
  if (p->trans.baseptr) {
    free (p->trans.baseptr);
  }
#endif
  free (p);
  return 0;
}

#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
int dart_adapt_transtable_get_win (int16_t seg_id, MPI_Win * win)
{
  node_t p;
  p = dart_transtable_globalalloc;

  while ((p != NULL) && (seg_id != ((p -> trans).seg_id))) {
    p = p -> next;
  }
  if (!p) {

    DART_LOG_ERROR ("Invalid seg_id: %d, can not get the related window object",
                    seg_id);
    return -1;
  }

  *win = (p -> trans).win;
  return 0;
}
#endif

/*
int dart_adapt_transtable_get_addr (uin16_t index, int offset, int* base, void **addr)
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

int dart_adapt_transtable_get_disp(int16_t seg_id,
                                   int rel_unitid,
                                   MPI_Aint * disp_s)
{
  MPI_Aint trans_disp = 0;
  node_t p = dart_transtable_globalalloc;
  *disp_s  = 0;

  DART_LOG_TRACE("dart_adapt_transtable_get_disp() "
                 "seq_id:%d rel_unitid:%d", seg_id, rel_unitid);
  while (p != NULL) {
    int16_t trans_seg_id = (p->trans).seg_id;
    DART_LOG_TRACE("dart_adapt_transtable_get_disp: p.trans.seq_id:%d",
                   trans_seg_id);
    if (seg_id == trans_seg_id) {
      break;
    }
    p = p->next;
  }
  if (p == NULL) {
    DART_LOG_ERROR(
      "Invalid seg_id: %d, can not get the related displacement", seg_id);
    return -1;
  }
  trans_disp = (p->trans).disp[rel_unitid];
  *disp_s    = trans_disp;
  DART_LOG_TRACE("dart_adapt_transtable_get_disp > dist:%lld", trans_disp);
  return 0;
}

#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
int dart_adapt_transtable_get_baseptr(
  int16_t    seg_id,
  int        rel_unitid,
  char   **  baseptr_s)
{
  node_t p;
  p = dart_transtable_globalalloc;

  while ((p != NULL) && (seg_id != ((p -> trans).seg_id))) {
    p = p -> next;
  }
  if (!p) {
    DART_LOG_ERROR("Invalid seg_id: %d, can not get the related baseptr",
                   seg_id);
    return -1;
  }

  *baseptr_s = (p -> trans).baseptr[rel_unitid];
  return 0;
}
#endif

int dart_adapt_transtable_get_selfbaseptr(
  int16_t    seg_id,
  char   **  baseptr)
{
  node_t p;
  p = dart_transtable_globalalloc;

  while ((p != NULL) && (seg_id != ((p -> trans).seg_id))) {
    p = p -> next;
  }
  if (!p) {
    DART_LOG_ERROR ("Invalid seg_id: %d, can not get the related baseptr",
                    seg_id);
    return -1;
  }
  *baseptr = (p -> trans).selfbaseptr;
  return 0;
}

int dart_adapt_transtable_get_size(
  int16_t   seg_id,
  size_t  * size)
{
  node_t p;
  p = dart_transtable_globalalloc;

  while ((p != NULL) && (seg_id != ((p -> trans).seg_id))) {
    p = p -> next;
  }

  if (!p) {
    DART_LOG_ERROR("Invalid seg_id: %d, can not get the related memory size",
                   seg_id);
    return -1;
  }
  *size = (p -> trans).size;
  return 0;
}

int dart_adapt_transtable_destroy ()
{
  node_t p, pre;
  p = dart_transtable_globalalloc;
  if (p != NULL) {
    DART_LOG_DEBUG("Free up the translation table forcibly as there are "
                   "still global memory blocks functioning on this team");
  }
  while (p != NULL) {
    pre = p;
    p = p -> next;

    free (pre->trans.disp);
#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
    if (pre->trans.baseptr) {
      free (pre->trans.baseptr);
    }
#endif
    free (pre);
  }
  dart_transtable_globalalloc = NULL;
  return 0;
}
