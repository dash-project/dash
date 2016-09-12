/**
 * This is needed for usleep.
 * TODO: check whether this is OK to use!
 */
#define _XOPEN_SOURCE 500

#include <stdlib.h>
#include <unistd.h>
#include <mpi.h>

#include <dash/dart/if/dart_active_messages.h>
#include <dash/dart/if/dart_globmem.h>
#include <dash/dart/mpi/dart_translation.h>
#include <dash/dart/mpi/dart_team_private.h>
#include <dash/dart/mpi/dart_globmem_priv.h>

/**
 * TODO:
 *  1) Ensure proper locking of parallel threads!
 *  2) Should we allow for return values to be passed back?
 *  3) Check the impact of the spinlock on other network activity of the unit.
 */

struct dart_amsgq {
  dart_gptr_t  queue;     // the active message queue
  dart_gptr_t  lock;      // a lock to prevent race-conditions during processing
  dart_gptr_t  tailpos;   // the current position of the queue's tail
  char        *dbuf;      // a double buffer used during message processing to shorten lock times
  int          size;      // the size (in byte) of the message queue
  dart_team_t  team;
};

static const int lock_locked = 1;
static const int lock_free   = 0;

dart_amsgq_t
dart_amsg_openq(int size, dart_team_t team)
{
  int *addr;
  dart_unit_t unitid;
  dart_gptr_t lock;
  dart_gptr_t queue;
  dart_gptr_t tailpos;
  struct dart_amsgq *res = calloc(1, sizeof(struct dart_amsgq));
  res->size = size;
  res->dbuf = malloc(size);
  res->team = team;


  dart_team_myid (team, &unitid);

  // allocate the queue
  if (dart_team_memalloc_aligned(team, size, &(res->queue)) != DART_OK) {
    DART_LOG_ERROR("Failed to allocate %i byte for queue", size);
  }
  DART_GPTR_COPY(queue, res->queue);
  queue.unitid = unitid;
  dart_gptr_getaddr (queue, (void**)&addr);
  memset(addr, 0, size);

  // allocate the lock
  if (dart_team_memalloc_aligned(team, sizeof(int), &(res->lock)) != DART_OK) {
    DART_LOG_ERROR("Failed to allocate %i byte for lock", sizeof(int));
  }
  DART_GPTR_COPY(lock, res->lock);
  lock.unitid = unitid;
  if (dart_gptr_getaddr (lock, (void**)&addr) != DART_OK) {
    DART_LOG_ERROR("Failed to get local address for lock");
  }
  *addr = lock_free;

  // allocate the tailpos pointer
  dart_team_memalloc_aligned(team, sizeof(int), &(res->tailpos));
  DART_GPTR_COPY(tailpos, res->tailpos);
  tailpos.unitid = unitid;
  dart_gptr_getaddr (tailpos, (void**)&addr);
  *addr = 0;

  uint16_t index = lock.flags; // TODO: this needs fixing, see branch freeflags
  MPI_Win win = dart_win_lists[index];

  MPI_Win_flush(unitid, win);

  return res;
}


dart_ret_t
dart_amsg_trysend(dart_unit_t target, dart_amsgq_t amsgq, dart_amsg_t *msg)
{
  MPI_Aint lock_disp;
  MPI_Aint tailpos_disp;
  MPI_Aint queue_disp;
  uint16_t index = amsgq->lock.flags; // TODO: this needs fixing, see branch freeflags
  MPI_Win win = dart_win_lists[index];
  MPI_Win tailpos_win;
  int msg_size = (sizeof(msg->fn) + sizeof(msg->data_size) + msg->data_size);
  int remote_offset;
  MPI_Win queue_win;


  if (dart_adapt_transtable_get_disp (amsgq->lock.segid, target, &lock_disp) == -1
   || dart_adapt_transtable_get_disp (amsgq->tailpos.segid, target, &tailpos_disp) == -1
   || dart_adapt_transtable_get_disp (amsgq->queue.segid, target, &queue_disp) == -1)
  {
    return DART_ERR_INVAL;
  }

  // TODO: implement an alternative that works without shared memory support
  if (dart_adapt_transtable_get_win(amsgq->lock.segid, &tailpos_win) == -1 || tailpos_win == MPI_WIN_NULL)
  {
    DART_LOG_ERROR("Failed to query shared memory window. Shared memory support disabled in DART?");
    return DART_ERR_INVAL;
  }

  if (dart_adapt_transtable_get_win(amsgq->queue.segid, &queue_win) == -1 || queue_win == MPI_WIN_NULL)
  {
    DART_LOG_ERROR("Failed to query shared memory window of queue. Shared memory support disabled in DART?");
    return DART_ERR_INVAL;
  }

  //lock the tailpos window
  MPI_Win_lock(MPI_LOCK_EXCLUSIVE, target, 0, tailpos_win);

  // Add the size of the message to the tailpos at the target
  MPI_Fetch_and_op(&msg_size, &remote_offset, MPI_INT32_T, target, 0, MPI_SUM, tailpos_win);

  if ((remote_offset + msg_size) >= amsgq->size) {
    int tmp;
    // if not, revert the operation and free the lock to try again.
    MPI_Fetch_and_op(&remote_offset, &tmp, MPI_INT32_T, target, 0, MPI_REPLACE, tailpos_win);
    MPI_Win_unlock(target, tailpos_win);
    DART_LOG_INFO("Not enough space for message of size %i at unit %i (current offset %i)", msg_size, target, remote_offset);
    return DART_ERR_AGAIN;
  }

  // lock the target queue before releasing the tailpos window to avoid potential race conditions
  MPI_Win_lock(MPI_LOCK_EXCLUSIVE, target, 0, queue_win);
  MPI_Win_unlock(target, tailpos_win);

  // we now have a slot in the message queue
  queue_disp += remote_offset;
  size_t fnptr_size = sizeof(msg->fn);
  MPI_Put((void*)&(msg->fn), fnptr_size, MPI_BYTE, target, queue_disp, fnptr_size, MPI_BYTE, win);
  queue_disp += fnptr_size;
  MPI_Put(&(msg->data_size), sizeof(msg->data_size), MPI_BYTE, target, queue_disp, msg->data_size, MPI_BYTE, win);
  queue_disp += sizeof(msg->data_size);
  MPI_Put(msg->data, msg->data_size, MPI_BYTE, target, queue_disp, msg->data_size, MPI_BYTE, win);
  MPI_Win_flush(target, win);
  MPI_Win_unlock(target, queue_win);

  DART_LOG_INFO("Sent message of size %i to unit %i ending at offset %i", msg_size, target, remote_offset);

  return DART_OK;
}


