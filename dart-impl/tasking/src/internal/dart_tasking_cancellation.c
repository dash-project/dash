#include <dash/dart/base/logging.h>
#include <dash/dart/base/atomic.h>
#include <dash/dart/base/assert.h>
#include <dash/dart/if/dart_tasking.h>
#include <dash/dart/tasking/dart_tasking_taskqueue.h>
#include <dash/dart/tasking/dart_tasking_datadeps.h>
#include <dash/dart/tasking/dart_tasking_cancellation.h>
#include <dash/dart/tasking/dart_tasking_priv.h>
#include <dash/dart/tasking/dart_tasking_remote.h>

/**
 * Cancellation related functionality.
 */

// true if dart_task_cancele has been called
static volatile bool cancel_requested = 0;

static dart_team_t cancel_team;

void
dart__tasking__cancellation_init()
{
  dart_team_clone(DART_TEAM_ALL, &cancel_team);
}

void
dart__tasking__cancellation_fini()
{
  dart_team_destroy(&cancel_team);
}

bool
dart__tasking__cancellation_requested()
{
  return cancel_requested;
}

void
dart__tasking__cancel_task(dart_task_t *task)
{
  task->state = DART_TASK_CANCELLED;
  dart_tasking_datadeps_release_local_task(task, dart__tasking__current_thread());
  DART_DEC_AND_FETCH32(&task->parent->num_children);
  dart__tasking__destroy_task(task);
}

static void
cancel_thread_tasks(dart_thread_t *thread)
{
  dart_task_t* task;
#ifdef DART_TASK_THREADLOCAL_Q
  dart_taskqueue_t *target_queue = &thread->queue;
#else
  dart_taskqueue_t *target_queue = dart__tasking__get_taskqueue();
#endif
  while ((task = dart_tasking_taskqueue_pop(target_queue)) != NULL) {
    DART_LOG_TRACE("Cancelling task %p", task);
    dart__tasking__cancel_task(task);
  }
}

static void
dart__tasking__cancellation_barrier(dart_thread_t *thread) {
  // counter to spin on while waiting for threads to enter the barrier
  static volatile int thread_enter_counter  = 0;
  // counter to spin on while waiting for all threads to finish cancelling
  static volatile int thread_cancel_counter = 0;

  cancel_requested = 1;
  int num_threads = dart__tasking__num_threads();
  DART_LOG_TRACE("Thread %d entering cancellation_barrier", thread->thread_id);

  DART_INC_AND_FETCH32(&thread_enter_counter);
  while (DART_FETCH32(&thread_enter_counter) < num_threads) { }

  // wait for all units to arrive here to make sure we don't
  // release tasks too early
  if (thread->thread_id == 0) {
    dart_barrier(cancel_team);
  }

  while (dart__tasking__num_tasks()) {
    cancel_thread_tasks(thread);
    if (thread->thread_id == 0) {
      dart_tasking_remote_progress(cancel_team);
    }
  }

  // signal that we are done canceling our tasks
  DART_INC_AND_FETCH32(&thread_cancel_counter);
  if (thread->thread_id == 0) {
    // thread 0 resets the barrier after it's done
    // make sure all incoming requests have been served, serves as barrier
    while (DART_FETCH32(&thread_cancel_counter) < num_threads) { }
    dart_tasking_remote_progress_blocking(cancel_team);
    DART_LOG_TRACE("Finished with cancellation barrier!");
    thread_cancel_counter = 0;
    cancel_requested      = false;
  } else {
    // busy wait for the barrier release
    while(cancel_requested) { }
  }
}

void dart__tasking__abort_current_task(dart_thread_t* thread)
{
  // mark current task as cancelled
  thread->current_task->state = DART_TASK_CANCELLED;
  if (thread->current_task->state == DART_TASK_ROOT) {
    // aborting the main task means aborting the application
    DART_LOG_ERROR("Aborting the main task upon user request!");
    dart_abort(DART_EXIT_ABORT);
  }
  // jump back before the beginning of the task
  DART_LOG_DEBUG("abort_current_task: Aborting execution of task %p",
                 thread->current_task);
  longjmp(thread->current_task->taskctx->cancel_return, 1);
}

void
dart__tasking__cancel_bcast()
{
  DART_LOG_DEBUG("dart__tasking__cancel_bcast: cancelling remaining task "
                 "execution throgh broadcast!");
  if (!cancel_requested) {
    cancel_requested = true;
    // send cancellation request to all other units
    dart_tasking_remote_bcast_cancel(cancel_team);
  }
  dart_thread_t *thread = dart__tasking__current_thread();
  // jump back to the thread's main routine
  dart__tasking__abort_current_task(thread);
}

void
dart__tasking__cancel_barrier()
{
  DART_LOG_DEBUG("dart__tasking__cancel_global: cancelling remaining task "
                 "execution in collective call!");
  // signal cancellation
  cancel_requested = 1;
  dart_thread_t *thread = dart__tasking__current_thread();
  // jump back to the thread's main routine
  dart__tasking__abort_current_task(thread);
}

void
dart__tasking__abort()
{
  dart_thread_t *thread = dart__tasking__current_thread();
  DART_LOG_DEBUG("dart__tasking__abort: Aborting current task in thread %p",
                 thread);
  dart__tasking__abort_current_task(thread);
}

bool
dart__tasking__should_abort()
{
  // the current task should abort if cancellation was requested and
  // it's an actual task, not the root task (which cannot abort)
  return cancel_requested &&
              !dart__tasking__is_root_task(dart__tasking__current_task());
}

void
dart__tasking__cancel_start()
{
  // just start the cancellation process, nothing else to be done here
  DART_LOG_DEBUG("Received cancellation request (%i)", cancel_requested);
  cancel_requested = true;
}

void
dart__tasking__check_cancellation(dart_thread_t *thread) {
  if (cancel_requested) {
    if (!dart__tasking__is_root_task(dart__tasking__current_task())) {
      // abort task
      DART_LOG_DEBUG("Thread %d aborting task %p\n",
                     thread->thread_id, thread->current_task);
      dart__tasking__abort_current_task(thread);
    } else {
      dart__tasking__cancellation_barrier(thread);
    }
  }
}
