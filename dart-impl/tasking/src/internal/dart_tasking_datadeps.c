
#include <dash/dart/base/assert.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/base/mutex.h>
#include <dash/dart/base/atomic.h>
#include <dash/dart/base/stack.h>
#include <dash/dart/if/dart_tasking.h>
#include <dash/dart/tasking/dart_tasking_priv.h>
#include <dash/dart/tasking/dart_tasking_tasklist.h>
#include <dash/dart/tasking/dart_tasking_datadeps.h>
#include <dash/dart/tasking/dart_tasking_remote.h>
#include <dash/dart/tasking/dart_tasking_taskqueue.h>
#include <dash/dart/tasking/dart_tasking_copyin.h>

#include <stdint.h>

//#define DART_DEPHASH_SIZE 1023
#define DART_DEPHASH_SIZE 511

/**
 * TODO:
 *   - Extract implementation of list and hash table
 *   - Clarify the locking in the hash table: do we need element-wise locks?
 */

/**
 * Management of task data dependencies using a hash map that maps pointers to tasks.
 * The hash map implementation is taken from dart_segment.
 * The hash uses the absolute local address stored in the gptr since that is used
 * throughout the task handling code.
 */

#define IS_OUT_DEP(taskdep) \
  (((taskdep).type == DART_DEP_OUT || (taskdep).type == DART_DEP_INOUT))

#define DEP_ADDR(dep) \
  ((dep).gptr.addr_or_offs.addr)

#define DEP_ADDR_EQ(dep1, dep2) \
  (DEP_ADDR(dep1) == DEP_ADDR(dep2))

// we know that the stack member entry is the first element of the struct
// so we can cast directly
#define DART_DEPHASH_ELEM_POP(__freelist) \
  (dart_dephash_elem_t*)((void*)dart__base__stack_pop(&__freelist))

#define DART_DEPHASH_ELEM_PUSH(__freelist, __elem) \
  dart__base__stack_push(&__freelist, &DART_STACK_MEMBER_GET(__elem))

/**
 * Represents a dependency in the dependency hash table.
 */
struct dart_dephash_elem {
  union {
    // atomic list used for free elements
    DART_STACK_MEMBER_DEF;
    // double linked list
    struct {
      dart_dephash_elem_t *next;
      dart_dephash_elem_t *prev;
    };
  };
  dart_dephash_elem_t      *next_in_task;   // list in the task struct
  dart_task_dep_t           dep;            // IN or OUT dependency information
  dart_dephash_elem_t      *dep_list;       // For OUT: start of list of assigned IN dependencies
                                            // For IN:  back-pointer to OUT dependency
  taskref                   task;           // task this dependency belongs to
  /*
   * TODO: this needs reconsideration, try to use atomics instead of locking
   *       the bucket and only lock the bucket when removing an element.
   * *Possible solution*
   *
   * When registering with an out dep:
   * CAS num_consumers *and* use_cnt to increment both and abort if
   * num_consumers is -1.
   *
   * When deregistering from an output dependency:
   * decrement num_consumers and if 0 CAS use_cnt *and* num_consumers to signal
   * ownership of the element (num_consumer=-1) and detect whether someone else
   * has registered with the outdep in between (in which case we do not remove
   * the object). Needs thorough testing!
  union {
    struct {
      int32_t               num_consumers;  // For OUT: the number of consumers still not completed
      int32_t               use_cnt;        // incremented everytime an input dependency is registered with an output dependency,
                                            // needed to detect race conditions when freeing an output dependency
    };
    uint64_t                refcnt_value;   // value used for CAS operations on the above values
  };
  */
  int32_t                   num_consumers;
  dart_global_unit_t        origin;         // the unit owning the task
  uint16_t                  owner_thread;   // the thread that owns the element
  //dart_tasklock_t           lock;           // lock used for element-wise locking
  bool                      is_dummy;       // whether an output dependency is not backed by a task
};

/**
 * Represents the head of a bucket in the dependency hash table.
 */
struct dart_dephash_head {
  dart_dephash_elem_t *head;
  dart_tasklock_t      lock;
  uint32_t             num_outdeps;
};

#define DART_DEPHASH_HEAD_INITIALIZER { \
  .head = NULL,                         \
  .lock = TASKLOCK_INITIALIZER,         \
  .num_outdeps = 0}

/**
 * Dependency hash element pool to speed up dependency handling.
 */
#define DART_DEPHASH_ELEM_POOL_SIZE (1024)

typedef struct dart_dephash_elem_pool dart_dephash_elem_pool_t;

struct dart_dephash_elem_pool {
  DART_STACK_MEMBER_DEF;
  uint32_t                  pos;
  dart_dephash_elem_t       elems[DART_DEPHASH_ELEM_POOL_SIZE];
};

/**
 * Per-thread memory pool.
 */
_Thread_local static dart_dephash_elem_pool_t *dephash_elem_pool = NULL;

/**
 * List of allocated dephash elem pools.
 */
static dart_stack_t dephash_elem_pool_list = DART_STACK_INITIALIZER;

#define DART_DEPHASH_ELEM_POOL_POP(__freelist) \
  (dart_dephash_elem_pool_t*)((void*)dart__base__stack_pop(&__freelist))

#define DART_DEPHASH_ELEM_POOL_PUSH(__freelist, __elem) \
  dart__base__stack_push(&__freelist, &DART_STACK_MEMBER_GET(__elem))

/**
 * Per-thread free-list for dependency elements. The elements are never
 * deallocated but stored in the free list for later re-use.
 */
_Thread_local static dart_stack_t dephash_elem_freelist = DART_STACK_INITIALIZER;

/**
 * Array of pointers to the thread-local dephash_elem_freelist, needed to return
 * elements to the correct owner.
 */
static dart_stack_t **dephash_elem_freelist_list;

// list of incoming remote dependency requests defered to matching step
static dart_dephash_elem_t *unhandled_remote_indeps  = NULL;
//static dart_dephash_elem_t *unhandled_remote_outdeps = NULL;
static dart_dephash_head_t  unhandled_remote_outdeps = DART_DEPHASH_HEAD_INITIALIZER;
static dart_mutex_t         unhandled_remote_mutex   = DART_MUTEX_INITIALIZER;

// list of tasks that have been deferred because they are in a phase
// that is not ready to run yet
dart_taskqueue_t            local_deferred_tasks; // visible outside this TU

static dart_global_unit_t myguid;

static dart_ret_t
release_remote_dependencies(dart_task_t *task);

static void
dephash_recycle_elem(dart_dephash_elem_t *elem);

static dart_ret_t
dart_tasking_datadeps_match_delayed_local_indep(
  const dart_task_dep_t * dep,
        dart_task_t     * task);

static inline int hash_gptr(dart_gptr_t gptr)
{
  // use larger types to accomodate for the shifts below
  // and unsigned to force logical shifts (instead of arithmetic)
  // NOTE: we ignore the teamid here because gptr in dependencies contain
  //       global unit IDs
  uint64_t hash   = gptr.addr_or_offs.offset;
  uint64_t unitid = gptr.unitid; // 64-bit required for shift
  // cut off the lower 2 bit, we assume that pointers are 4-byte aligned
  hash >>= 2;
  // mix in unit, team and segment ID
  hash ^= (unitid << 32); // 24 bit unit ID
  // using a prime number in modulo stirs reasonably well

  DART_LOG_TRACE("hash_gptr(u:%lu, o:%p) => (%lu)",
                 unitid, gptr.addr_or_offs.addr,
                 (hash % DART_DEPHASH_SIZE));

  return (hash % DART_DEPHASH_SIZE);
}

