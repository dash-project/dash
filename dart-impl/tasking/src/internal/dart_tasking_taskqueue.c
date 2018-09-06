
#include <dash/dart/base/assert.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/tasking/dart_tasking_taskqueue.h>
#include <dash/dart/tasking/dart_tasking_datadeps.h>

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

static
dart_ret_t task_deque_filter_runnable(struct task_deque *deque);

/********************
 * Public methods   *
 ********************/

void
dart_tasking_taskqueue_init(dart_taskqueue_t *tq)
{
  for (dart_task_prio_t i = DART_PRIO_HIGH; i < __DART_PRIO_COUNT; i++) {
    tq->queues[i].head = tq->queues[i].tail = NULL;
  }
  tq->num_elem      = 0;
  dart__base__mutex_init(&tq->mutex);
}

void
dart_tasking_taskqueue_push(
  dart_taskqueue_t *tq,
  dart_task_t      *task)
{
  dart__base__mutex_lock(&tq->mutex);
  dart_tasking_taskqueue_push_unsafe(tq, task);
  dart__base__mutex_unlock(&tq->mutex);
}

void
dart_tasking_taskqueue_push_unsafe(
  dart_taskqueue_t *tq,
  dart_task_t      *task)
{
  DART_ASSERT_MSG(task != NULL,
      "dart_tasking_taskqueue_push: task may not be NULL!");
  DART_ASSERT(task->prio > DART_PRIO_PARENT && task->prio < __DART_PRIO_COUNT);

#ifdef DART_ENABLE_ASSERTIONS
  for (dart_task_prio_t i = DART_PRIO_HIGH; i < __DART_PRIO_COUNT; ++i) {
    DART_ASSERT_MSG(task != tq->queues[i].head,
      "dart_tasking_taskqueue_push: task %p is already head of task queue", task);
  }
#endif // DART_ENABLE_ASSERTIONS

  task->next = NULL;
  task->prev = NULL;

  task_deque_push(&tq->queues[task->prio], task);
  ++tq->num_elem;
}

dart_task_t *
dart_tasking_taskqueue_pop(dart_taskqueue_t *tq)
{
  if (tq->num_elem == 0) return NULL; // shortcut on empty q
  dart__base__mutex_lock(&tq->mutex);
  dart_task_t *task = dart_tasking_taskqueue_pop_unsafe(tq);
  dart__base__mutex_unlock(&tq->mutex);
  return task;
}

dart_task_t *
dart_tasking_taskqueue_pop_unsafe(dart_taskqueue_t *tq)
{
  if (0 == tq->num_elem){
    // another thread stole the task
    return NULL;
  }
  --(tq->num_elem);
  dart_task_t *task = NULL;
  for (dart_task_prio_t i = DART_PRIO_HIGH;
                        i < __DART_PRIO_COUNT && task == NULL;
                      ++i) {
    task = task_deque_pop(&tq->queues[i]);
  }
  DART_ASSERT(task != NULL);
  return task;
}


void
dart_tasking_taskqueue_pushback(
  dart_taskqueue_t *tq,
  dart_task_t      *task)
{
  dart__base__mutex_lock(&tq->mutex);
  dart_tasking_taskqueue_pushback_unsafe(tq, task);
  dart__base__mutex_unlock(&tq->mutex);
}

void
dart_tasking_taskqueue_pushback_unsafe(
  dart_taskqueue_t *tq,
  dart_task_t      *task)
{
  DART_ASSERT_MSG(task != NULL,
      "dart_tasking_taskqueue_pushback: task may not be NULL!");
  task->next = NULL;
  task->prev = NULL;

  task_deque_pushback(&tq->queues[task->prio], task);

  ++(tq->num_elem);
}

void
dart_tasking_taskqueue_insert(
  dart_taskqueue_t *tq,
  dart_task_t      *task,
  unsigned int      pos)
{
  dart__base__mutex_lock(&tq->mutex);
  dart_tasking_taskqueue_insert_unsafe(tq, task, pos);
  dart__base__mutex_unlock(&tq->mutex);
}

void
dart_tasking_taskqueue_insert_unsafe(
  dart_taskqueue_t *tq,
  dart_task_t      *task,
  unsigned int      pos)
{
  DART_ASSERT_MSG(task != NULL,
      "dart_tasking_taskqueue_pushback: task may not be NULL!");
  task->next = NULL;
  task->prev = NULL;

  task_deque_insert(&tq->queues[task->prio], task, pos);

  ++(tq->num_elem);
}

