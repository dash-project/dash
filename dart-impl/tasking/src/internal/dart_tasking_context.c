
#include <ucontext.h>
#include <errno.h>
#include <stddef.h>

#include <dash/dart/base/env.h>
#include <dash/dart/base/assert.h>
#include <dash/dart/tasking/dart_tasking_priv.h>
#include <dash/dart/tasking/dart_tasking_context.h>

/**
 * Management of task contexts needed for proper yielding of tasks.
 *
 * NOTE: valgrind may report invalid read/write operations if tasks
 *       reference memory allocated in other contexts, i.e., stack variables
 *       passed as pointers to other tasks. This seems harmless, though.
 */

// Use 16K stack size per task
#define DEFAULT_TASK_STACK_SIZE (1<<14)

// the maximum number of ctx to store per thread
#define PER_THREAD_CTX_STORE 10

struct context_list_s {
  struct context_list_s *next;
  context_t              ctx;
  int                    length;
};


static size_t task_stack_size = DEFAULT_TASK_STACK_SIZE;

void dart__tasking__context_init()
{
  if (dart__base__env__task_stacksize() > -1) {
    task_stack_size = dart__base__env__task_stacksize();
  }
}

static void
dart__tasking__context_entry(void)
{
  context_list_t *ctxlist;
  dart_thread_t  *thread  = dart__tasking__current_thread();
  DART_STACK_POP(thread->ctxlist, ctxlist);

  context_func_t *fn  = ctxlist->ctx.fn;
  void           *arg = ctxlist->ctx.arg;
  ctxlist->ctx.fn = ctxlist->ctx.arg = NULL;

  // invoke function
  fn(arg);

  // fn should never return!
  DART_ASSERT_MSG(NULL, "task context invocation function returned!");
}


context_t* dart__tasking__context_create(context_func_t *fn, void *arg)
{
#ifdef USE_UCONTEXT

  // look for already allocated contexts
  // thread-local list, no locking required
  context_t* res = NULL;
  dart_thread_t* thread = dart__tasking__current_thread();
  if (thread->ctxlist != NULL) {
    res = &thread->ctxlist->ctx;
    thread->ctxlist->length = 0;
    thread->ctxlist = thread->ctxlist->next;
  }

  if (res == NULL) {
    // allocate a new context
    context_list_t *ctxlist = calloc(1, sizeof(context_list_t) + task_stack_size);
    ctxlist->next = NULL;
    // initialize context and set stack
    getcontext(&ctxlist->ctx.ctx);
    ctxlist->ctx.ctx.uc_link           = NULL;
    ctxlist->ctx.ctx.uc_stack.ss_sp    = (char*)(ctxlist + 1);
    ctxlist->ctx.ctx.uc_stack.ss_size  = task_stack_size;
    ctxlist->ctx.ctx.uc_stack.ss_flags = 0;
    res = &ctxlist->ctx;
  }

#ifdef DART_DEBUG
  // set the stack guards
  char *stack = (char*)res->uc_stack.ss_sp;
  *(uint64_t*)(stack) = 0xDEADBEEF;
  *(uint64_t*)(stack + task_stack_size - sizeof(uint64_t)) = 0xDEADBEEF;
#endif

  makecontext(&res->ctx, &dart__tasking__context_entry, 0, 0);
  res->fn  = fn;
  res->arg = arg;
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
                                (((char*)ctx) - offsetof(context_list_t, ctx));
  dart_thread_t* thread = dart__tasking__current_thread();
  if (thread->ctxlist != NULL && thread->ctxlist->length > PER_THREAD_CTX_STORE)
  {
    // don't keep too many ctx around
    free(ctxlist);
  } else {
    ctxlist->length = (thread->ctxlist != NULL) ? thread->ctxlist->length : 0;
    ctxlist->length++;
    DART_STACK_PUSH(thread->ctxlist, ctxlist);
  }
#else
  DART_ASSERT_MSG(NULL, "Cannot call %s without UCONTEXT support!", __FUNCTION__);
#endif
}

void dart__tasking__context_invoke(context_t* ctx)
{
#ifdef USE_UCONTEXT
  // first invocation --> prepend to thread's ctxlist
  if (ctx->fn) {
    dart_thread_t  *thread  = dart__tasking__current_thread();
    context_list_t *ctxlist = (context_list_t*)
                                (((char*)ctx) - offsetof(context_list_t, ctx));
    DART_STACK_PUSH(thread->ctxlist, ctxlist);
  }

  setcontext(&ctx->ctx);
#else
  DART_ASSERT_MSG(NULL, "Cannot call %s without UCONTEXT support!", __FUNCTION__);
#endif
}

dart_ret_t
dart__tasking__context_swap(context_t* old_ctx, context_t* new_ctx)
{
#ifdef USE_UCONTEXT
  // first invocation --> prepend to thread's ctxlist
  if (new_ctx->fn) {
    dart_thread_t  *thread  = dart__tasking__current_thread();
    context_list_t *ctxlist = (context_list_t*)
                             (((char*)new_ctx) - offsetof(context_list_t, ctx));
    DART_STACK_PUSH(thread->ctxlist, ctxlist);
  }

  old_ctx->fn = old_ctx->arg = NULL;

  int ret = swapcontext(&old_ctx->ctx, &new_ctx->ctx);
  if (ret == -1) {
    DART_LOG_ERROR("Call to swapcontext failed! (errno=%i)", errno);
    return DART_ERR_OTHER;
  } else {
    return DART_OK;
  }
#else
  DART_ASSERT_MSG(NULL, "Cannot call %s without UCONTEXT support!", __FUNCTION__);
#endif
}

void dart__tasking__context_cleanup()
{
#ifdef USE_UCONTEXT
  dart_thread_t* thread = dart__tasking__current_thread();

  while (thread->ctxlist) {
    context_list_t *ctxlist;
    DART_STACK_POP(thread->ctxlist, ctxlist);
    free(ctxlist);
  }
  thread->ctxlist = NULL;
#else
  DART_ASSERT_MSG(NULL, "Cannot call %s without UCONTEXT support!", __FUNCTION__);
#endif
}