static inline bool release_local_dep_counter(dart_task_t *task) {
  int32_t num_local_deps  = DART_DEC_AND_FETCH32(&task->unresolved_deps);
  int32_t num_remote_deps = DART_FETCH32(&task->unresolved_remote_deps);
  DART_LOG_DEBUG("release_local_dep_counter : Task %p has %d local and %d "
                 "remote unresolved dependencies left", task, num_local_deps,
                 num_remote_deps);
  DART_ASSERT_MSG(num_remote_deps >= 0 && num_local_deps >= 0,
                  "Dependency counter underflow detected in task %p [%d,%d]!",
                  task, num_remote_deps, num_local_deps);
  return (num_local_deps == 0 && num_remote_deps == 0);
}

static inline bool release_remote_dep_counter(dart_task_t *task) {
  int32_t num_remote_deps = DART_DEC_AND_FETCH32(&task->unresolved_remote_deps);
  int32_t num_local_deps  = DART_FETCH32(&task->unresolved_deps);
  DART_LOG_DEBUG("release_remote_dep_counter : Task %p has %d local and %d "
                 "remote unresolved dependencies left", task, num_local_deps,
                 num_remote_deps);
  DART_ASSERT_MSG(num_remote_deps >= 0 && num_local_deps >= 0,
                  "Dependency counter underflow detected in task %p [%d,%d]!",
                  task, num_remote_deps, num_local_deps);
  return (num_local_deps == 0 && num_remote_deps == 0);
}

static inline void instrument_task_dependency(
  dart_task_t *first,
  dart_task_t *last,
  dart_gptr_t  gptr)
{
  // TODO: insert call to ayudame here
  //       gptr.addr_or_offs.addr contains the memory address of the dependency
  //       if this addr is NULL, it is a direct task dependency, please use
  //       AYU_UNKNOWN_MEMADDR in that case
}

/**
 * Initialize the data dependency management system.
 */
dart_ret_t dart_tasking_datadeps_init()
{
  dart_myid(&myguid);
  dart_tasking_taskqueue_init(&local_deferred_tasks);
  int num_threads = dart__tasking__num_threads() + DART_TASKING_MAX_UTILITY_THREADS;
  dephash_elem_freelist_list = calloc(num_threads,
                                        sizeof(*dephash_elem_freelist_list));
  return dart_tasking_remote_init();
}

static void
free_dephash_list(dart_dephash_elem_t *list)
{
  dart_dephash_elem_t *elem = list;
  while (elem != NULL) {
    dart_dephash_elem_t *tmp = elem->next;
    dephash_recycle_elem(elem);
    elem = tmp;
  }
}

dart_ret_t dart_tasking_datadeps_reset(dart_task_t *task)
{
  if (task == NULL || task->local_deps == NULL) return DART_OK;

  DART_LOG_TRACE("Cleaning up dependency objects of task %p", task);

#ifdef DART_ENABLE_ASSERTIONS
  for (int i = 0; i < DART_DEPHASH_SIZE; ++i) {
    DART_ASSERT_MSG(task->local_deps[i].head == NULL,
                    "Found non-empty bucket while tearing down hash table of "
                    "task %p (elem %p, i %d)", task, task->local_deps[i].head, i);
  }
  DART_ASSERT_MSG(task->remote_successor == NULL,
                  "Found pending remote successors of task %p (elem %p)",
                  task, task->remote_successor);
#endif // DART_ENABLE_ASSERTIONS

  free(task->local_deps);
  task->local_deps = NULL;

  return DART_OK;
}

dart_ret_t dart_tasking_datadeps_fini()
{
  dart_tasking_datadeps_reset(dart__tasking__current_task());
  dart_dephash_elem_pool_t *pool;
  while (NULL != (pool = DART_DEPHASH_ELEM_POOL_POP(dephash_elem_pool_list))) {
    free(pool);
  }
  free(dephash_elem_freelist_list);
  dephash_elem_freelist_list = NULL;
  dart_tasking_taskqueue_finalize(&local_deferred_tasks);
  return dart_tasking_remote_fini();
}

/**
 * Check for new remote task dependency requests coming in
 */
dart_ret_t dart_tasking_datadeps_progress()
{
  return dart_tasking_remote_progress();
}

static void
inline
dephash_list_insert_elem_after_nolock(
  dart_dephash_head_t *head,
  dart_dephash_elem_t *elem,
  dart_dephash_elem_t *prev)
{
  if (head->head == NULL) {
    // insert into empty bucket
    head->head = elem;
    elem->prev = NULL;
    elem->next = NULL;
  } else if (prev == NULL) {
    // insert at front of bucket
    elem->next = head->head;
    head->head->prev = elem;
    elem->prev = NULL;
    head->head = elem;
  } else {
    elem->next = prev->next;
    elem->prev = prev;
    prev->next = elem;
    if (elem->next != NULL) {
      elem->next->prev = elem;
    }
  }
  ++head->num_outdeps;
}


static void
inline
dephash_list_insert_elem_sorted_phase_desc_nolock(
  dart_dephash_head_t *head,
  dart_dephash_elem_t *elem)
{
  dart_dephash_elem_t *next, *iter;
  for (iter = head->head; iter != NULL; iter = next) {
    next = iter->next;
    if (iter->dep.phase <= elem->dep.phase) {
      break;
    }
  }
  dart_dephash_elem_t *prev = iter ? iter->prev : NULL;
  dephash_list_insert_elem_after_nolock(head, elem, prev);
}

/**
 * Allocate a new element for the dependency hash, possibly from a free-list
 */
static dart_dephash_elem_t *
dephash_allocate_elem(
  const dart_task_dep_t   *dep,
        taskref            task,
        dart_global_unit_t origin)
{
  // take an element from the free list if possible
  dart_dephash_elem_t *elem = NULL;
#ifdef DART_TASKING_NOMEMPOOL
  elem = malloc(sizeof(*elem));
#else // DART_TASKING_NOMEMPOOL
  elem = DART_DEPHASH_ELEM_POP(dephash_elem_freelist);
#endif // DART_TASKING_NOMEMPOOL

  if (elem == NULL){
    int thread_id = dart__tasking__thread_num();
    if (thread_id < 0) {
      // utility threads carry negative thread IDs so we need to adjust
      thread_id = -thread_id + dart__tasking__num_threads() - 1;
    }
    DART_ASSERT(thread_id >= 0 && thread_id < UINT16_MAX);
    // attempt to take an element from the pool
    if (dephash_elem_pool == NULL ||
        dephash_elem_pool->pos >= DART_DEPHASH_ELEM_POOL_SIZE) {
      // allocate a new pool and take from that
      dephash_elem_pool = malloc(sizeof(*dephash_elem_pool));
      dephash_elem_pool->pos = 0;
      // make sure this pool is registered for deallocation
      DART_DEPHASH_ELEM_POOL_PUSH(dephash_elem_pool_list, dephash_elem_pool);
      // upon first allocation, we also have to make sure that our freelist is
      // accessible from other threads
      if (dephash_elem_freelist_list[thread_id] == NULL) {
        dephash_elem_freelist_list[thread_id] = &dephash_elem_freelist;
      }
    }
    elem = &dephash_elem_pool->elems[dephash_elem_pool->pos++];
    elem->owner_thread = thread_id;
  }

  elem->task    = task;
  elem->origin  = origin;
  elem->dep     = *dep;
  elem->num_consumers = 0;
  elem->dep_list = NULL;
  //TASKLOCK_INIT(elem);
  elem->next = elem->prev = NULL;
  elem->is_dummy = false;

  DART_LOG_TRACE("Allocated elem %p (task %p)", elem, task.local);

  return elem;
}

