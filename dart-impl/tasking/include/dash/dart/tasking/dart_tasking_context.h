/*
 * Provide task context management functionality.
 */

#ifndef DART_TASKING_DART_TASKING_CONTEXT_H_
#define DART_TASKING_DART_TASKING_CONTEXT_H_

#include <dash/dart/base/macro.h>

// TODO: Make this a CMake variable?
#define USE_UCONTEXT 1


typedef void (context_func_t) (void*);

#ifdef USE_UCONTEXT
#include <ucontext.h>
typedef struct context_struct {
  ucontext_t      ctx;
  context_func_t *fn;
  void           *arg;
} context_t;
#else
// dummy type
typedef char context_t;
#endif

// opaque type
typedef struct context_list_s context_list_t;

/**
 * Initialize the task context store.
 */
void dart__tasking__context_init() DART_INTERNAL;

/**
 * Create a new context for a task to execute.
 */
context_t* dart__tasking__context_create(
  context_func_t *fn,
  void           *arg) DART_INTERNAL;

/**
 * Switch into \c new_ctx, storing the current context in \c oldcontext
 */
dart_ret_t
dart__tasking__context_swap(
  context_t* old_ctx,
  context_t* new_ctx) DART_INTERNAL;

/**
 * Release a context (after task finished execution).
 */
void dart__tasking__context_release(context_t* ctx) DART_INTERNAL;

/**
 * Invoke a context previously created using
 * \ref dart__tasking__context_create.
 */
void dart__tasking__context_invoke(context_t* ctx) DART_INTERNAL;

/**
 * Cleanup previously allocated contexts.
 */
void dart__tasking__context_cleanup() DART_INTERNAL;


#endif /* DART_TASKING_DART_TASKING_CONTEXT_H_ */
