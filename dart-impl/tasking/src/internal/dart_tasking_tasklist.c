#include <dash/dart/base/assert.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/base/mutex.h>
#include <dash/dart/tasking/dart_tasking_tasklist.h>

// free-list for task_list elements
static task_list_t *free_task_list = NULL;
static dart_mutex_t mutex = DART_MUTEX_INITIALIZER;

/**
 * Prepend the task to the tasklist.
 */
void dart_tasking_tasklist_prepend(task_list_t **tl, dart_task_t *task)
{
  if (tl == NULL) {
    DART_LOG_ERROR("Tasklist argument tl cannot be NULL!");
    return;
  }

  if (task == NULL) {
    DART_ASSERT_MSG(task != NULL,
        "Huh? Better do not put a NULL task into a tasklist...");
    return;
  }

  task_list_t *elem = dart_tasking_tasklist_allocate_elem();
  elem->task = task;
  elem->next = *tl;
  *tl = elem;
}


bool dart_tasking_tasklist_contains(
  task_list_t           * tl,
  struct dart_task_data * task)
{
  for (task_list_t *elem = tl; elem != NULL; elem = elem->next) {
    if (elem->task == task) return true;
  }
  return false;
}

void dart_tasking_tasklist_remove(
  task_list_t           ** tl,
  struct dart_task_data *  task)
{
  task_list_t * prev = *tl;
  if (tl == NULL || *tl == NULL) return;
  if ((*tl)->task == task) {
    *tl = (*tl)->next;
    dart_tasking_tasklist_deallocate_elem(prev);
  } else {
    for (task_list_t *elem = (*tl)->next; elem != NULL; elem = elem->next) {
      if (elem->task == task) {
        prev->next = elem->next;
        dart_tasking_tasklist_deallocate_elem(elem);
        break;
      }
      prev = elem;
    }
  }
}

struct dart_task_data *
dart_tasking_tasklist_pop(task_list_t ** tl)
{
  if (tl == NULL || *tl == NULL) return NULL;

  task_list_t *elem = *tl;
  dart_task_t *task = elem->task;
  *tl = elem->next;
  dart_tasking_tasklist_deallocate_elem(elem);
  return task;
}

void dart_tasking_tasklist_fini()
{
  while (free_task_list != NULL) {
    task_list_t *next = free_task_list->next;
    free_task_list->next = NULL;
    free(free_task_list);
    free_task_list = next;
  }
}

task_list_t * dart_tasking_tasklist_allocate_elem()
{
  task_list_t *tl = NULL;
  if (free_task_list != NULL) {
    dart__base__mutex_lock(&mutex);
    if (free_task_list != NULL) {
      tl = free_task_list;
      free_task_list = free_task_list->next;
    }
    dart__base__mutex_unlock(&mutex);
  }
  if (tl == NULL){
    tl = calloc(1, sizeof(task_list_t));
  }
  return tl;
}

void dart_tasking_tasklist_deallocate_elem(task_list_t *tl)
{
  tl->task = NULL;
  dart__base__mutex_lock(&mutex);
  tl->next = free_task_list;
  free_task_list = tl;
  dart__base__mutex_unlock(&mutex);
}