static void
register_at_out_dep_nolock(
  dart_dephash_elem_t *out_elem,
  dart_dephash_elem_t *in_elem)
{
  in_elem->dep_list = out_elem;
  // register this dependency
  DART_STACK_PUSH(out_elem->dep_list, in_elem);
  //int nc = DART_INC_AND_FETCH32(&out_elem->num_consumers);
  int nc = ++out_elem->num_consumers;
  DART_LOG_TRACE("Registered in dep %p with out dep %p of task %p (num_consumers: %d)",
                 in_elem, out_elem, out_elem->task.local, nc);
  DART_ASSERT_MSG(nc > 0, "Dependency %p has negative number of consumers: %d!",
                  out_elem, nc);
}

static int32_t
deregister_in_dep_nolock(
  dart_dephash_elem_t *in_elem)
{
  dart_dephash_elem_t *out_elem = in_elem->dep_list;
  in_elem->dep_list = NULL;
  //int32_t nc = DART_DEC_AND_FETCH32(&out_elem->num_consumers);
  int nc = --out_elem->num_consumers;
  DART_LOG_TRACE("Deregistered in dep %p from out dep %p (consumers: %d)",
                 in_elem, out_elem, nc);
  DART_ASSERT_MSG(nc >= 0, "Dependency %p has negative number of consumers: %d",
                  out_elem, nc);
  return nc;
}

/**
 * Deallocate an element
 */
static void dephash_recycle_elem(dart_dephash_elem_t *elem)
{
  if (elem != NULL) {
    elem->next = NULL;
    elem->prev = NULL;
#ifdef DART_TASKING_NOMEMPOOL
    free(elem);
#else // DART_TASKING_NOMEMPOOL
    dart_stack_t *lifo = dephash_elem_freelist_list[elem->owner_thread];
    DART_LOG_TRACE("Pushing elem %p (prev=%p, next=%p) to freelist (head %p, thread %d)",
                   elem, elem->prev, elem->next, lifo->head.node, elem->owner_thread);
    DART_DEPHASH_ELEM_PUSH(*lifo, elem);
#endif // DART_TASKING_NOMEMPOOL
  }
}

static void dephash_require_alloc(dart_task_t *task)
{
  if (task != NULL && task->local_deps == NULL) {
    LOCK_TASK(task);
    if (task->local_deps == NULL) {
      // allocate new dependency hash table
      task->local_deps = calloc(DART_DEPHASH_SIZE, sizeof(dart_dephash_head_t));
    }
    UNLOCK_TASK(task);
  }
}

/**
 * Add a task with dependency to the parent's dependency hash table.
 * The dependency is added to the front of the bucket.
 */
static void
dephash_add_local_nolock(
  const dart_task_dep_t * dep,
        dart_task_t     * task,
        int               slot)
{
  dart_dephash_elem_t *new_elem = dephash_allocate_elem(dep, TASKREF(task), myguid);

  DART_STACK_PUSH_MEMB(task->deps_owned, new_elem, next_in_task);

  dart_task_t *parent = task->parent;
  dephash_require_alloc(parent);
  DART_LOG_TRACE("Adding elem %p of task %p to slot %d with head %p",
                 new_elem, task, slot, parent->local_deps[slot].head);
  // put the new entry at the beginning of the list
  dephash_list_insert_elem_after_nolock(&parent->local_deps[slot], new_elem, NULL);
}

static void dephash_add_local_out(
  const dart_task_dep_t * dep,
        dart_task_t     * task)
{
  int slot = hash_gptr(dep->gptr);
  dart_task_t *parent = task->parent;

  LOCK_TASK(&parent->local_deps[slot]);
  dephash_add_local_nolock(dep, task, slot);
  UNLOCK_TASK(&parent->local_deps[slot]);
}

static void dephash_remove_dep_from_bucket_nolock(
  dart_dephash_elem_t *elem,
  dart_dephash_head_t *local_deps,
  int                  slot)
{
  DART_LOG_TRACE("Removing elem %p (prev=%p, next=%p) from slot %d",
                 elem, elem->prev, elem->next, slot);

  if (elem->prev != NULL) {
    elem->prev->next = elem->next;
    if (elem->next != NULL) {
      elem->next->prev = elem->prev;
    }
  } else {
    // we have to change the head of the bucket
    local_deps[slot].head = elem->next;
    if (elem->next != NULL) {
      elem->next->prev = NULL;
    }
  }
  elem->next = elem->prev = NULL;
}

void dart__dephash__print_stats(const dart_task_t *task)
{
  uint32_t max_elems = 0;
  uint32_t min_elems = UINT32_MAX;
  uint32_t sum_elems = 0;
  uint32_t empty     = 0;
  uint32_t n = 0;
  double mean = 0.0, M2 = 0.0;
  for (uint32_t i = 0; i < DART_DEPHASH_SIZE; ++i) {
    uint32_t nb = task->local_deps[i].num_outdeps;
    if (nb == 0) {
      ++empty;
    } else {
      double delta, delta2;
      n++;
      delta  = nb - mean;
      mean  += delta / n;
      delta2 = nb - mean;
      M2    += delta2*delta2;
      if (nb > max_elems) max_elems = nb;
      if (nb < min_elems) min_elems = nb;
      sum_elems += nb;
    }
  }
  DART_LOG_INFO(
    "Task %p hash table: entries:%d, sum: %d, min: %d, max: %d, empty: %d, mean: %g, variance: %g",
    task, DART_DEPHASH_SIZE, sum_elems, min_elems, max_elems, empty, mean, M2/(n-1));
}

static void
release_dependency(dart_dephash_elem_t *elem)
{
  DART_ASSERT_MSG(elem->task.local != NULL,
                  "Cannot release dependency %p without task!", elem);
  if (elem->origin.id == myguid.id) {
    DART_LOG_TRACE("Releasing local %s dependency %p",
                   elem->dep.type == DART_DEP_IN ? "in" : "out", elem);
    bool runnable = release_local_dep_counter(elem->task.local);
    if (runnable) {
      dart__tasking__enqueue_runnable(elem->task.local);
    }
  } else {
    // send remote output dependency release together with reference
    // to the output dependency
    dart_tasking_remote_release_task(elem->origin, elem->task, (uintptr_t)elem);
  }
}

static void
dephash_release_next_out_dependency_nolock(
  dart_dephash_elem_t *elem)
{
  dart_dephash_elem_t *next_out_dep = elem;
  do {
    // dependencies in this slot are ordered by descending phase so we
    // walk backwards
    next_out_dep = next_out_dep->prev;
    if (next_out_dep == NULL) break;
    if (DEP_ADDR_EQ(next_out_dep->dep, elem->dep)) {
      release_dependency(next_out_dep);
      // done here
      break;
    }
  } while (1);
}

