
#include <dash/dart/tasking/dart_tasking_taskqueue.h>


void
dart_tasking_taskqueue_init(dart_taskqueue_t *tq)
{
  tq->head = tq->tail = NULL;
  pthread_mutex_init(&tq->mutex, NULL);
}

dart_task_t *
dart_tasking_taskqueue_pop(dart_taskqueue_t *tq)
{
  dart_task_t *task = tq->head;
  pthread_mutex_lock(&tq->mutex);
  if (tq->head != NULL) {
    if (tq->head == tq->tail) {
      task = tq->head;
      tq->head = tq->tail = NULL;
    } else {
      // simply advance the head pointer
      tq->head = task->next;
      // the head has no previous element
      tq->head->prev = NULL;
    }
    task->prev = NULL;
    task->next = NULL;
  }
  pthread_mutex_unlock(&tq->mutex);
  return task;
}

void
dart_tasking_taskqueue_push(dart_taskqueue_t *tq, dart_task_t *task)
{
  pthread_mutex_lock(&tq->mutex);
  if (tq->head == NULL) {
    // task queue previously empty
    tq->head   = task;
    tq->tail   = tq->head;
    task->next = NULL;
    task->prev = NULL;
  } else {
    task->next     = tq->head;
    tq->head->prev = task;
    tq->head       = task;
    task->prev     = NULL;
  }
  pthread_mutex_unlock(&tq->mutex);
}

dart_task_t *
dart_tasking_taskqueue_popback(dart_taskqueue_t *tq)
{
  dart_task_t * task = NULL;
  if (tq->tail != NULL)
  {
    pthread_mutex_lock(&tq->mutex);

    // re-check
    if (tq->tail != NULL)
    {
      task = tq->tail;
      tq->tail = task->prev;
      if (tq->tail == NULL) {
        // stealing the last element in the queue
        tq->head = NULL;
      }
      task->prev = NULL;
      task->next = NULL;
    }

    pthread_mutex_unlock(&tq->mutex);
  }

  return task;
}

void
dart_tasking_taskqueue_finalize(dart_taskqueue_t *tq)
{
  pthread_mutex_destroy(&tq->mutex);
  tq->head = tq->tail = NULL;
}

