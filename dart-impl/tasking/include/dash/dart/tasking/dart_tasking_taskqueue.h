/*
 * dart_tasking_taskqueue.h
 *
 *  Created on: Jan 16, 2017
 *      Author: joseph
 */

#ifndef DART_TASKING_TASKQUEUE_H_
#define DART_TASKING_TASKQUEUE_H_

#include <pthread.h>

#include <dash/dart/tasking/dart_tasking_priv.h>

/**
 * Initialize a task queue.
 */
void
dart_tasking_taskqueue_init(dart_taskqueue_t *tq) DART_INTERNAL;

/**
 * Pop a task from the HEAD of the task queue.
 */
dart_task_t *
dart_tasking_taskqueue_pop(dart_taskqueue_t *tq) DART_INTERNAL;

/**
 * Push a task to the HEAD of the task queue.
 */
void
dart_tasking_taskqueue_push(dart_taskqueue_t *tq, dart_task_t *task) DART_INTERNAL;

/**
 * Add a task to the back of the queue.
 */
void
dart_tasking_taskqueue_pushback(
  dart_taskqueue_t *tq,
  dart_task_t      *task) DART_INTERNAL;

/**
 * Insert a task at an arbitrary position, starting at 0 as the head.
 */
void
dart_tasking_taskqueue_insert(
  dart_taskqueue_t *tq,
  dart_task_t *task,
  unsigned int pos) DART_INTERNAL;

/**
 * Pop a task from the back of the task queue.
 * Used to steal tasks from other threads.
 */
dart_task_t *
dart_tasking_taskqueue_popback(dart_taskqueue_t *tq) DART_INTERNAL;

/**
 * Remove the task \c task from the taskqueue \c tq.
 * The world will die in flames if \c task is not in \c tq, so handle with care!
 */
dart_ret_t
dart_tasking_taskqueue_remove(
  dart_taskqueue_t *tq,
  dart_task_t      *task) DART_INTERNAL;

/**
 * Check whether the task queue is empty.
 *
 * \return   0 if the task queue is not empty.
 *         !=0 if the task queue is empty.
 */
DART_INLINE
int
dart_tasking_taskqueue_isempty(const dart_taskqueue_t *tq)
{
  return (tq->lowprio.head == NULL && tq->highprio.head);
}

/**
 * Move the tasks enqueued in \c src to the queue dst.
 * Tasks are prepended at the destination queue.
 */
dart_ret_t
dart_tasking_taskqueue_move(
  dart_taskqueue_t *dst,
  dart_taskqueue_t *src) DART_INTERNAL;

/**
 * Finalize a task queue, releasing held resources.
 */
void
dart_tasking_taskqueue_finalize(dart_taskqueue_t *tq) DART_INTERNAL;


#endif /* DART_TASKING_TASKQUEUE_H_ */