static void dephash_release_out_dependency(
  dart_dephash_elem_t *elem,
  dart_dephash_head_t *local_deps)
{
  DART_LOG_TRACE("Releasing output dependency %p (num_consumers %d)",
                 elem, elem->num_consumers);
  int slot = hash_gptr(elem->dep.gptr);
  LOCK_TASK(&local_deps[slot]);
  DART_ASSERT_MSG(elem->dep_list == NULL || elem->num_consumers > 0,
                  "Consumer-less output dependency has dependencies: %p",
                  elem->dep_list);
  if (elem->dep_list != NULL) {
    dart_dephash_elem_t *dep_list = elem->dep_list;
    elem->task.local = NULL;
    elem->dep_list   = NULL;
    // unlock the slot here, no need to keep the lock
    // NOTE: keeping the lock can be dangerous as release_dependency might trigger
    //       operations on the hash table (inserting remote dependencies), which can
    //       lead to a deadlock
    UNLOCK_TASK(&local_deps[slot]);
    do {
      dart_dephash_elem_t *in_dep;

      DART_STACK_POP(dep_list, in_dep);
      if (NULL == in_dep) break;
      DART_LOG_TRACE("  -> Releasing input dependency %p from out %p",
                    in_dep, elem);
      DART_ASSERT_MSG(in_dep->dep.type == DART_DEP_IN || in_dep->dep.type == DART_DEP_COPYIN,
                      "Invalid dependency type %d in dependency %p",
                      in_dep->dep.type, in_dep);
      release_dependency(in_dep);
      // NOTE: keep the dependency object in place, it will be cleaned up
      //       by the owning task
    } while (1);
  } else {
    //UNLOCK_TASK(elem);
    // if there are no active input dependencies we can immediately release the next
    // output dependency
    int32_t num_consumers = elem->num_consumers;
    if (num_consumers == 0) {
      DART_LOG_TRACE("Dependency %p has no consumers left, releasing next dep", elem);
      dephash_release_next_out_dependency_nolock(elem);
      // remove from hash table bucket
      dephash_remove_dep_from_bucket_nolock(elem, local_deps, slot);
      // recycle dephash element
      dephash_recycle_elem(elem);
    }
    UNLOCK_TASK(&local_deps[slot]);
  }

}

static void dephash_release_in_dependency(
  dart_dephash_elem_t *elem,
  dart_dephash_head_t *local_deps)
{
  // decrement the counter of the associated output dependency and release the
  // next output dependency if all input dependencies have completed
  dart_dephash_elem_t *out_dep = elem->dep_list;
  if (out_dep != NULL) {
    DART_ASSERT_MSG(out_dep->task.local == NULL,
                    "Output dependency %p is still active!", out_dep);
    int slot = hash_gptr(out_dep->dep.gptr);
    /**
     * Be safe here: lock the bucket to avoid race conditions.
     */
    LOCK_TASK(&local_deps[slot]);
    //int32_t num_consumers = DART_DEC_AND_FETCH32(&out_dep->num_consumers);
    int32_t num_consumers = --out_dep->num_consumers;
    DART_LOG_TRACE("Releasing input dependency %p (output dependency %p with nc %d)",
                  elem, out_dep, num_consumers);
    DART_ASSERT_MSG(num_consumers >= 0, "Found negative number of consumers for "
                    "dependency %p: %d", elem, num_consumers);
    dephash_recycle_elem(elem);
    if (num_consumers == 0){
      // release the next output dependency
      DART_LOG_TRACE("Dependency %p has no consumers left, releasing next dep",
                    out_dep);
      dephash_release_next_out_dependency_nolock(out_dep);

      // remove the output dependency from the bucket
      dephash_remove_dep_from_bucket_nolock(out_dep, local_deps, slot);

      // finally recycle the output dependency
      dephash_recycle_elem(out_dep);
    }
    UNLOCK_TASK(&local_deps[slot]);
  } else {
    DART_LOG_TRACE("Skipping input dependency %p as it has no output dependency!", elem);
    dephash_recycle_elem(elem);
  }
}

/**
 * Remove the dependencies of a task from the parent's dependency hash table.
 */
static void dephash_release_local_task(
  dart_task_t     * task)
{
  dart_dephash_elem_t *elem;
  DART_LOG_TRACE("Releasing local data dependencies of task %p", task);
  do {
    DART_STACK_POP_MEMB(task->deps_owned, elem, next_in_task);
    if (NULL == elem) break;

    DART_LOG_TRACE("Releasing dependency object %p (type %d, consumers %d)",
                   elem, elem->dep.type, elem->num_consumers);

    if (IS_OUT_DEP(elem->dep)) {
      // release all input dependencies
      dephash_release_out_dependency(elem, task->parent->local_deps);
    } else {
      dephash_release_in_dependency(elem, task->parent->local_deps);
    }
  } while (1);
  task->deps_owned = NULL;
}

dart_ret_t
dart_tasking_datadeps_handle_defered_local()
{
  dart_tasking_taskqueue_lock(&local_deferred_tasks);

  DART_LOG_TRACE("Releasing %zu deferred local tasks from queue %p",
                 local_deferred_tasks.num_elem, &local_deferred_tasks);

  dart_task_t *task;
  while (
    (task = dart_tasking_taskqueue_pop_unsafe(&local_deferred_tasks)) != NULL)
  {
    // enqueue the task if it has gained no additional remote dependencies
    // since it's deferrement
    // Note: if the task has gained dependencies we drop the reference
    //       here because it will be released through a dependency release later.
    LOCK_TASK(task);
    DART_ASSERT(task->state == DART_TASK_DEFERRED);
    bool runnable = dart_tasking_datadeps_is_runnable(task);
    task->state = DART_TASK_CREATED;
    UNLOCK_TASK(task);
    if (runnable){
      DART_LOG_TRACE("Releasing deferred task %p\n", task);
      dart__tasking__enqueue_runnable(task);
    }
  }

  dart_tasking_taskqueue_unlock(&local_deferred_tasks);
  // NOTE: no need to wake up threads here, it's done by the caller
  return DART_OK;
}

dart_ret_t
dart_tasking_datadeps_handle_defered_remote_indeps(
  dart_taskphase_t      matching_phase,
  dart_dephash_elem_t **release_candidates)
{
  dart_dephash_elem_t *rdep;
  DART_LOG_DEBUG("Handling previously unhandled remote input dependencies: %p",
                 unhandled_remote_indeps);

  // create tasks requested by remote units to handle copyin deps
  dart_tasking_copyin_create_delayed_tasks();

  dart_task_t *root_task = dart__tasking__root_task();
  dart_dephash_elem_t *next = unhandled_remote_indeps;
  unhandled_remote_indeps = NULL;
  while ((rdep = next) != NULL) {
    next = rdep->next;

    if (rdep->dep.phase > matching_phase) {
      // Skip any dependency we cannot handle yet
      DART_STACK_PUSH(unhandled_remote_indeps, rdep);
      continue;
    }

    if (rdep->dep.type == DART_DEP_DELAYED_IN) {
      // dispatch handling of delayed local dependencies
      dart_tasking_datadeps_match_delayed_local_indep(
        &rdep->dep, rdep->task.local);
      dephash_recycle_elem(rdep);
      continue;
    }

    /**
     * Iterate over all possible tasks and find the closest-matching
     * local task that satisfies the remote dependency.
     */
    DART_LOG_TRACE("Handling delayed remote dependency for task %p from unit %i phase %i",
                   rdep->task.local, rdep->origin.id, rdep->dep.phase);
    dart_dephash_head_t *local_deps = root_task->local_deps;
    if (local_deps != NULL) {
      int slot = hash_gptr(rdep->dep.gptr);
      LOCK_TASK(&local_deps[slot]);
      dart_dephash_elem_t *local;
      dart_dephash_elem_t *prev;
      for (local = local_deps[slot].head, prev = NULL;
           local != NULL;
           local = local->next) {
        if (DEP_ADDR_EQ(local->dep, rdep->dep)) {
          if (local->dep.phase == rdep->dep.phase) {
            DART_LOG_ERROR(
              "Found conflicting dependencies on local memory address %p in "
              "phase %d: local OUT task %p ('%s'), remote IN from unit %d",
              rdep->dep.gptr.addr_or_offs.addr, rdep->dep.phase, local->task.local,
              (local->task.local != NULL) ? local->task.local->descr : "(UNKNOWN)",
              rdep->origin.id);
            dart_abort(DART_EXIT_ABORT);
          }
          if (local->dep.phase < rdep->dep.phase) {
            // 'tis the one
            break;
          } else if (local->is_dummy){
            // a dummy output dependency, we need to adapt the phase to the earlier
            // phase of this input dependency
            DART_LOG_TRACE("Adjusting dummy dependency %p from phase %d to %d",
                           local, local->dep.phase, rdep->dep.phase - 1);
            local->dep.phase = rdep->dep.phase - 1;
            break;
          }
          prev = local;
        }
      }

      {
        dart_dephash_elem_t *check_elem = local;
        if (local == NULL) {
          check_elem = prev;
        }
        if (check_elem != NULL) {
          DART_ASSERT_MSG(check_elem->dep.phase != rdep->dep.phase,
                          "Found conflicting dependencies on local memory address %p in phase %d: local task %p ('%s'), remote ");
        }
      }

      bool runnable = false;
      if (local == NULL) {
        // create a dummy output dependency and register this dependency with it
        dart_dephash_elem_t *out_dep = dephash_allocate_elem(&rdep->dep,
                                                             TASKREF(NULL),
                                                             rdep->origin);
        out_dep->is_dummy = true;
        // output depdendencies live in the previous phase
        --(out_dep->dep.phase);
        dephash_list_insert_elem_after_nolock(&local_deps[slot], out_dep, prev);
        local = out_dep;
        DART_LOG_TRACE("Inserting dummy output dep %p for delayed input dep "
                       "from task %p, unit %d, phase %d, slot %d",
                       out_dep, rdep->task.local, rdep->origin.id,
                       rdep->dep.phase, slot);
        // make the successor output dependency aware of the dummy
        if (prev != NULL && prev->task.local != NULL) {
          DART_FETCH_AND_INC32(&prev->task.local->unresolved_deps);
        }
      }

      if (local->task.local == NULL) {
        runnable = true;
      }

      register_at_out_dep_nolock(local, rdep);
      UNLOCK_TASK(&local_deps[slot]);

      if (runnable) {
        DART_LOG_TRACE("Delayed dep %p of task %p from unit %d is immediately runnable",
                       rdep, rdep->task.local, rdep->origin.id);
        // save the release for after matching the output dependencies
        DART_STACK_PUSH_MEMB(*release_candidates, rdep, next_in_task);
      }
    }
  }

  return DART_OK;
}

