#include <dash/dart/if/dart_communication.h>
#include <dash/dart/tasking/dart_tasking_priv.h>
#include <dash/dart/tasking/dart_tasking_context.h>
#include <dash/dart/tasking/dart_tasking_wait.h>
#include <dash/dart/tasking/dart_tasking_priv.h>
#include <dash/dart/tasking/dart_tasking_taskqueue.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/base/mutex.h>
#include <dash/dart/base/assert.h>

#include <stdlib.h>

static dart_taskqueue_t handle_list;
static dart_taskqueue_t handle_list_tmp;


void
dart__task__wait_init()
{
#if defined(HAVE_RESCHEDULING_YIELD) && HAVE_RESCHEDULING_YIELD
  dart_tasking_taskqueue_init(&handle_list);
  dart_tasking_taskqueue_init(&handle_list_tmp);
#endif // HAVE_RESCHEDULING_YIELD
}

void
dart__task__wait_fini()
{
#if defined(HAVE_RESCHEDULING_YIELD) && HAVE_RESCHEDULING_YIELD
  dart_tasking_taskqueue_finalize(&handle_list);
  dart_tasking_taskqueue_finalize(&handle_list_tmp);
#endif // HAVE_RESCHEDULING_YIELD
}

static void
test_yield(dart_handle_t *handles, size_t num_handle)
{
  int32_t flag;
  if (num_handle == 1) {
    while (dart_test(handles, &flag) == DART_OK && !flag)
      dart_task_yield(0);
  } else {
    while (dart_testall(handles, num_handle, &flag) == DART_OK && !flag)
      dart_task_yield(0);
  }
}

dart_ret_t
dart__task__wait_handle(dart_handle_t *handles, size_t num_handle)
{
  // check whether the handles are all NULL
  bool all_null = true;
  for (size_t i = 0; i < num_handle; ++i) {
    if (handles[i] != DART_HANDLE_NULL) {
      all_null = false;
      break;
    }
  }

  if (all_null) return DART_OK;

#if defined(HAVE_RESCHEDULING_YIELD) && HAVE_RESCHEDULING_YIELD
  dart_task_t *current_task = dart_task_current_task();
  if (dart__tasking__is_root_task(current_task)) {
    // we cannot requeue the root task so we test-and-yield
    current_task->exec->wait_handle = NULL;
    test_yield(handles, num_handle);
  } else {
    dart_wait_handle_t *waithandle = malloc(sizeof(*waithandle) +
                                            sizeof(dart_handle_t)*num_handle);
    memcpy(waithandle->handle, handles, sizeof(*handles)*num_handle);
    waithandle->num_handle    = num_handle;
    current_task->exec->wait_handle = waithandle;
    // mark the task as waiting so that it won't be requeued immediately
    current_task->state = DART_TASK_BLOCKED;
    DART_LOG_TRACE("wait_handle: Blocking task %p (%p)",
                   current_task, current_task->exec->wait_handle);
    dart_task_yield(-1);
    if (current_task->exec->wait_handle != NULL) {
      DART_LOG_DEBUG("wait_handle: yield did not block task %p until completion, "
                     "falling back to test-yield!", current_task);
      free(current_task->exec->wait_handle);
      current_task->exec->wait_handle = NULL;
      current_task->state = DART_TASK_SUSPENDED;
      test_yield(handles, num_handle);
    }
    // TODO: check wait_handle field and fall back to yield-test cycles
    DART_LOG_TRACE("wait_handle: Resuming task %p (%p)",
                   current_task, current_task->exec->wait_handle);
  }

  return DART_OK;
#else
  return dart_waitall(handles, num_handles);
#endif // HAVE_RESCHEDULING_YIELD
}

void
dart__task__wait_progress()
{
  if (handle_list.num_elem > 0 &&
      dart_tasking_taskqueue_trylock(&handle_list_tmp) == DART_OK) {
    dart_task_t *task;
    // check each task from the handle_list and put it into a temporary
    // list if necessary
    while ((task = dart_tasking_taskqueue_pop(&handle_list)) != NULL) {
      int32_t flag;
      DART_LOG_TRACE("wait_handle: Testing task %p", task);
      dart_wait_handle_t *waithandle = task->exec->wait_handle;
      dart_testall(waithandle->handle, waithandle->num_handle, &flag);
      if (flag) {
        // all transfers finished, the task can be requeued
        task->state       = DART_TASK_SUSPENDED;
        task->exec->wait_handle = NULL;
        DART_LOG_TRACE("wait_handle: Unblocking task %p", task);
        dart__tasking__enqueue_runnable(task);
        free(waithandle);
      } else {
        // put the task into the new list for further waiting
        dart_tasking_taskqueue_pushback_unsafe(&handle_list_tmp, task);
      }
    }
    // move the remaining tasks to the main queue
    dart_tasking_taskqueue_lock(&handle_list);
    dart_tasking_taskqueue_move_unsafe(&handle_list, &handle_list_tmp);
    dart_tasking_taskqueue_unlock(&handle_list);
    dart_tasking_taskqueue_unlock(&handle_list_tmp);
  }
}

void
dart__task__wait_enqueue(dart_task_t *task)
{
  DART_LOG_TRACE("Enqueueing blocked task %p \n", task);
  DART_ASSERT(task->exec->wait_handle != NULL);
  dart_tasking_taskqueue_pushback(&handle_list, task);
}
