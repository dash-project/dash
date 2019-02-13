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
#include <alloca.h>

static dart_taskqueue_t handle_list;            // queue of tasks needing progress
static dart_taskqueue_t handle_list_returning;  // queue of tasks returning to the handle_list
static dart_taskqueue_t handle_list_processing; // queue of tasks currently being processed

struct dart_wait_handle_s {
  union {
    DART_STACK_MEMBER_DEF;
    struct {
      size_t              num_handle;
      dart_handle_t       handle[];
    };
  };
};


#define WAIT_HANDLE_FREELIST_MAX_SIZE 16
static dart_stack_t waithandle_freelist[WAIT_HANDLE_FREELIST_MAX_SIZE];

static inline
dart_wait_handle_t *allocate_waithandle(int num_handle)
{
  dart_wait_handle_t * waithandle = NULL;
  DART_ASSERT_MSG(num_handle > 0, "Refusing to allocate empty waithandle!");
  if (num_handle > WAIT_HANDLE_FREELIST_MAX_SIZE ||
      (NULL == (waithandle = (dart_wait_handle_t *)
                  dart__base__stack_pop(&waithandle_freelist[num_handle-1])))) {
    waithandle = malloc(sizeof(*waithandle) + num_handle*sizeof(dart_handle_t));
  }
  return waithandle;
}

static inline
void release_waithandle(dart_wait_handle_t *waithandle)
{
  DART_ASSERT_MSG(waithandle->num_handle > 0,
                  "Refusing to release empty waithandle!");
  if (waithandle->num_handle > WAIT_HANDLE_FREELIST_MAX_SIZE) {
    free(waithandle);
  } else {
    dart__base__stack_push(&waithandle_freelist[waithandle->num_handle-1],
                           &DART_STACK_MEMBER_GET(waithandle));
  }
}

void
dart__task__wait_init()
{
#if defined(HAVE_RESCHEDULING_YIELD) && HAVE_RESCHEDULING_YIELD
  dart_tasking_taskqueue_init(&handle_list);
  dart_tasking_taskqueue_init(&handle_list_processing);
  dart_tasking_taskqueue_init(&handle_list_returning);
#endif // HAVE_RESCHEDULING_YIELD

  for (int i = 0; i < WAIT_HANDLE_FREELIST_MAX_SIZE; ++i) {
    dart__base__stack_init(&waithandle_freelist[i]);
  }
}

void
dart__task__wait_fini()
{
#if defined(HAVE_RESCHEDULING_YIELD) && HAVE_RESCHEDULING_YIELD
  dart_tasking_taskqueue_finalize(&handle_list);
  dart_tasking_taskqueue_finalize(&handle_list_returning);
  dart_tasking_taskqueue_finalize(&handle_list_processing);
#endif // HAVE_RESCHEDULING_YIELD
  dart_wait_handle_t *waithandle;
  for (int i = 0; i < WAIT_HANDLE_FREELIST_MAX_SIZE; ++i) {
    while (NULL != (waithandle = (dart_wait_handle_t *)
                    dart__base__stack_pop(&waithandle_freelist[i]))) {
      free(waithandle);
    }
  }
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
    current_task->wait_handle = NULL;
    test_yield(handles, num_handle);
  } else {
    dart_wait_handle_t *waithandle = allocate_waithandle(num_handle);
    memcpy(waithandle->handle, handles, sizeof(*handles)*num_handle);
    waithandle->num_handle    = num_handle;
    current_task->wait_handle = waithandle;
    // mark the task as waiting so that it won't be requeued immediately
    current_task->state = DART_TASK_BLOCKED;
    DART_LOG_TRACE("wait_handle: Blocking task %p (%p)",
                   current_task, current_task->wait_handle);
    dart_task_yield(-1);
    if (current_task->wait_handle != NULL) {
      DART_LOG_DEBUG("wait_handle: yield did not block task %p until completion, "
                     "falling back to test-yield!", current_task);
      release_waithandle(current_task->wait_handle);
      current_task->wait_handle = NULL;
      current_task->state = DART_TASK_SUSPENDED;
      test_yield(handles, num_handle);
    }
    // TODO: check wait_handle field and fall back to yield-test cycles
    DART_LOG_TRACE("wait_handle: Resuming task %p (%p)",
                   current_task, current_task->wait_handle);
  }

  return DART_OK;
#else
  return dart_waitall(handles, num_handles);
#endif // HAVE_RESCHEDULING_YIELD
}

#define NUM_CHUNK_HANDLE 64

