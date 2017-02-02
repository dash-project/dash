

#ifndef DART_TASKING_TASKLIST_H_
#define DART_TASKING_TASKLIST_H_

#include <dash/dart/tasking/dart_tasking_priv.h>


void dart_tasking_tasklist_init();

/**
 * Prepend the task to the tasklist.
 */
void dart_tasking_tasklist_prepend(task_list_t **tl, struct dart_task_data *task);

task_list_t * dart_tasking_tasklist_allocate_elem();

void dart_tasking_tasklist_deallocate_elem(task_list_t *tl);

void dart_tasking_tasklist_fini();

#endif /* DART_TASKING_TASKLIST_H_ */
