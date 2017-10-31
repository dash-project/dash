#ifndef DART__MPI__DART_ACTIVE_MESSAGES_H_
#define DART__MPI__DART_ACTIVE_MESSAGES_H_


#include <dash/dart/if/dart_types.h>
#include <stdbool.h>

struct dart_amsgq;

typedef struct dart_amsgq* dart_amsgq_t;

typedef void (*dart_task_action_t) (void *);

/**
 * Initialize an active message queue of size \c size on all units in team.
 *
 * This is a collective operation involving all units in team.
 *
 * \param msg_size   The maximum expected size of messages.
 *                   Messages may be smaller than this.
 *                   Used in combination with \c msg_count to determine
 *                   buffer sizes.
 * \param msg_count  The number of messages of size \c msg_size to reserve
 *                   space for.
 * \param team       The team of units used for the allocation.
 */
dart_ret_t
dart_amsg_openq(
  size_t         msg_size,
  size_t         msg_count,
  dart_team_t    team,
  dart_amsgq_t * queue);

/**
 * Try to send an active message to unit \c target through message queue \c amsgq.
 * At the target, a task will be created that executes \c fn with arguments \c data.
 * The argument data of size \c data_size will be copied to the target unit's message queue.
 * The call fails if there is not sufficient space available in the target's message queue.
 *
 * Implementation: The queue is shortly locked through a compare_and_swap atomic to determine
 *                 the position in the queue and update the queue tail using fetch_and_op.
 *                 The queue is then released before the actual message is copied.
 *
 * Note: Although the built-in notification of dart_locks, We do not use them since:
 *       1) One global lock would potentially harm performance (no two message queues could be used at the same time)
 *       2) We would need N locks for fine-grained locking (leading to O(N) required memory per unit and O(N^2) global memory)
 *
 * Note: All data required to execute the function contained in the active message must be contained in the data argument,
 *       i.e., no external references can be handled at the moment.
 */
dart_ret_t
dart_amsg_trysend(
    dart_team_unit_t    target,
    dart_amsgq_t        amsgq,
    dart_task_action_t  fn,
    const void         *data,
    size_t              data_size);

/**
 * Send an active message to all units in \c team queue.
 * The call blocks until all messages have been delivered.
 */
dart_ret_t
dart_amsg_bcast(
    dart_team_t         team,
    dart_amsgq_t        amsgq,
    dart_task_action_t  fn,
    const void         *data,
    size_t              data_size);

/**
 * If available, dequeue all messages in the local queue by calling the function and on the supplied data argument (see dart_amsg_t).
 *
 * Implementation: The local queue will be locked (again using compare_and_swap) to grab a copy of the current content
 *                 and released before processing starts.
 */
dart_ret_t
dart_amsg_process(dart_amsgq_t amsgq);

/**
 * If available, dequeue all messages in the local queue by calling the function and on the supplied data argument (see dart_amsg_t).
 *
 * Implementation: The local queue will be locked (again using compare_and_swap) to grab a copy of the current content
 *                 and released before processing starts.
 *
 * This function is similar to \c dart_amsgq_process except that it blocks
 * until processing can be performed in case another thread is currently
 * processing messages.
 */
dart_ret_t
dart_amsg_process_blocking(dart_amsgq_t amsgq, dart_team_t team);

/**
 * Collective operation on all members of the team involved in the active message queue.
 * Synchronizes all units in the team and processes all remaining messages.
 */
dart_ret_t
dart_amsg_sync(dart_amsgq_t amsgq);

/**
 * Close the queue, discarding all remaining messages and deallocating all allocated memory.
 */
dart_ret_t
dart_amsg_closeq(dart_amsgq_t amsgq);


dart_team_t
dart_amsg_team(const dart_amsgq_t amsgq);


#endif /* DART__MPI__DART_ACTIVE_MESSAGES_H_ */
