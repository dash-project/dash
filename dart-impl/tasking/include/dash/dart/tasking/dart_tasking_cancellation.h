#ifndef DART_TASKING_CANCELLATION_H_
#define DART_TASKING_CANCELLATION_H_

#include <dash/dart/tasking/dart_tasking_priv.h>
#include <dash/dart/base/macro.h>

void
dart__tasking__check_cancellation(dart_thread_t *thread) DART_INTERNAL;

bool
dart__tasking__cancellation_requested() DART_INTERNAL;

void
dart__tasking__cancel_task(dart_task_t *task) DART_INTERNAL;

void
dart__tasking__abort_current_task(dart_thread_t *thread) DART_NORETURN DART_INTERNAL;

void
dart__tasking__cancel_start() DART_INTERNAL;

void
dart__tasking__cancel_bcast() DART_NORETURN DART_INTERNAL;

void
dart__tasking__cancel_barrier() DART_NORETURN DART_INTERNAL;

void
dart__tasking__abort() DART_NORETURN DART_INTERNAL;

bool
dart__tasking__should_abort() DART_INTERNAL;



#endif // DART_TASKING_CANCELLATION_H_