dart_ret_t
dart_tasking_datadeps_handle_defered_remote_outdeps(
  dart_taskphase_t matching_phase)
{
  DART_LOG_DEBUG("Handling previously unhandled remote output dependencies: %p",
                 unhandled_remote_outdeps.head);

  dart_task_t *root_task = dart__tasking__root_task();
  dephash_require_alloc(root_task);
  dart_dephash_elem_t *next = unhandled_remote_outdeps.head;
  unhandled_remote_outdeps.head        = NULL;
  unhandled_remote_outdeps.num_outdeps = 0;
  // iterate over all delayed remote output deps
  dart_dephash_elem_t *rdep;
  while ((rdep = next) != NULL) {
    next = rdep->next;
    bool runnable = false;

    if (rdep->dep.phase > matching_phase) {
      // Skip any dependency we cannot handle yet
      dephash_list_insert_elem_sorted_phase_desc_nolock(
        &unhandled_remote_outdeps, rdep);
      continue;
    }

    DART_LOG_TRACE("Handling remote dependency %p", rdep);

    dart_taskphase_t phase = rdep->dep.phase;

    int slot = hash_gptr(rdep->dep.gptr);
    dart_dephash_head_t *local_deps = root_task->local_deps;
    LOCK_TASK(&local_deps[slot]);
    dart_dephash_elem_t *local;
    // prev is the previous dependency on the same memory location
    dart_dephash_elem_t *prev = NULL;
    for (local = local_deps[slot].head;
         local != NULL;
         local = local->next) {
      if (DEP_ADDR_EQ(local->dep, rdep->dep)) {
        if (local->dep.phase <= phase) {
          // 'tis the one
          break;
        }
        prev = local;
      }
    }

    // make sure there are no colliding dependencies
    if (local != NULL && local->dep.phase == phase && local->task.local != NULL) {
      DART_LOG_ERROR("Found coliding remote output dependencies in phase %d! "
                     "(local task %p, local dep obj %p)",
                     local->dep.phase, local->task.local, local);
      dart_abort(DART_EXIT_ABORT);
    }

    bool needs_insert = true;

    dart_dephash_elem_t *dummy_task = NULL;
    if (local != NULL && local->is_dummy) {
      dummy_task = local;
      // if there are no previous output dependencies we can release directly
      if (prev == NULL) {
        runnable = true;
      }
    } else if (prev != NULL && prev->is_dummy) {
      dummy_task = prev;
      // if there are no previous output dependencies we can release directly
      if (local == NULL) {
        runnable = true;
      }
    }
    //if (local == NULL) {
    //  if (prev != NULL && prev->is_dummy) {
    if (dummy_task != NULL) {
        /**
        * We found a dummy dependency that was created in a later phase.
        * Thus, we can capture it. All input dependencies in this dummy have a
        * phase later than our dependency and earlier than the previous
        * regular output dependency (as later dependencies have been stolen
        * before thanks to the ordered insertion).
        */
        DART_LOG_TRACE("Capturing dummy dependency %p for remote dependency %p",
                       dummy_task, rdep);
        dummy_task->task   = rdep->task;
        dummy_task->origin = rdep->origin;
        // release the remote depedendency object, we're working on the existing on
        dephash_recycle_elem(rdep);
        rdep = dummy_task; // defensive!
        needs_insert = false;
        // there is nothing else to do with `local`
        local = NULL;
        // mark all tasks dependent on dummy_task to actually have a dependency
        dart_dephash_elem_t *dep;
        for (dep = dummy_task->dep_list;
             dep != NULL;
             dep = dep->next) {
          DART_INC_AND_FETCH32(&dep->task.local->unresolved_deps);
        }
      }
    //}


    if (needs_insert) {
      DART_LOG_TRACE("Inserting remote out dependency %p after %p",
                     rdep, prev);
      // insert the remote dependency into the bucket
      dephash_list_insert_elem_after_nolock(&local_deps[slot], rdep, prev);
    }

    /**
     * If there is an earlier dependency: check whether we can steal input
     * dependencies from it for which our phase is a better match.
     */
    if (local != NULL) {
      // check if this dependency is a better match than the next one
      //LOCK_TASK(local);
      if (local->task.local == NULL) {
        DART_LOG_WARN("Task in dependency %p already completed, cannot steal!",
                      local);
      } else {
        dart_dephash_elem_t *in_dep = local->dep_list;
        dart_dephash_elem_t *prev = NULL;
        dart_dephash_elem_t *next = in_dep;
        while (NULL != (in_dep = next))
        {
          next = in_dep->next;
          if (in_dep->dep.phase > rdep->dep.phase) {
            // steal this dependency
            DART_LOG_TRACE("Stealing in dep %p (ph %d) from out dep %p (ph %d) "
                           "to out dep %p (ph %d)",
                           in_dep, in_dep->dep.phase,
                           local, local->dep.phase,
                           rdep, rdep->dep.phase);
            if (prev == NULL) {
              local->dep_list = next;
            } else {
              prev->next = next;
            }
            deregister_in_dep_nolock(in_dep);
            register_at_out_dep_nolock(rdep, in_dep);
          }

          prev = in_dep;
        }
      }
      //UNLOCK_TASK(local);
    }

    /**
     * Finally, release the task if it is runnable.
     */
    if (runnable) {
      release_dependency(rdep);
    }

    UNLOCK_TASK(&local_deps[slot]);
  }

  return DART_OK;
}

