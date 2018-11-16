#ifndef DART__BASE__INTERNAL__TASKING_WAIT_H__
#define DART__BASE__INTERNAL__TASKING_WAIT_H__

#include <dash/dart/if/dart_communication.h>
#include <dash/dart/base/macro.h>

void
dart__task__wait_init() DART_INTERNAL;

void
dart__task__wait_fini() DART_INTERNAL;

dart_ret_t
dart__task__wait_handle(
  dart_handle_t *handles,
  size_t         num_handle) DART_INTERNAL;

dart_ret_t
dart__task__detach_handle(
  dart_handle_t *handles,
  size_t         num_handle) DART_INTERNAL;

void
dart__task__wait_progress() DART_INTERNAL;

void
dart__task__wait_enqueue(dart_task_t *task) DART_INTERNAL;

#endif // DART__BASE__INTERNAL__TASKING_WAIT_H__
