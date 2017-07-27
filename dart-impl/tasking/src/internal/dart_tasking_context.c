

#include <dash/dart/base/env.h>
#include <dash/dart/tasking/dart_tasking_priv.h>
#include <dash/dart/tasking/dart_tasking_context.h>

struct context_list_s {
  struct context_list_s *next;
  context_t              ctx;
};

// Use 16K stack size per task
#define DEFAULT_TASK_STACK_SIZE (1<<14)

static size_t task_stack_size = DEFAULT_TASK_STACK_SIZE;

void dart__tasking__context_init()
{
  if (dart__base__env__task_stacksize() > -1) {
    task_stack_size = dart__base__env__task_stacksize();
  }
}


context_t* dart__tasking__context_create()
{
#ifdef USE_UCONTEXT

  // look for already allocated contexts
  // thread-local list, no locking required
  context_t* res = NULL;
  dart_thread_t* thread = dart__tasking_current_thread();
  if (thread->ctxlist != NULL) {
    res = &thread->ctxlist->ctx;
    thread->ctxlist = thread->ctxlist->next;
  }

  if (res == NULL) {
    // allocate a new context
    context_list_t *ctxlist = malloc(sizeof(context_list_t) + task_stack_size);
    // initialize context and set stack
    getcontext(&ctxlist->ctx);
    ctxlist->ctx.uc_link           = NULL;
    ctxlist->ctx.uc_stack.ss_sp    = (char*)(ctxlist + 1);
    ctxlist->ctx.uc_stack.ss_size  = task_stack_size;
    ctxlist->ctx.uc_stack.ss_flags = 0;
    res = &ctxlist->ctx;
  }

#ifdef DART_DEBUG
  // set the stack guards
  char *stack = (char*)res->uc_stack.ss_sp;
  *(uint64_t*)(stack) = 0xDEADBEEF;
  *(uint64_t*)(stack + task_stack_size - sizeof(uint64_t)) = 0xDEADBEEF;
#endif

  return res;
#endif
  return NULL;
}

void dart__tasking__context_release(context_t* ctx)
{
#ifdef USE_UCONTEXT

#ifdef DART_DEBUG
  // check the stack guards
  char *stack = (char*)ctx->uc_stack.ss_sp;
  if (*(uint64_t*)(stack) != 0xDEADBEEF &&
      *(uint64_t*)(stack + task_stack_size - sizeof(uint64_t)) != 0xDEADBEEF)
  {
    DART_LOG_WARN(
        "Possible TASK STACK OVERFLOW detected! "
        "Consider changing the stack size via DART_TASK_STACKSIZE! "
        "(current stack size: %zu)", task_stack_size);
  }
#endif

  // thread-local list, no locking required
  context_list_t *ctxlist = (context_list_t*)
                                (((char*)ctx) - sizeof(context_list_t *));

  dart_thread_t* thread = dart__tasking_current_thread();
  ctxlist->next = thread->ctxlist;
  thread->ctxlist = ctxlist;
#endif
}

void dart__tasking__context_cleanup()
{
#ifdef USE_UCONTEXT
  dart_thread_t* thread = dart__tasking_current_thread();

  context_list_t *ctxlist = thread->ctxlist;

  while (ctxlist != NULL) {
    context_list_t *next = ctxlist->next;
    free(ctxlist);
    ctxlist = next;
  }
  thread->ctxlist = NULL;
#endif
}
