
#include <dash/dart/base/assert.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/tasking/dart_tasking_taskqueue.h>

/********************
 * Private method   *
 ********************/

static
dart_task_t *task_deque_pop(struct task_deque *deque);

dart_task_t *
dart_tasking_taskqueue_pop(dart_taskqueue_t *tq);


static
void task_deque_push(struct task_deque *deque, dart_task_t *task);

static
void task_deque_pushback(struct task_deque *deque, dart_task_t *task);

static
void task_deque_insert(
  struct task_deque *deque,
  dart_task_t       *task,
  unsigned int       pos);

static
dart_task_t * task_deque_popback(struct task_deque *deque);

static
dart_ret_t task_deque_move(struct task_deque *dst, struct task_deque *src);

/********************
 * Public methods   *
 ********************/

void
dart_tasking_taskqueue_init(dart_taskqueue_t *tq)
{
  tq->lowprio.head  = tq->lowprio.tail  = NULL;
  tq->highprio.head = tq->highprio.tail = NULL;
  dart__base__mutex_init(&tq->mutex);
}


void
dart_tasking_taskqueue_push(
  dart_taskqueue_t *tq,
  dart_task_t      *task)
{
  DART_ASSERT_MSG(task != NULL,
      "dart_tasking_taskqueue_push: task may not be NULL!");
  DART_ASSERT_MSG(task != tq->highprio.head && task != tq->lowprio.head,
    "dart_tasking_taskqueue_push: task %p is already head of task queue", task);
  task->next = NULL;
  task->prev = NULL;
  dart__base__mutex_lock(&tq->mutex);

  if (task->prio == DART_PRIO_HIGH) {
    task_deque_push(&tq->highprio, task);
  } else {
    task_deque_push(&tq->lowprio, task);
  }

  dart__base__mutex_unlock(&tq->mutex);
}

void
dart_tasking_taskqueue_pushback(
  dart_taskqueue_t *tq,
  dart_task_t      *task)
{
  DART_ASSERT_MSG(task != NULL,
      "dart_tasking_taskqueue_pushback: task may not be NULL!");
  task->next = NULL;
  task->prev = NULL;
  dart__base__mutex_lock(&tq->mutex);

  if (task->prio == DART_PRIO_HIGH) {
    task_deque_pushback(&tq->highprio, task);
  } else {
    task_deque_pushback(&tq->lowprio, task);
  }

  dart__base__mutex_unlock(&tq->mutex);
}

void
dart_tasking_taskqueue_insert(
  dart_taskqueue_t *tq,
  dart_task_t      *task,
  unsigned int      pos)
{
  DART_ASSERT_MSG(task != NULL,
      "dart_tasking_taskqueue_pushback: task may not be NULL!");
  task->next = NULL;
  task->prev = NULL;
  dart__base__mutex_lock(&tq->mutex);

  if (task->prio == DART_PRIO_HIGH) {
    task_deque_insert(&tq->highprio, task, pos);
  } else {
    task_deque_insert(&tq->lowprio, task, pos);
  }

  dart__base__mutex_unlock(&tq->mutex);
}


dart_task_t *
dart_tasking_taskqueue_popback(dart_taskqueue_t *tq)
{
  dart__base__mutex_lock(&tq->mutex);
  dart_task_t * task = task_deque_popback(&tq->highprio);
  if (task == NULL) {
    task = task_deque_popback(&tq->lowprio);
  }
  dart__base__mutex_unlock(&tq->mutex);

  return task;
}

dart_ret_t
dart_tasking_taskqueue_remove(dart_taskqueue_t *tq, dart_task_t *task)
{
  if (task != NULL) {
    dart__base__mutex_lock(&tq->mutex);
    dart_task_t *prev = task->prev;
    dart_task_t *next = task->next;
    if (prev != NULL) {
      prev->next = next;
    }
    if (next != NULL) {
      next->prev = prev;
    }
    task->next = task->prev = NULL;

    if (task == tq->highprio.head) {
      tq->highprio.head = next;
    } else if (task == tq->lowprio.head) {
      tq->lowprio.head = next;
    }

    if (task == tq->highprio.tail) {
      tq->highprio.tail = prev;
    } else if (task == tq->lowprio.tail) {
      tq->lowprio.tail = prev;
    }
    dart__base__mutex_unlock(&tq->mutex);
  }
}

dart_ret_t
dart_tasking_taskqueue_move(dart_taskqueue_t *dst, dart_taskqueue_t *src)
{
  if (dst == NULL || src == NULL) {
    return DART_ERR_INVAL;
  }
  dart__base__mutex_lock(&dst->mutex);
  dart__base__mutex_lock(&src->mutex);

  dart_ret_t ret;
  ret = task_deque_move(&dst->highprio, &src->highprio);
  if (ret != DART_OK) {
    dart__base__mutex_unlock(&dst->mutex);
    dart__base__mutex_unlock(&src->mutex);
    return ret;
  }

  ret = task_deque_move(&dst->lowprio, &src->lowprio);

  dart__base__mutex_unlock(&dst->mutex);
  dart__base__mutex_unlock(&src->mutex);
  return DART_OK;
}

