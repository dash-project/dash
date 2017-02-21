
#include <dash/dart/base/assert.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/tasking/dart_tasking_taskqueue.h>


void
dart_tasking_taskqueue_init(dart_taskqueue_t *tq)
{
  tq->head = tq->tail = NULL;
  dart_mutex_init(&tq->mutex);
}

dart_task_t *
dart_tasking_taskqueue_pop(dart_taskqueue_t *tq)
{
  dart_mutex_lock(&tq->mutex);
  dart_task_t *task = tq->head;
  if (tq->head != NULL) {
    if (tq->head == tq->tail) {
      DART_LOG_INFO("dart_tasking_taskqueue_pop: taking last element from queue tq:%p tq->head:%p", tq, tq->head);
      tq->head = tq->tail = NULL;
    } else {
      DART_LOG_INFO("dart_tasking_taskqueue_pop: taking element from queue tq:%p tq->head:%p tq->tail:%p", tq, tq->head, tq->tail);
      // simply advance the head pointer
      tq->head = task->next;
      // the head has no previous element
      tq->head->prev = NULL;
    }
    task->prev = NULL;
    task->next = NULL;
  }
  dart_mutex_unlock(&tq->mutex);
  return task;
}

void
dart_tasking_taskqueue_push(dart_taskqueue_t *tq, dart_task_t *task)
{
  if (task == NULL) {
    DART_ASSERT_MSG(task != NULL, "dart_tasking_taskqueue_push: task may not be NULL!");
    return;
  }
  if (task == tq->head) {
    DART_ASSERT_MSG(task != tq->head, "dart_tasking_taskqueue_push: task is already head of task queue");
    return;
  }
  task->next = NULL;
  task->prev = NULL;
  dart_mutex_lock(&tq->mutex);
  if (tq->head == NULL) {
    // task queue previously empty
    DART_LOG_INFO("dart_tasking_taskqueue_push: task %p to empty task queue tq:%p tq->head:%p", task, tq, tq->head);
    tq->head   = task;
    tq->tail   = tq->head;
  } else {
    DART_LOG_INFO("dart_tasking_taskqueue_push: task %p to task queue tq:%p tq->head:%p tq->tail:%p", task, tq, tq->head, tq->tail);
    task->next     = tq->head;
    tq->head->prev = task;
    tq->head       = task;
  }
  dart_mutex_unlock(&tq->mutex);
}

dart_task_t *
dart_tasking_taskqueue_popback(dart_taskqueue_t *tq)
{
  dart_task_t * task = NULL;
  if (tq->tail != NULL)
  {
    dart_mutex_lock(&tq->mutex);

    // re-check
    if (tq->tail != NULL)
    {
      DART_LOG_INFO("dart_tasking_taskqueue_popback: tq:%p tq->head:%p tq->tail=%p", tq, tq->head, tq->tail);
      task = tq->tail;
      tq->tail = task->prev;
      if (tq->tail == NULL) {
        // stealing the last element in the queue
        DART_LOG_INFO("dart_tasking_taskqueue_popback: last element from queue tq:%p tq->head:%p tq->tail=%p", tq, tq->head, tq->tail);
        tq->head = NULL;
      } else {
        tq->tail->next = NULL;
      }
      task->prev = NULL;
      task->next = NULL;
    }

    dart_mutex_unlock(&tq->mutex);
  }

  return task;
}

dart_ret_t
dart_tasking_taskqueue_move(dart_taskqueue_t *dst, dart_taskqueue_t *src)
{
  if (dst == NULL || src == NULL) {
    return DART_ERR_INVAL;
  }
  if (src->head != NULL && src->tail != NULL) {
    dart_mutex_lock(&dst->mutex);
    dart_mutex_lock(&src->mutex);

    if (src->head != NULL && src->tail != NULL) {
      /**
       * For now we simply prepend the src queue to the dest queue.
       * This assumes that the tasks in the src queue are hotter than the ones
       * in the dest queue.
       */

      src->tail->next = dst->head;
      dst->head->prev = src->tail;
      dst->head = src->head;
      src->tail = src->head = NULL;

    }
    dart_mutex_unlock(&dst->mutex);
    dart_mutex_unlock(&src->mutex);
  }
  return DART_OK;
}

void
dart_tasking_taskqueue_finalize(dart_taskqueue_t *tq)
{
  dart_mutex_destroy(&tq->mutex);
  tq->head = tq->tail = NULL;
}