static void
dart_tasking_datadeps_release_runnable_remote_indeps(
  dart_dephash_elem_t *release_candidates)
{
  if (release_candidates != NULL) {
    dart_dephash_elem_t *elem;
    do {
      DART_STACK_POP_MEMB(release_candidates, elem, next_in_task);
      if (elem == NULL) break;
      //LOCK_TASK(elem);
      /**
       * If the indep's output dependency has no task it means that the task
       * is runnable.
       */
      if (elem->dep_list->task.local == NULL) {
        // safe to send a remote release now
        release_dependency(elem);
      }
      //UNLOCK_TASK(elem);
    } while (1);
  }
}

dart_ret_t
dart_tasking_datadeps_handle_defered_remote(dart_taskphase_t matching_phase)
{
  /**
  * List of dephash elements representing remote tasks that were deemed to be
  * runnable during matching. They need to be checked again after matching
  * completed.
  * NOTE: the list is formed using the next_in_task member and there is no multi-
  *       threaded access.
  */
  dart_dephash_elem_t *release_candidates = NULL;

  dart__base__mutex_lock(&unhandled_remote_mutex);
  // insert the deferred remote input dependencies
  dart_tasking_datadeps_handle_defered_remote_indeps(
    matching_phase, &release_candidates);

  // match the remote output dependencies
  dart_tasking_datadeps_handle_defered_remote_outdeps(matching_phase);
  dart__base__mutex_unlock(&unhandled_remote_mutex);

  // check whether we can release any task with remote input deps
  dart_tasking_datadeps_release_runnable_remote_indeps(release_candidates);

  return DART_OK;
}

static dart_ret_t
dart_tasking_datadeps_handle_local_direct(
  const dart_task_dep_t * dep,
        dart_task_t     * task)
{
  dart_task_t *deptask = dep->task;
  if (deptask != DART_TASK_NULL) {
    LOCK_TASK(deptask);
    if (IS_ACTIVE_TASK(deptask)) {
      dart_tasking_tasklist_prepend(&(deptask->successor), task);
      int32_t unresolved_deps = DART_INC_AND_FETCH32(
                                    &task->unresolved_deps);
      DART_LOG_TRACE("Making task %p a direct local successor of task %p "
                     "(successor: %p, state: %i | num_deps: %i)",
                     task, deptask,
                     deptask->successor, deptask->state, unresolved_deps);
      instrument_task_dependency(deptask, task, DART_GPTR_NULL);
    }
    UNLOCK_TASK(deptask);
  }
  return DART_OK;
}


static dart_ret_t
dart_tasking_datadeps_handle_copyin(
  const dart_task_dep_t * dep,
        dart_task_t     * task)
{
  int slot;
  dart_gptr_t dest_gptr;

  dest_gptr.addr_or_offs.addr = dep->copyin.dest;
  dest_gptr.flags             = 0;
  dest_gptr.segid             = DART_TASKING_DATADEPS_LOCAL_SEGID;
  dest_gptr.teamid            = 0;
  dest_gptr.unitid            = myguid.id;
  slot = hash_gptr(dest_gptr);
  int iter = 0;
  DART_LOG_TRACE("Handling copyin dep (unit %d, phase %d)",
                 dep->copyin.gptr.unitid, dep->phase);

  do {
    dart_task_t *parent = task->parent;
    // check whether this is the first task with copyin
    if (parent->local_deps != NULL) {
      LOCK_TASK(&parent->local_deps[slot]);
      dart_dephash_elem_t *elem = NULL;
      for (elem = parent->local_deps[slot].head;
           elem != NULL; elem = elem->next) {
        if (elem->dep.gptr.addr_or_offs.addr == dep->copyin.dest) {
          if (elem->dep.phase < dep->phase) {
            // phases are stored in decending order so we can stop here
            break;
          }
          // So far we can only re-use prefetching in the same phase
          // TODO: can we figure out whether we can go back further?
          //       Might need help from the remote side.
          if (IS_OUT_DEP(elem->dep) && dep->phase == elem->dep.phase) {
            UNLOCK_TASK(&parent->local_deps[slot]);
            // we're not the first --> add a dependency to the task that does the copy
            DART_INC_AND_FETCH32(&task->unresolved_deps);

            // register the depdendency with the output dependency
            dart_task_dep_t in_dep;
            in_dep.type  = DART_DEP_IN;
            in_dep.gptr  = dest_gptr;
            in_dep.phase = dep->phase;
            dart_dephash_elem_t *new_elem;
            new_elem = dephash_allocate_elem(&in_dep, TASKREF(task), myguid);
            DART_STACK_PUSH_MEMB(task->deps_owned, new_elem, next_in_task);
            register_at_out_dep_nolock(elem, new_elem);

            dart_task_t *elem_task = elem->task.local;
            DART_LOG_TRACE("Copyin: task %p waits for copyin task %p", task, elem_task);

            // we're done
            return DART_OK;
          }
        }
      }
      UNLOCK_TASK(&parent->local_deps[slot]);
    }

    // this shouldn't happen
    DART_ASSERT_MSG(iter == 0, "FAILED to create copyin task!");

    // we haven't found a task that does the prefetching in this phase
    // so create a new one
    DART_LOG_TRACE("Creating copyin task in phase %d (dest %p)",
                   dep->phase, dep->copyin.dest);
    dart_tasking_copyin_create_task(dep, dest_gptr, TASKREF(task));
  } while (0 == iter++);

  return DART_OK;
}


/**
 * Match a local data dependency.
 * This ignores phases and matches a dependency to the last previous depdendency
 * encountered.
 */