void
dart_tasking_taskqueue_finalize(dart_taskqueue_t *tq)
{
  dart__base__mutex_destroy(&tq->mutex);
  tq->lowprio.head  = tq->lowprio.tail  = NULL;
  tq->highprio.head = tq->highprio.tail = NULL;
}


/********************
 * Private methods  *
 ********************/

static
dart_task_t *task_deque_pop(struct task_deque *deque)
{
  dart_task_t *task = deque->head;
  if (deque->head != NULL) {
    DART_ASSERT(deque->head != NULL && deque->tail != NULL);
    if (deque->head == deque->tail) {
      DART_LOG_TRACE(
          "dart_tasking_taskqueue_pop: taking last element from queue "
          "tq:%p tq->head:%p", deque, deque->head);
      deque->head = deque->tail = NULL;
    } else {
      DART_LOG_TRACE(
          "dart_tasking_taskqueue_pop: taking element from queue "
          "tq:%p tq->head:%p tq->tail:%p", deque, deque->head, deque->tail);
      // simply advance the head pointer
      deque->head = task->next;
      // the head has no previous element
      deque->head->prev = NULL;
    }
    task->prev = NULL;
    task->next = NULL;
  }
  return task;
}

dart_task_t *
dart_tasking_taskqueue_pop(dart_taskqueue_t *tq)
{
  dart__base__mutex_lock(&tq->mutex);
  dart_task_t *task = task_deque_pop(&tq->highprio);
  if (task == NULL) {
    task = task_deque_pop(&tq->lowprio);
  }
  dart__base__mutex_unlock(&tq->mutex);
  return task;
}


static
void task_deque_push(struct task_deque *deque, dart_task_t *task)
{
  if (deque->head == NULL) {
    // task queue previously empty
    DART_LOG_TRACE("dart_tasking_taskqueue_push: task %p to empty task queue "
        "tq:%p tq->head:%p", task, deque, deque->head);
    deque->head   = task;
    deque->tail   = deque->head;
  } else {
    DART_LOG_TRACE("dart_tasking_taskqueue_push: task %p to task queue "
        "tq:%p tq->head:%p tq->tail:%p", task, deque, deque->head, deque->tail);
    task->next     = deque->head;
    deque->head->prev = task;
    deque->head       = task;
  }
  DART_ASSERT(deque->head != NULL && deque->tail != NULL);
}

static
void task_deque_pushback(struct task_deque *deque, dart_task_t *task)
{
  if (deque->head == NULL) {
    // task queue previously empty
    DART_LOG_TRACE("dart_tasking_taskqueue_pushback: task %p to empty task queue "
        "tq:%p tq->head:%p", task, deque, deque->head);
    deque->head   = task;
    deque->tail   = deque->head;
  } else {
    DART_LOG_TRACE("dart_tasking_taskqueue_pushback: task %p to task queue "
        "tq:%p tq->head:%p tq->tail:%p", task, deque, deque->head, deque->tail);
    task->prev     = deque->tail;
    deque->tail->next = task;
    deque->tail       = task;
  }
  DART_ASSERT(deque->head != NULL && deque->tail != NULL);
}

static
void task_deque_insert(
  struct task_deque *deque,
  dart_task_t       *task,
  unsigned int       pos)
{
  // insert at front?
  if (pos == 0 || deque->head == NULL) {
    task_deque_push(deque, task);
    return;
  }

  unsigned int count = 0;
  dart_task_t *tmp = deque->head;
  // find the position to insert
  while (tmp != NULL && count++ < pos) {
    tmp = tmp->next;
  }

  // insert at back?
  if (tmp == NULL || tmp->next == NULL) {
    task_deque_pushback(deque, task);
    return;
  }

  task->next = NULL;
  task->prev = NULL;

  // insert somewhere in between!
  task->next       = tmp->next;
  task->next->prev = task;
  task->prev       = tmp;
  tmp->next        = task;

  DART_ASSERT(deque->head != NULL && deque->tail != NULL);
}

static
dart_task_t * task_deque_popback(struct task_deque *deque)
{
  dart_task_t * task = NULL;
  if (deque->tail != NULL)
  {
    DART_ASSERT(deque->head != NULL && deque->tail != NULL);
    DART_LOG_TRACE("dart_tasking_taskqueue_popback: "
        "tq:%p tq->head:%p tq->tail=%p", deque, deque->head, deque->tail);
    task = deque->tail;
    deque->tail = task->prev;
    if (deque->tail == NULL) {
      // stealing the last element in the queue
      DART_LOG_TRACE("dart_tasking_taskqueue_popback: last element from "
          "queue tq:%p tq->head:%p tq->tail=%p", deque, deque->head, deque->tail);
      deque->head = NULL;
    } else {
      deque->tail->next = NULL;
    }
    task->prev = NULL;
    task->next = NULL;
  }
  return task;
}


static
dart_ret_t task_deque_move(struct task_deque *dst, struct task_deque *src)
{
  if (src->head != NULL && src->tail != NULL) {

    if (src->head != NULL && src->tail != NULL) {

      if (dst->head != NULL) {
        src->tail->next = dst->head;
        dst->head->prev = src->tail;
      } else {
        dst->tail = src->tail;
      }
      dst->head = src->head;
      src->tail = src->head = NULL;
    }
  }
  return DART_OK;
}