dart_task_t *
dart_tasking_taskqueue_popback(dart_taskqueue_t *tq)
{
  if (tq->num_elem == 0) return NULL; // shortcut on empty q
  dart__base__mutex_lock(&tq->mutex);
  dart_task_t * task = dart_tasking_taskqueue_popback_unsafe(tq);
  dart__base__mutex_unlock(&tq->mutex);
  return task;
}

dart_task_t *
dart_tasking_taskqueue_popback_unsafe(dart_taskqueue_t *tq)
{
  if (0 == tq->num_elem){
    // another thread stole the task
    return NULL;
  }
  --(tq->num_elem);
  dart_task_t *task = NULL;
  for (dart_task_prio_t i = DART_PRIO_HIGH;
                        i < __DART_PRIO_COUNT && task == NULL;
                      ++i) {
    task = task_deque_popback(&tq->queues[i]);
  }

  DART_ASSERT(task != NULL);
  return task;
}

void
dart_tasking_taskqueue_remove(dart_taskqueue_t *tq, dart_task_t *task)
{
  if (task != NULL) {
    dart__base__mutex_lock(&tq->mutex);
    dart_tasking_taskqueue_remove_unsafe(tq, task);
    dart__base__mutex_unlock(&tq->mutex);
  }
}

void
dart_tasking_taskqueue_remove_unsafe(dart_taskqueue_t *tq, dart_task_t *task)
{
  if (task != NULL) {
    dart_task_t *prev = task->prev;
    dart_task_t *next = task->next;
    if (prev != NULL) {
      prev->next = next;
    }
    if (next != NULL) {
      next->prev = prev;
    }
    task->next = task->prev = NULL;

    for (dart_task_prio_t i = DART_PRIO_HIGH; i < __DART_PRIO_COUNT; ++i) {
      if (task == tq->queues[i].head) {
        tq->queues[i].head = next;
        break;
      }
    }

    for (dart_task_prio_t i = DART_PRIO_HIGH; i < __DART_PRIO_COUNT; ++i) {
      if (task == tq->queues[i].tail) {
        tq->queues[i].tail = prev;
        break;
      }
    }

    --(tq->num_elem);
  }
}

void
dart_tasking_taskqueue_move(dart_taskqueue_t *dst, dart_taskqueue_t *src)
{
  dart__base__mutex_lock(&dst->mutex);
  dart__base__mutex_lock(&src->mutex);
  dart_tasking_taskqueue_move_unsafe(dst, src);
  dart__base__mutex_unlock(&src->mutex);
  dart__base__mutex_unlock(&dst->mutex);
}

void
dart_tasking_taskqueue_move_unsafe(dart_taskqueue_t *dst, dart_taskqueue_t *src)
{
  for (dart_task_prio_t i = DART_PRIO_HIGH; i < __DART_PRIO_COUNT; ++i) {
    task_deque_move(&dst->queues[i], &src->queues[i]);
  }
  dst->num_elem += src->num_elem;
  src->num_elem  = 0;
}

void
dart_tasking_taskqueue_finalize(dart_taskqueue_t *tq)
{
  DART_ASSERT(tq->num_elem == 0);
  dart__base__mutex_destroy(&tq->mutex);
  for (dart_task_prio_t i = DART_PRIO_HIGH; i < __DART_PRIO_COUNT; ++i) {
    tq->queues[i].head  = tq->queues[i].tail  = NULL;
  }
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
          "tq:%p tq->head:%p tq->tail:%p", deque, deque->head, deque->tail);
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
  // post condition
  DART_ASSERT((deque->head != NULL && deque->tail != NULL)
            || (deque->head == NULL && deque->tail == NULL));
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


static
dart_ret_t
task_deque_filter_runnable(
  struct task_deque *deque)
{
  dart_task_t *task  = deque->head;
  // find the first head that is not filtered
  while (task != NULL && !dart_tasking_datadeps_is_runnable(task)) {
    deque->head = task->next;
    if (task->next != NULL) task->next->prev = NULL;
    task->next = NULL;
  }
  // walk through the rest of the list
  task  = deque->head;
  while (task != NULL) {
    dart_task_t *next = task->next;
    if (!dart_tasking_datadeps_is_runnable(task)) {
      // unlink this task
      if (task->prev != NULL) {
        task->prev->next = task->next;
      }
      if (task->next != NULL) {
        task->next->prev = task->prev;
      }
      // we just drop the task, it will come again once it's runnable
    }
    task->next = task->prev = NULL;
    task = next;
  }
  return DART_OK;
}