static dart_ret_t
dart_tasking_datadeps_match_local_dependency(
  const dart_task_dep_t * dep,
        dart_task_t     * task)
{
  dart_task_t *parent = task->parent;

  // TODO: we cannot short-cut here because we need to store all local input
  //       dependencies to match against remote input dependencies
  //if (parent->local_deps == NULL) return DART_OK;
  dephash_require_alloc(parent);

  int slot;
  slot = hash_gptr(dep->gptr);

  // lock task to make sure the hash table is consistent
  // TODO: more fine-grained locking on bucket level
  LOCK_TASK(&parent->local_deps[slot]);

  DART_LOG_TRACE("Matching local dependency for task %p (off: %p, type:%d)",
                 task, dep->gptr.addr_or_offs.addr, dep->type);

  /*
  * iterate over all dependent tasks until we find the first task with
  * OUT|INOUT dependency on the same pointer
  */
  dart_dephash_elem_t *prev = NULL;
  dart_dephash_elem_t *elem = NULL;
  for (elem = parent->local_deps[slot].head;
      elem != NULL; prev = elem, elem = elem->next)
  {
    DART_ASSERT_MSG(elem->prev == prev,
                    "Corrupt double linked list: elem %p, elem->prev %p, prev %p",
                    elem, elem->prev, prev);

    if (DEP_ADDR_EQ(elem->dep, *dep)) {
      break;
    }
  }

  if (dep->type == DART_DEP_IN) {
    dart_dephash_elem_t *new_elem = dephash_allocate_elem(dep, TASKREF(task), myguid);
    DART_STACK_PUSH_MEMB(task->deps_owned, new_elem, next_in_task);
    if (elem == NULL) {
      /**
       * Couldn't find matching output dependency.
       * Insert a dummy output dependency and register the input dependency
       * with it.
       * The dummy dependency won't have a task assigned to it and has the phase
       * of the input dependency.
       * A remote output dependency serving this input dependency may then
       * capture this dummy dependency and adjust the phase.
       */

      // create a dummy output dependency and register with it
      dart_task_dep_t out_dep = *dep;
      out_dep.type = DART_DEP_OUT;
      // put the dummy output dependency in a previous phase
      --out_dep.phase;
      dart_dephash_elem_t *dummy = dephash_allocate_elem(&out_dep, TASKREF(NULL), myguid);
      dummy->is_dummy = true;
      register_at_out_dep_nolock(dummy, new_elem);
      // put the dummy dependency in the hash table
      dephash_list_insert_elem_after_nolock(&parent->local_deps[slot], dummy, NULL);
      // NOTE: the dummy dependency is not registered with the task because
      // the task does not own it. It is already released and will be free'd
      // once the input dependency is released.
      DART_LOG_TRACE("Inserting dummy dependency %p for input dependency %p "
                      "of task %p in phase %d",
                     dummy, new_elem, task, out_dep.phase);
    } else {
      //LOCK_TASK(elem);
      if (elem->task.local != NULL) {
        int32_t unresolved_deps = DART_INC_AND_FETCH32(
                                      &task->unresolved_deps);
        DART_LOG_TRACE("Making task %p a local successor of task %p "
                      "(num_deps: %i, outdep: %p)",
                      task, elem->task.local, unresolved_deps, elem);
        register_at_out_dep_nolock(elem, new_elem);
      } else {
        //DART_INC_AND_FETCH32(&elem->num_consumers);
        elem->num_consumers++;
        // register the output dependency with the input dependency for later release
        new_elem->dep_list = elem;
        DART_LOG_TRACE("Task of out dep %p already running, not waiting to finish",
                       elem);
      }
      //UNLOCK_TASK(elem);
    }
  } else {
    if (elem != NULL) {
      int32_t unresolved_deps = DART_INC_AND_FETCH32(
                                    &task->unresolved_deps);

      // check if we already have an input dependency on that task and remove it
      dart_dephash_elem_t *prev = NULL;
      dart_dephash_elem_t *iter;
      for (iter = elem->dep_list; iter != NULL; iter = iter->next) {
        if (iter->task.local == task) {
          DART_LOG_TRACE("Removing input dependency %p of task %p from output "
                         "dependency %p of task %p",
                         iter, task, elem, elem->task.local);
          if (prev == NULL) {
            // first element, replace head
            elem->dep_list = iter->next;
          } else {
            prev->next = iter->next;
          }
          elem->num_consumers--;
          task->unresolved_deps--;
        }
        prev = iter;
      }
      if (iter != NULL) {
        dephash_recycle_elem(iter);
      }

      DART_LOG_TRACE("Making task %p a local successor of task %p in out dep %p"
                    "(num_deps: %i)",
                    task, elem->task.local, elem, unresolved_deps);
    } else {
      DART_LOG_TRACE("No previous out dependency for task %p", task);
    }
    // insert output dependency into the hash table
    dephash_add_local_nolock(dep, task, slot);
  }

  UNLOCK_TASK(&parent->local_deps[slot]);

  return DART_OK;
}


/**
 * Match a delayed local input dependency.
 * This is similar to \ref dart_tasking_datadeps_match_local_dependency but
 * handles the local dependency honoring the phase, i.e., later depdendencies
 * are skipped. This also potententially adds dependencies to the graph.
 */
static dart_ret_t
dart_tasking_datadeps_match_delayed_local_indep(
  const dart_task_dep_t * dep,
        dart_task_t     * task)
{
  dart_task_t *parent = task->parent;

  // shortcut if no dependencies to match, yet
  if (parent->local_deps == NULL) return DART_OK;

  int slot;
  slot = hash_gptr(dep->gptr);

  DART_LOG_DEBUG("Handling delayed input dependency in phase %d", dep->phase);

  LOCK_TASK(&parent->local_deps[slot]);
  for (dart_dephash_elem_t *elem = parent->local_deps[slot].head;
       elem != NULL; elem = elem->next)
  {
    // skip output dependencies that were created in a later phase
    if (elem->dep.phase >= dep->phase) {
      continue;
    }

    if (DEP_ADDR_EQ(elem->dep, *dep)) {
      dart_task_t *elem_task = elem->task.local;
      DART_ASSERT_MSG(elem_task != task,
                      "Cannot insert existing task with delayed dependency!");

      DART_ASSERT(IS_ACTIVE_TASK(elem_task));
      dart_dephash_elem_t *new_elem;
      new_elem = dephash_allocate_elem(dep, TASKREF(task), myguid);

      //LOCK_TASK(elem);
      if (elem->task.local != NULL) {
        int32_t unresolved_deps = DART_INC_AND_FETCH32(
                                      &task->unresolved_deps);
        DART_LOG_TRACE("Making task %p a local successor of task %p using delayed dependency "
                      "(state: %i | num_deps: %i)",
                      task, elem_task,
                      elem_task->state, unresolved_deps);
      }
      register_at_out_dep_nolock(elem, new_elem);
      //UNLOCK_TASK(elem);
      // we're done here
      break;
    }
  }
  UNLOCK_TASK(&parent->local_deps[slot]);

  if (!IS_OUT_DEP(*dep)) {
    DART_LOG_TRACE("No matching output dependency found for local input "
        "dependency %p of task %p in phase %i",
        DEP_ADDR(*dep), task, task->phase);
  }

  return DART_OK;
}


/**
 * Find all tasks this task depends on and add the task to the dependency hash
 * table. All earlier tasks are considered up to the first task with OUT|INOUT
 * dependency.
 */
dart_ret_t dart_tasking_datadeps_handle_task(
    dart_task_t           *task,
    const dart_task_dep_t *deps,
    size_t                 ndeps)
{
  DART_LOG_DEBUG("Datadeps: task %p has %zu data dependencies in phase %i",
                 task, ndeps, task->phase);

  // order dependencies: copyin dependencies need to come first
  // to avoid a circular dependency with the copyin-task
  for (size_t i = 0; i < ndeps; i++) {
    if (deps[i].type == DART_DEP_COPYIN) {
      dart_task_dep_t dep = deps[i];
      // adjust the phase of the dependency if required
      if (dep.phase == DART_PHASE_TASK) {
        dep.phase = task->phase;
      }
      dart_tasking_datadeps_handle_copyin(&dep, task);
    }
  }

  for (size_t i = 0; i < ndeps; i++) {
    dart_task_dep_t dep = deps[i];
    if (dep.type == DART_DEP_IGNORE) {
      // ignored
      continue;
    }

    // check for duplicate dependencies
    if (dep.type == DART_DEP_IN) {
      bool needs_skipping = false;
      for (int j = 0; j < ndeps; ++j) {
        if (deps[j].type == DART_DEP_OUT && DART_GPTR_EQUAL(deps[j].gptr, dep.gptr)){
          // skip this dependency because there is an OUT dependency handling it
          // we need to do this to avoid inserting a dummy for an input dependency
          // first and then inserting the output dependency
          // TODO: this is a O(n^2) check. Can we do better?
          // TODO: see if we can detect that efficiently while inserting the OUT
          //       dependency into the tree.
          DART_LOG_TRACE("Skipping dependency %d due to conflicting "
                         "input-output dependency on same task %p", i, task);
          needs_skipping = true;
          break;
        }
      }
      // skip processing of this dependency
      if (needs_skipping) continue;
    }

    // adjust the phase of the dependency if required
    if (dep.phase == DART_PHASE_TASK) {
      dep.phase = task->phase;
    }

    // get the global unit ID in the dependency
    dart_global_unit_t guid;
    if (dep.gptr.teamid != DART_TEAM_ALL) {
      dart_team_unit_l2g(dep.gptr.teamid,
                         DART_TEAM_UNIT_ID(dep.gptr.unitid),
                         &guid);
    } else {
      guid.id = dep.gptr.unitid;
    }

    if (dep.type != DART_DEP_DIRECT) {
      DART_LOG_TRACE("Datadeps: task %p dependency %zu: type:%i unit:%i "
                     "seg:%i addr:%p phase:%i",
                     task, i, dep.type, guid.id, dep.gptr.segid,
                     DEP_ADDR(dep), dep.phase);
    }

    if (dep.type == DART_DEP_DIRECT) {
      dart_tasking_datadeps_handle_local_direct(&dep, task);
    } else if (dep.type == DART_DEP_COPYIN){
      // set the numaptr
      if (task->numaptr == NULL) task->numaptr = dep.copyin.dest;
      // nothing to be done, handled above
      continue;
    } else if (guid.id != myguid.id) {
        if (task->parent->state == DART_TASK_ROOT) {
          dart_tasking_remote_datadep(&dep, guid, task);
          int32_t unresolved_deps = DART_INC_AND_FETCH32(&task->unresolved_remote_deps);
          DART_LOG_TRACE(
            "Sent remote dependency request for task %p "
            "(unit=%i, team=%i, segid=%i, offset=%p, num_deps=%i)",
            task, guid.id, dep.gptr.teamid, dep.gptr.segid,
            dep.gptr.addr_or_offs.addr, unresolved_deps);
        } else {
          DART_LOG_WARN("Ignoring remote dependency in nested task!");
        }
    } else {
      // translate the pointer to a local pointer
      dep.gptr = dart_tasking_datadeps_localize_gptr(dep.gptr);
      if (dep.type == DART_DEP_DELAYED_IN) {
        /**
         * delayed input dependencies should be treated as remote dependencies.
         * the creation of the task using this dependency has been delayed until
         * the matching step so we can process it here.
         */
        dart_tasking_datadeps_match_delayed_local_indep(&dep, task);

      } else {
        // match both input and output dependencies
        dart_tasking_datadeps_match_local_dependency(&dep, task);

        // set the numaptr
        if (task->numaptr == NULL) task->numaptr = dep.gptr.addr_or_offs.addr;
      }
    }
  }

  return DART_OK;
}

