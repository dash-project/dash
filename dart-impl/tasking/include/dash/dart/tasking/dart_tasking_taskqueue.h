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
 * Pop a task from the HEAD of the task queue.
 *
 * The task queue should have been locked be the caller.
 */
dart_task_t *
dart_tasking_taskqueue_pop_unsafe(dart_taskqueue_t *tq) DART_INTERNAL;

/**
 * Push a task to the HEAD of the task queue.
 */
void
dart_tasking_taskqueue_push(
  dart_taskqueue_t *tq,
  dart_task_t      *task) DART_INTERNAL;

/**
 * Push a task to the HEAD of the task queue.
 *
 * The task queue should have been locked be the caller.
 */
void
dart_tasking_taskqueue_push_unsafe(
  dart_taskqueue_t *tq,
  dart_task_t      *task) DART_INTERNAL;

/**
 * Add a task to the back of the queue.
 */
void
dart_tasking_taskqueue_pushback(
  dart_taskqueue_t *tq,
  dart_task_t      *task) DART_INTERNAL;

/**
 * Add a task to the back of the queue.
 *
 * The task queue should have been locked be the caller.
 */
void
dart_tasking_taskqueue_pushback_unsafe(
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
 * Insert a task at an arbitrary position, starting at 0 as the head.
 *
 * The task queue should have been locked be the caller.
 */
void
dart_tasking_taskqueue_insert_unsafe(
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
 * Pop a task from the back of the task queue.
 * Used to steal tasks from other threads.
 *
 * The task queue should have been locked be the caller.
 */
dart_task_t *
dart_tasking_taskqueue_popback_unsafe(dart_taskqueue_t *tq) DART_INTERNAL;

/**
 * Remove the task \c task from the taskqueue \c tq.
 * The world will die in flames if \c task is not in \c tq, so handle with care!
 */
void
dart_tasking_taskqueue_remove(
  dart_taskqueue_t *tq,
  dart_task_t      *task) DART_INTERNAL;

/**
 * Remove the task \c task from the taskqueue \c tq.
 * The world will die in flames if \c task is not in \c tq, so handle with care!
 *
 * The task queue should have been locked be the caller.
 */
void
dart_tasking_taskqueue_remove_unsafe(
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
  return (tq->num_elem == 0);
}

/**
 * Move the tasks enqueued in \c src to the queue \c dst.
 * Tasks are prepended at the destination queue.
 */
void
dart_tasking_taskqueue_move(
  dart_taskqueue_t *dst,
  dart_taskqueue_t *src) DART_INTERNAL;

/**
 * Move the tasks enqueued in \c src to the queue \c dst.
 * Tasks are prepended at the destination queue.
 *
 * The task queue should have been locked be the caller.
 */
void
dart_tasking_taskqueue_move_unsafe(
  dart_taskqueue_t *dst,
  dart_taskqueue_t *src) DART_INTERNAL;

/**
 * Lock the task queue to perform larger operations atomically.
 *
 * Use in combination with the \c *_unsafe variants of the taskqueue operations.
 *
 * \sa dart__base__mutex_lock
 * \sa dart_tasking_taskqueue_unlock
 */
DART_INLINE
dart_ret_t
dart_tasking_taskqueue_lock(dart_taskqueue_t *tq)
{
  return dart__base__mutex_lock(&tq->mutex);
}

/**
 * Try to lock the task queue to perform larger operations atomically.
 *
 * Use in combination with the \c *_unsafe variants of the taskqueue operations.
 *
 * \sa dart__base__mutex_trylock
 * \sa dart_tasking_taskqueue_unlock
 */
DART_INLINE
dart_ret_t
dart_tasking_taskqueue_trylock(dart_taskqueue_t *tq)
{
  return dart__base__mutex_trylock(&tq->mutex);
}

/**
 * Unlock the task queue.
 *
 * \sa dart__base__mutex_unlock
 * \sa dart_tasking_taskqueue_trylock
 * \sa dart_tasking_taskqueue_lock
 *
 */
DART_INLINE
dart_ret_t
dart_tasking_taskqueue_unlock(dart_taskqueue_t *tq)
{
  return dart__base__mutex_unlock(&tq->mutex);
}

/**
 * Check whether the taskqueue contains a task of the given priority (or higher).
 */
DART_INLINE
bool
dart_tasking_taskqueue_has_prio_task(
  dart_taskqueue_t *tq,
  dart_task_prio_t  prio)
{
  for (int i = DART_PRIO_HIGH; i >= prio; --i) {
    if (tq->queues[i].head != NULL) return true;
  }
  return false;
}


/**
 * Finalize a task queue, releasing held resources.
 */
void
dart_tasking_taskqueue_finalize(dart_taskqueue_t *tq) DART_INTERNAL;


#endif /* DART_TASKING_TASKQUEUE_H_ */