dart_ret_t
dart_amsg_process(dart_amsgq_t amsgq)
{
  int unitid;
  dart_gptr_t tailposg;
  dart_gptr_t queueg;
  MPI_Aint lock_disp;
  MPI_Aint tailpos_disp;
  MPI_Aint queue_disp;
  uint16_t index = amsgq->lock.flags; // TODO: this needs fixing, see branch freeflags
//  MPI_Win win = dart_win_lists[index];
  MPI_Win tailpos_win;
  MPI_Win queue_win;
  int  *tailpos_addr;
  int   zero = 0;
  int   tailpos;
  void *queue;
  char *dbuf = amsgq->dbuf;

  dart_team_myid (amsgq->team, &unitid);

  if (dart_adapt_transtable_get_disp(amsgq->lock.segid, unitid, &lock_disp) == -1
   || dart_adapt_transtable_get_disp(amsgq->tailpos.segid, unitid, &tailpos_disp) == -1
   || dart_adapt_transtable_get_disp(amsgq->queue.segid, unitid, &queue_disp) == -1)
  {
    return DART_ERR_INVAL;
  }

  // TODO: implement an alternative that works without shared memory support
  if (dart_adapt_transtable_get_win(amsgq->lock.segid, &tailpos_win) == -1 || tailpos_win == MPI_WIN_NULL)
  {
    DART_LOG_ERROR("Failed to query shared memory window of tailpos. Shared memory support disabled in DART?");
    return DART_ERR_INVAL;
  }

  if (dart_adapt_transtable_get_win(amsgq->queue.segid, &queue_win) == -1 || queue_win == MPI_WIN_NULL)
  {
    DART_LOG_ERROR("Failed to query shared memory window of queue. Shared memory support disabled in DART?");
    return DART_ERR_INVAL;
  }

  MPI_Win_lock(MPI_LOCK_EXCLUSIVE, unitid, 0, tailpos_win);
//
//  DART_GPTR_COPY(tailposg, amsgq->tailpos);
//  tailposg.unitid = unitid;
//  dart_gptr_getaddr (tailposg, (void**)&tailpos_addr);
  MPI_Get(&tailpos, 1, MPI_INT32_T, unitid, 0, 1, MPI_INT32_T, tailpos_win);
  DART_LOG_INFO("Checking for new active messages (tailpos=%i)", tailpos);

  /**
   * process messages while they are available, i.e.,
   * repeat if messages come in while processing previous messages
   */
  if (tailpos > 0) {

    // lock the tailpos window
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, unitid, 0, queue_win);
    // copy the content of the queue for processing
    DART_GPTR_COPY(queueg, amsgq->queue);
    queueg.unitid = unitid;
    dart_gptr_getaddr (queueg, &queue);
    memcpy(dbuf, queue, tailpos);
    MPI_Win_unlock(unitid, queue_win);

    // reset the tailpos and release the lock on the message queue
    MPI_Put(&zero, 1, MPI_INT32_T, unitid, 0, 1, MPI_INT32_T, tailpos_win);
    MPI_Win_unlock(unitid, tailpos_win);

    // process the messages by invoking the functions on the data supplied
    int pos = 0;
    int startpos = pos;
    while (pos < tailpos) {
      startpos = pos;
      // unpack the message
      function_t *fn = *(function_t**)(dbuf + pos);
      pos += sizeof(fn);
      int data_size  = *(int*)(dbuf + pos);
      pos += sizeof(int);
      void *data     = dbuf + pos;
      pos += data_size;

      if (pos > tailpos) {
        DART_LOG_ERROR("Message out of bounds (expected %i but saw %i)\n", tailpos, pos);
        return DART_ERR_INVAL;
      }

      // invoke the message
      DART_LOG_INFO("Invoking active message %p on data %p of size %i starting from tailpos %i", fn, data, data_size, startpos);
      fn(data);
    }
  } else {
    MPI_Win_unlock(unitid, tailpos_win);
    DART_LOG_INFO("No messages to process.");
  }
  return DART_OK;
}

dart_ret_t
dart_amsg_closeq(dart_amsgq_t amsgq)
{
  free(amsgq->dbuf);
  amsgq->dbuf = NULL;

  dart_team_memfree(amsgq->team, amsgq->queue);
  dart_team_memfree(amsgq->team, amsgq->lock);
  dart_team_memfree(amsgq->team, amsgq->tailpos);

  free(amsgq);

  return DART_OK;
}
