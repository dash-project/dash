
// whether to use mmap to allocate stack
#define USE_MMAP 1

#ifdef USE_MMAP
#define _GNU_SOURCE
#include <sys/mman.h>
#endif

#include <ucontext.h>
#include <errno.h>
#include <stddef.h>
#include <unistd.h>
#include <errno.h>

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
 *
 * TODO: make the choice of whether to use mmap automatically
 */

// Use 16K stack size per task
#define DEFAULT_TASK_STACK_SIZE (1<<14)

// the maximum number of ctx to store per thread
#define PER_THREAD_CTX_STORE 10

struct context_list_s {
  struct context_list_s *next;
  context_t              ctx;
  int                    length;
  char                  *stack;
#ifdef USE_MMAP
  // guard page, only used for mmap
  char                  *ub_guard;
  size_t                 size;
#endif
};


static size_t task_stack_size = DEFAULT_TASK_STACK_SIZE;
static size_t page_size;

static size_t
dart__tasking__context_pagesize()
{
  long sz = sysconf(_SC_PAGESIZE);
  return sz;
}

static inline size_t
dart__tasking__context_adjust_size(size_t size)
{
  size_t mask = page_size - 1;
  return (size + mask) & ~mask;
}

void dart__tasking__context_init()
{
  page_size = dart__tasking__context_pagesize();
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

static context_list_t *
dart__tasking__context_allocate()
{
#ifdef USE_MMAP
  // align to page boundary: first page contains struct data and pointer to
  //                         second page, the start of the stack
  size_t size = dart__tasking__context_adjust_size(sizeof(context_list_t))
              + dart__tasking__context_adjust_size(task_stack_size)
              + page_size;
  // TODO: allocate guard pages and mprotect() them?
  context_list_t *ctxlist = mmap(NULL, size,
                                 PROT_EXEC|PROT_READ|PROT_WRITE,
                                 MAP_PRIVATE|MAP_ANONYMOUS|MAP_STACK,
                                 -1, 0);
  ctxlist->stack    = ((char*)ctxlist) + page_size;
  ctxlist->ub_guard = ctxlist->stack + task_stack_size;
  ctxlist->size     = size;
  // mprotect upper-bound page
  if (mprotect(ctxlist->ub_guard, page_size, PROT_NONE) != 0) {
    DART_LOG_WARN("Failed(%d) to mprotect upper bound page of size %zu at %p: %s",
                  errno, page_size, ctxlist->ub_guard, strerror(errno));
  }
#else
  size_t size = sizeof(context_list_t) + task_stack_size;
  context_list_t *ctxlist = calloc(1, size);
  ctxlist->stack = (char*)(ctxlist + 1);
#endif
  return ctxlist;
}

static void
dart__tasking__context_free(context_list_t *ctxlist)
{
#ifdef USE_MMAP
  munmap(ctxlist, ctxlist->size);
#else
  free(ctxlist);
#endif
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
    context_list_t *ctxlist = dart__tasking__context_allocate();
    ctxlist->next = NULL;
    // initialize context and set stack
    getcontext(&ctxlist->ctx.ctx);
    ctxlist->ctx.ctx.uc_link           = NULL;
    ctxlist->ctx.ctx.uc_stack.ss_sp    = ctxlist->stack;
    ctxlist->ctx.ctx.uc_stack.ss_size  = task_stack_size;
    ctxlist->ctx.ctx.uc_stack.ss_flags = 0;
    res = &ctxlist->ctx;
  }

#ifdef DART_DEBUG
  // set the stack guards
  char *stack = (char*)res->ctx.uc_stack.ss_sp;
  *(uint64_t*)(stack) = 0xDEADBEEF;
  *(uint64_t*)(stack + task_stack_size - sizeof(uint64_t)) = 0xDEADBEEF;
#endif // DART_DEBUG

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
  char *stack = (char*)ctx->ctx.uc_stack.ss_sp;
  if (*(uint64_t*)(stack) != 0xDEADBEEF &&
      *(uint64_t*)(stack + task_stack_size - sizeof(uint64_t)) != 0xDEADBEEF)
  {
    DART_LOG_WARN(
        "Possible TASK STACK OVERFLOW detected! "
        "Consider changing the stack size via DART_TASK_STACKSIZE! "
        "(current stack size: %zu)", task_stack_size);
  }
#endif // DART_DEBUG

  // thread-local list, no locking required
  context_list_t *ctxlist = (context_list_t*)
                                (((char*)ctx) - offsetof(context_list_t, ctx));

  dart_thread_t* thread = dart__tasking__current_thread();
  if (thread->ctxlist != NULL && thread->ctxlist->length > PER_THREAD_CTX_STORE)
  {
    // don't keep too many ctx around
    dart__tasking__context_free(ctxlist);
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

  if (old_ctx->fn) {
    // make sure we do not call the entry function upon next swap
    old_ctx->fn = old_ctx->arg = NULL;
  }

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
    dart__tasking__context_free(ctxlist);
  }
#else
  DART_ASSERT_MSG(NULL, "Cannot call %s without UCONTEXT support!", __FUNCTION__);
#endif
}
