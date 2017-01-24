
#include <dash/dart/base/logging.h>
#include <dash/dart/tasking/dart_tasking_tasklist.h>

// free-list for task_list elements
static task_list_t *free_task_list = NULL;


/**
 * Prepend the task to the tasklist.
 */
void dart_tasking_tasklist_prepend(task_list_t **tl, dart_task_t *task)
{
  if (tl == NULL) {
    DART_LOG_ERROR("Tasklist argument tl cannot be NULL!");
    return;
  }
  task_list_t *elem = dart_tasking_tasklist_allocate_elem();
  elem->task = task;
  elem->next = *tl;
  *tl = elem;
}


void dart_tasking_tasklist_fini() {
  while (free_task_list != NULL) {
    task_list_t *next = free_task_list->next;
    free_task_list->next = NULL;
    free(free_task_list);
    free_task_list = next;
  }
}

task_list_t * allocate_task_list_elem(){
  task_list_t *tl = NULL;
  if (free_task_list != NULL) {
    tl = free_task_list;
    free_task_list = free_task_list->next;
  } else {
    tl = calloc(1, sizeof(task_list_t));
  }
  return tl;
}

void deallocate_task_list_elem(task_list_t *tl) {
  tl->task = NULL;
  tl->next = free_task_list;
  free_task_list = tl;
}