static void process_handle_chunk(
  dart_task_t   **tasks,
  int             num_tasks,
  dart_handle_t * handles,
  int             num_handles)
{
  int32_t *flags = alloca(sizeof(int32_t)*num_handles);

  dart_testsome(handles, num_handles, flags);

  int c = 0;
  for (int i = 0; i < num_tasks; ++i) {
    dart_task_t *task = tasks[i];
    int task_completed = true;
    for (int j = 0; j < tasks[i]->wait_handle->num_handle; ++j) {
      if (!flags[c]) {
        task_completed = false;
      }
      ++c;
    }
    if (task_completed) {
      release_waithandle(task->wait_handle);
      task->wait_handle = NULL;
      if (task->state != DART_TASK_DETACHED) {
        // all transfers finished, the task can be requeued
        task->state       = DART_TASK_SUSPENDED;
        DART_LOG_TRACE("wait_handle: Unblocking task %p", task);
        dart__tasking__enqueue_runnable(task);
      } else {
        DART_LOG_TRACE("wait_handle: Releasing detached task %p", task);
        dart__tasking__release_detached(task);
      }
    } else {
      dart_tasking_taskqueue_pushback_unsafe(&handle_list_returning, task);
    }
  }
}

void
dart__task__wait_progress()
{
  if (handle_list.num_elem > 0 &&
      dart_tasking_taskqueue_trylock(&handle_list_returning) == DART_OK) {
    dart_task_t *task;
    // check each task from the handle_list and put it into a temporary
    // list if necessary
    dart_tasking_taskqueue_lock(&handle_list);
    dart_tasking_taskqueue_move(&handle_list_processing, &handle_list);
    dart_tasking_taskqueue_unlock(&handle_list);
    while (handle_list_processing.num_elem > 0) {
      // collect tasks and their handles to process as chunk
      dart_task_t *tasks[NUM_CHUNK_HANDLE];
      int num_tasks = 0;
      dart_handle_t handle[NUM_CHUNK_HANDLE];
      int num_handle = 0;
      while ((task = dart_tasking_taskqueue_pop_unsafe(&handle_list_processing)) != NULL) {
        if (task->wait_handle->num_handle > NUM_CHUNK_HANDLE) {
          process_handle_chunk(&task, 1,
                              task->wait_handle->handle,
                              task->wait_handle->num_handle);
        } else {
          if ((num_handle + task->wait_handle->num_handle) > NUM_CHUNK_HANDLE) {
            // put back into queue and try again after we processed the current chunk
            dart_tasking_taskqueue_push_unsafe(&handle_list_processing, task);
            break;
          }

          tasks[num_tasks++] = task;
          for (int i = 0; i < task->wait_handle->num_handle; ++i) {
            handle[num_handle++] = task->wait_handle->handle[i];
          }
        }
      }
      if (num_handle) {
        process_handle_chunk(tasks, num_tasks, handle, num_handle);
      }
    }
    // move the remaining tasks to the main queue
    if (handle_list_returning.num_elem > 0) {
      dart_tasking_taskqueue_lock(&handle_list);
      dart_tasking_taskqueue_move_unsafe(&handle_list, &handle_list_returning);
      dart_tasking_taskqueue_unlock(&handle_list);
    }
    dart_tasking_taskqueue_unlock(&handle_list_returning);
  }
}

void
dart__task__wait_enqueue(dart_task_t *task)
{
  DART_LOG_TRACE("Enqueueing blocked task %p \n", task);
  if (task->wait_handle == NULL || task->wait_handle->num_handle == 0) {
    release_waithandle(task->wait_handle);
    dart__tasking__release_detached(task);
  } else {
    dart_tasking_taskqueue_pushback(&handle_list, task);
  }
}


dart_ret_t
dart__task__detach_handle(
  dart_handle_t *handles,
  size_t         num_handle)
{
  dart_task_t *task = dart__tasking__current_task();

  // mark the task as detached
  dart__tasking__mark_detached(task);

  int num_nn_handles = 0;
  for (int i = 0; i < num_handle; ++i) {
    if (handles[i]) ++num_nn_handles;
  }

  if (num_nn_handles) {
    // register the task for waiting
    dart_wait_handle_t *waithandle = allocate_waithandle(num_nn_handles);
    int c = 0;
    for (int i = 0; i < num_handle; ++i) {
      if (handles[i]) waithandle->handle[c++] = handles[i];
    }

    waithandle->num_handle    = num_nn_handles;
    task->wait_handle = waithandle;
  }
  return DART_OK;
}