/**
 * Handle an incoming dependency request by enqueuing it for later handling.
 */
dart_ret_t dart_tasking_datadeps_handle_remote_task(
    const dart_task_dep_t  *rdep,
    const taskref           remote_task,
    dart_global_unit_t      origin)
{
  DART_LOG_TRACE("Enqueuing remote task %p from unit %i for later resolution",
                remote_task.remote, origin.id);
  // cache this request and resolve it later
  dart_dephash_elem_t *rs = dephash_allocate_elem(rdep, remote_task, origin);

  DART_ASSERT_MSG(rdep->phase == DART_PHASE_FIRST ||
                  !dart__tasking__phase_is_runnable(rdep->phase),
                  "Phase %d of received dependency %p (task %p, unit %d) is "
                  "already runnable (%d)!",
                  rdep->phase, rs, remote_task.local, origin.id,
                  dart__tasking__phase_runnable());

  // TODO: are these locks really necessary?
  dart__base__mutex_lock(&unhandled_remote_mutex);
  if (rdep->type == DART_DEP_IN) {
    DART_STACK_PUSH(unhandled_remote_indeps, rs);
  } else {
    /**
     * store the dependency in phase-descending order so we can later insert
     * them into the hash table, starting with the highest phase, to match
     * input dependencies that had not been assigned an output dependency before.
     */
    dephash_list_insert_elem_sorted_phase_desc_nolock(&unhandled_remote_outdeps, rs);
  }
  dart__base__mutex_unlock(&unhandled_remote_mutex);
  return DART_OK;
}

/**
 * Release remote and local dependencies of a local task
 */
dart_ret_t dart_tasking_datadeps_release_local_task(
    dart_task_t   *task,
    dart_thread_t *thread)
{
  DART_LOG_TRACE("Releasing local dependencies of task %p", task);

  // start with removing this task from the hash maps
  dephash_release_local_task(task);
  // release the remote dependencies
  release_remote_dependencies(task);

  DART_LOG_TRACE("Releasing local direct dependencies of task %p", task);
  // release local successors
  dart_task_t *succ;
  while ((succ = dart_tasking_tasklist_pop(&task->successor)) != NULL) {
    DART_LOG_TRACE("  Releasing task %p", succ);

    LOCK_TASK(succ);
    bool runnable = release_local_dep_counter(succ);
    dart_task_state_t state = succ->state;
    UNLOCK_TASK(succ);
    DART_LOG_TRACE("  Task %p: state %d runnable %i", succ, state, runnable);

    if (runnable) {
      if (state == DART_TASK_CREATED) {
        dart__tasking__enqueue_runnable(succ);
      } else {
        DART_ASSERT_MSG(state == DART_TASK_DEFERRED ||
                        state == DART_TASK_NASCENT,
                        "Unexpected task state %d in dependency release!", state);
      }
    }
  }

  return DART_OK;
}

/**
 * Handle an incoming release of a remote dependency.
 * The release might be deferred until after the matching of dependencies
 * has completed.
 */
dart_ret_t dart_tasking_datadeps_release_remote_task(
  dart_task_t        *local_task,
  uintptr_t           elem,
  dart_global_unit_t  unit)
{

  if (elem != 0) {
    // store the remote dephash element reference in the dependencie's gptr
    dart_task_dep_t dep;
    dep.gptr.unitid = unit.id;
    dep.gptr.addr_or_offs.offset = elem;
    dart_dephash_elem_t *new_elem = dephash_allocate_elem(&dep, TASKREF(NULL), unit);
    DART_STACK_PUSH(local_task->remote_successor, new_elem);
    DART_LOG_TRACE("Storing dependency %p from unit %d in dep object %p",
                   (void*)elem, unit.id, new_elem);
  }

  // release the task if it is runnable
  LOCK_TASK(local_task);
  bool runnable = release_remote_dep_counter(local_task);
  dart_task_state_t state = local_task->state;
  UNLOCK_TASK(local_task);

  if (runnable) {
    // enqueue as runnable
    if (state == DART_TASK_CREATED) {
      dart__tasking__enqueue_runnable(local_task);
    } else {
      // if the task is nascent or deferred someone else will
      // take care of enqueueing it
      DART_ASSERT_MSG(state == DART_TASK_NASCENT ||
                      state == DART_TASK_DEFERRED,
                      "Unexpected task state: %d", state);
    }
  }
  return DART_OK;
}

dart_ret_t dart_tasking_datadeps_release_remote_dep(
  dart_dephash_elem_t *elem)
{
  dart_task_t *parent = dart__tasking__root_task();
  DART_ASSERT(elem != NULL);

  if (elem->dep.type == DART_DEP_IN) {
    dephash_release_in_dependency(elem, parent->local_deps);
  } else {
    dephash_release_out_dependency(elem, parent->local_deps);
  }

  return DART_OK;
}

/**
 * Release the remote dependencies of \c task.
 */
static dart_ret_t release_remote_dependencies(dart_task_t *task)
{
  DART_LOG_TRACE("Releasing remote dependencies for task %p (rs:%p)",
                 task, task->remote_successor);
  do {
    dart_dephash_elem_t *rs;
    DART_STACK_POP(task->remote_successor, rs);
    if (rs == NULL) {
      break;
    }

    // send the release
    dart_global_unit_t guid;
    guid.id = rs->dep.gptr.unitid;
    uintptr_t depref = rs->dep.gptr.addr_or_offs.offset;
    dart_tasking_remote_release_dep(guid, rs->task, depref);
    dephash_recycle_elem(rs);
  } while (1);
  task->remote_successor = NULL;
  return DART_OK;
}
