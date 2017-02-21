
#include <dash/dart/base/assert.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/base/mutex.h>
#include <dash/dart/base/atomic.h>
#include <dash/dart/if/dart_tasking.h>
#include <dash/dart/tasking/dart_tasking_priv.h>
#include <dash/dart/tasking/dart_tasking_tasklist.h>
#include <dash/dart/tasking/dart_tasking_datadeps.h>
#include <dash/dart/tasking/dart_tasking_remote.h>
#include <dash/dart/tasking/dart_tasking_taskqueue.h>

#include <stdbool.h>

#define DART_DEPHASH_SIZE 1024

/**
 * Management of task data dependencies using a hash map that maps pointers to tasks.
 * The hash map implementation is taken from dart_segment.
 * The hash uses the absolute local address stored in the gptr since that is used
 * throughout the task handling code.
 */


#define IS_OUT_DEP(taskdep) (((taskdep).type == DART_DEP_OUT || (taskdep).type == DART_DEP_INOUT))

typedef struct dart_dephash_elem {
  struct dart_dephash_elem *next;
  union taskref             task;
  dart_task_dep_t           taskdep;
  uint64_t                  phase;
} dart_dephash_elem_t;

static dart_dephash_elem_t *local_deps[DART_DEPHASH_SIZE];
static dart_dephash_elem_t *freelist_head = NULL;
static dart_mutex_t         local_deps_mutex;

static dart_dephash_elem_t *unhandled_remote_deps = NULL;
static dart_mutex_t         unhandled_remote_mutex;

static dart_mutex_t         deferred_remote_mutex;
static dart_dephash_elem_t *deferred_remote_releases = NULL;


static dart_ret_t
release_remote_dependencies(dart_task_t *task);

static void
dephash_recycle_elem(dart_dephash_elem_t *elem);

static inline int hash_gptr(dart_gptr_t gptr)
{
  /**
   * Use the upper 61 bit of the pointer since we assume that pointers
   * are 8-byte aligned.
   */
  uint64_t offset = gptr.addr_or_offs.offset;
  offset >>= 3;
  // use triplet (7, 11, 10), consider adding (21,17,48)
  // proposed by Marsaglia
  return ((offset ^ (offset >> 7) ^ (offset >> 11) ^ (offset >> 17))
              % DART_DEPHASH_SIZE);
  //return ((gptr.addr_or_offs.offset >> 3) % DART_DEPHASH_SIZE);
}

/**
 * Initialize the data dependency management system.
 */
dart_ret_t dart_tasking_datadeps_init()
{
  memset(local_deps, 0, sizeof(dart_dephash_elem_t*) * DART_DEPHASH_SIZE);

  dart_mutex_init(&local_deps_mutex);
  dart_mutex_init(&unhandled_remote_mutex);
  dart_mutex_init(&deferred_remote_mutex);

  return dart_tasking_remote_init();
}

dart_ret_t dart_tasking_datadeps_reset()
{
  for (int i = 0; i < DART_DEPHASH_SIZE; ++i) {
    dart_dephash_elem_t *elem = local_deps[i];
    while (elem != NULL) {
      dart_dephash_elem_t *tmp = elem->next;
      dephash_recycle_elem(elem);
      elem = tmp;
    }
  }
  memset(local_deps, 0, sizeof(dart_dephash_elem_t*) * DART_DEPHASH_SIZE);
  return DART_OK;
}

dart_ret_t dart_tasking_datadeps_fini()
{
  dart_mutex_destroy(&local_deps_mutex);
  dart_mutex_destroy(&unhandled_remote_mutex);
  dart_mutex_destroy(&deferred_remote_mutex);
  dart_tasking_datadeps_reset();
  dart_dephash_elem_t *elem = freelist_head;
  while (elem != NULL) {
    dart_dephash_elem_t *tmp = elem->next;
    free(elem);
    elem = tmp;
  }
  freelist_head = NULL;
  return dart_tasking_remote_fini();
}

/**
 * Check for new remote task dependency requests coming in
 */
dart_ret_t dart_tasking_datadeps_progress()
{
  return dart_tasking_remote_progress();
}

/**
 * Allocate a new element for the dependency hash, possibly from a free-list
 */
static dart_dephash_elem_t *
dephash_allocate_elem(const dart_task_dep_t *dep, taskref task)
{
  // take an element from the free list if possible
  dart_dephash_elem_t *elem = NULL;
  if (freelist_head != NULL) {
    dart_mutex_lock(&local_deps_mutex);
    if (freelist_head != NULL) {
      DART_STACK_POP(freelist_head, elem);
    }
    dart_mutex_unlock(&local_deps_mutex);
  }

  if (elem == NULL){
    elem = calloc(1, sizeof(dart_dephash_elem_t));
  }

  DART_ASSERT(task.local != NULL);
  DART_ASSERT(elem->task.local == NULL);
  elem->task = task;
  elem->taskdep = *dep;

  return elem;
}


/**
 * Deallocate an element
 */
static void dephash_recycle_elem(dart_dephash_elem_t *elem)
{
  if (elem != NULL) {
    memset(elem, 0, sizeof(*elem));
    dart_mutex_lock(&local_deps_mutex);
    DART_STACK_PUSH(freelist_head, elem);
    dart_mutex_unlock(&local_deps_mutex);
  }
}

/**
 * Add a task with dependency to the local dependency hash table.
 */
static dart_ret_t dephash_add_local(const dart_task_dep_t *dep, taskref task)
{
  dart_dephash_elem_t *elem = dephash_allocate_elem(dep, task);
  // we can take the task's phase only for local tasks
  // so we have to do it here instead of dephash_allocate_elem
  elem->phase = task.local->phase;
  // put the new entry at the beginning of the list
  int slot = hash_gptr(dep->gptr);
  dart_mutex_lock(&local_deps_mutex);
  DART_STACK_PUSH(local_deps[slot], elem);
  dart_mutex_unlock(&local_deps_mutex);

  return DART_OK;
}

#if 0
/**
 * Iterate over the list of unhandled remote dependencies to find remote tasks that
 * depend on this task and do one of the following:
 * 1) If  the remote dependency and \c task are in the same phase, the
 *    remote dependency will be handled by \c task.
 * 2) If the remote dependency stems from a phase earlier than the phase of
 *    \c task, \c task has a direct task dependency to the task of the remote
 *    dependency (to prevent overwriting of IN data before use).
 */
static dart_ret_t
check_unresolved_remote_deps(
  dart_task_t           *task,
  const dart_task_dep_t *dep)
{
  DART_LOG_DEBUG("Checking whether local task %p can handle previously unhandled"
                 "dependencies", task);
  if (IS_OUT_DEP(*dep) && unhandled_remote_deps != NULL) {
    dart_mutex_lock(&local_deps_mutex);
    dart_dephash_elem_t *elem = unhandled_remote_deps;
    dart_dephash_elem_t *prev = NULL;
    while (elem != NULL) {
      dart_dephash_elem_t *next = elem->next;
      if (elem->taskdep.gptr.addr_or_offs.addr == dep->gptr.addr_or_offs.addr)
      {
        if (elem->phase >= task->phase) {
          // move the remote request from unhandled into the task
          if (!prev) {
            unhandled_remote_deps = elem->next;
          } else {
            prev->next = elem->next;
          }
          DART_LOG_TRACE("Previously unhandled remote dependency "
                         "{address:%p, origin=%i, ph:%i} to be handled by "
                         "task %p",
                         dep->gptr.addr_or_offs.addr,
                         elem->taskdep.gptr.unitid, elem->phase, task);
          dart_mutex_lock(&(task->mutex));
          DART_STACK_PUSH(task->remote_successor, elem);
          dart_mutex_unlock(&(task->mutex));
        } else if (elem->phase < task->phase) {
          // send direct task dependency
          dart_tasking_remote_direct_taskdep(
              DART_GLOBAL_UNIT_ID(elem->taskdep.gptr.unitid),
              task,
              elem->task);
          DART_INC32_AND_FETCH(&task->unresolved_deps);
          // we leave the task in the unhandled task list for
          // handling by another task
        }
      } else {
        prev = elem;
      }
      elem = next;
    }
    dart_mutex_unlock(&local_deps_mutex);

  }
  return DART_OK;
}

#endif //0

#if 0
dart_ret_t
dart_tasking_datadeps_release_unhandled_remote()
{
  dart_dephash_elem_t *elem = unhandled_remote_deps;
  while (elem != NULL) {
    dart_dephash_elem_t *next = elem->next;
    DART_LOG_DEBUG("Releasing remote task %p from unit %i, "
                   "which was not handled in phase %i",
                   elem->task.remote, elem->taskdep.gptr.unitid,
                   elem->phase);
    dart_tasking_remote_release(
        DART_GLOBAL_UNIT_ID(elem->taskdep.gptr.unitid),
        elem->task, &elem->taskdep);
    dephash_recycle_elem(elem);
    elem = next;
  }
  unhandled_remote_deps = NULL;
  return DART_OK;
}
#endif

dart_ret_t
dart_tasking_datadeps_release_unhandled_remote()
{
  dart_dephash_elem_t *rdep;
  DART_LOG_DEBUG("Handling previously unhandled remote dependencies: %p",
                 unhandled_remote_deps);
  dart_mutex_lock(&unhandled_remote_mutex);
  dart_dephash_elem_t *next = unhandled_remote_deps;
  while ((rdep = next) != NULL) {
    next = rdep->next;
    /**
     * Iterate over all possible tasks and find the closest-matching
     * local task that satisfies the remote dependency.
     * For tasks with a higher phase than the resolving task, send direct
     * task dependencies.
     */
    dart_global_unit_t origin = DART_GLOBAL_UNIT_ID(rdep->taskdep.gptr.unitid);
    dart_task_t *candidate = NULL;
    DART_LOG_DEBUG("Handling delayed remote dependency for task %p from unit %i",
                   rdep->task, origin.id);
    int slot = hash_gptr(rdep->taskdep.gptr);
    for (dart_dephash_elem_t *elem = local_deps[slot];
                              elem != NULL;
                              elem = elem->next) {
      dart_task_t *task = elem->task.local;
      // lock the task to avoid race condiditions in updating the state
      dart_mutex_lock(&task->mutex);
      if (elem->taskdep.gptr.addr_or_offs.addr == rdep->taskdep.gptr.addr_or_offs.addr &&
          task->state != DART_TASK_FINISHED) {
        /*
         * Remote INPUT task dependencies are considered to refer to the
         * previous phase. Use +1 here to avoid underflow.
         */
        if (!IS_OUT_DEP(elem->taskdep) || (task->phase + 1) > rdep->phase) {
          dart_mutex_unlock(&task->mutex);
          // send direct task dependency because this local task has to wait
          // for the remote task to finish
          // TODO: can we reduce the number of direct remote dependencies by
          //       considering transitive local dependencies?
          dart_tasking_remote_direct_taskdep(
              origin,
              task,
              rdep->task);
          int32_t unresolved_deps = DART_INC32_AND_FETCH(
                                      &task->unresolved_deps);
          DART_LOG_DEBUG("DIRECT task dep: task %p depends on remote %p at "
                         "unit %i and has %i dependencies",
                         task,
                         rdep->task,
                         origin.id,
                         unresolved_deps);
        } else {
          // check whether a previously encountered candidate
          // comes from an earlier phase than this candidate
          if (candidate == NULL || task->phase > candidate->phase) {
            // release the lock on the previous candidate
            if (candidate != NULL) {
              dart_mutex_unlock(&candidate->mutex);
            }
            // keep the current task/candidate locked until we find another
            // candidate or have added the remote_successor (below).
            candidate = task;
            DART_LOG_TRACE("Making local task %p a candidate for "
                           "remote task %p", candidate, rdep->task.remote);
          } else {
            dart_mutex_unlock(&task->mutex);
          }
        }
      } else {
        dart_mutex_unlock(&task->mutex);
      }
    }

    if (candidate != NULL) {
      // we have a local task to satisfy the remote task
      dart_dephash_elem_t *rs = dephash_allocate_elem(&rdep->taskdep, rdep->task);
      // the taskdep's gptr unit is used to store the origin
      rs->taskdep.gptr.unitid = origin.id;
      rs->phase = rdep->phase;
      DART_STACK_PUSH(candidate->remote_successor, rs);
      dart_mutex_unlock(&(candidate->mutex));
      DART_LOG_DEBUG("Found local task %p to satisfy remote dependency of "
                     "task %p from origin %i",
                     candidate, rdep->task.remote, origin.id);
    } else {
      // the remote dependency cannot be served --> send release
      DART_LOG_DEBUG("Releasing remote task %p from unit %i, "
                     "which was not handled in phase %i",
                     rdep->task.remote, origin.id,
                     rdep->phase);
      dart_tasking_remote_release(origin, rdep->task, &rdep->taskdep);
    }
  }

  unhandled_remote_deps = NULL;
  dart_mutex_unlock(&unhandled_remote_mutex);

  /**
   * Finally release all defered remote dependency releases
   */
  dart_mutex_lock(&deferred_remote_mutex);
  dart_dephash_elem_t *elem;
  next = deferred_remote_releases;
  while ((elem = next) != NULL) {
    next = elem->next;
    dart_task_t *task = elem->task.local;
    // immediately release the task
    int unresolved_deps = DART_DEC32_AND_FETCH(&task->unresolved_deps);
    DART_LOG_DEBUG("release_defered : Task with remote dep %p has %i "
                   "unresolved dependencies left", task, unresolved_deps);
    if (unresolved_deps < 0) {
      DART_LOG_ERROR("ERROR: task %p with remote dependency does not seem to "
                     "have unresolved dependencies!", task);
    } else if (unresolved_deps == 0) {
      // enqueue as runnable
      dart__base__tasking__enqueue_runnable(task);
    }
    dephash_recycle_elem(elem);
  }
  dart_mutex_unlock(&deferred_remote_mutex);

  return DART_OK;
}
/**
 * Find all tasks this task depends on and add the task to the dependency hash
 * table. All latest tasks are considered up to the first task with OUT|INOUT
 * dependency.
 */
dart_ret_t dart_tasking_datadeps_handle_task(
    dart_task_t           *task,
    const dart_task_dep_t *deps,
    size_t                 ndeps)
{
  dart_global_unit_t myid;
  dart_myid(&myid);
  DART_LOG_DEBUG("Datadeps: task %p has %zu data dependencies in phase %i",
                 task, ndeps, task->phase);
  for (size_t i = 0; i < ndeps; i++) {
    dart_task_dep_t dep = deps[i];
    // translate the offset to an absolute address
    dart_gptr_getoffset(dep.gptr, &dep.gptr.addr_or_offs.offset);
    int slot = hash_gptr(dep.gptr);
    DART_LOG_TRACE("Datadeps: task %p dependency %zu: type:%i unit:%i seg:%i addr:%p",
      task, i, dep.type, dep.gptr.unitid, dep.gptr.segid, dep.gptr.addr_or_offs.addr);

    if (dep.gptr.unitid != myid.id) {
      dart_tasking_remote_datadep(&dep, task);
    } else {
      /*
       * iterate over all dependent tasks until we find the first task with OUT|INOUT dependency on the same pointer
       */
      for (dart_dephash_elem_t *elem = local_deps[slot]; elem != NULL; elem = elem->next)
      {
        DART_ASSERT_MSG(elem->task.local != task, "Task already present in dependency hashmap!");
        DART_LOG_TRACE("Task %p local dependency on %p (s:%i) vs %p (s:%i) of task %p",
                          task, dep.gptr.addr_or_offs.addr, dep.gptr.segid,
                          elem->taskdep.gptr.addr_or_offs.addr, elem->taskdep.gptr.segid,
                          elem->task.local);
        if (elem->taskdep.gptr.addr_or_offs.addr == dep.gptr.addr_or_offs.addr) {
          dart_mutex_lock(&(elem->task.local->mutex));
          DART_LOG_TRACE("Checking task %p against task %p (deptype: %i vs %i)", elem->task.local, task, elem->taskdep.type, dep.type);
          if (elem->task.local->state != DART_TASK_FINISHED &&
              (IS_OUT_DEP(dep) || (dep.type == DART_DEP_IN  && IS_OUT_DEP(elem->taskdep)))){
            // OUT dependencies have to wait for all previous dependencies
            int32_t unresolved_deps = DART_INC32_AND_FETCH(&task->unresolved_deps);
            DART_LOG_DEBUG("Making task %p a local successor of task %p (successor: %p, num_deps: %i)",
                      task, elem->task.local, elem->task.local->successor, unresolved_deps);
            dart_tasking_tasklist_prepend(&(elem->task.local->successor), task);
          }
          dart_mutex_unlock(&(elem->task.local->mutex));
          if (IS_OUT_DEP(elem->taskdep)) {
            // we can stop at the first OUT|INOUT dependency
            DART_LOG_TRACE("Stopping search for dependencies for task %p at first OUT dependency encountered from task %p!", task, elem->task.local);;
            break;
          }
        }
      }

      taskref tr;
      tr.local = task;
      // add this task to the hash table
      dephash_add_local(&dep, tr);

      // can we resolve some previously unhandled
      // remote dependencies with this task?
#if 0
      check_unresolved_remote_deps(task, &dep);
#endif
    }
  }

  return DART_OK;
}

/**
 * Look for the latest task that satisfies \c dep of a remote task pointed
 * to by \c rtask and add it to the remote successor list.
 * Note that \c dep has to be a IN dependency.
 */
dart_ret_t dart_tasking_datadeps_handle_remote_task(
    const dart_phase_dep_t *rdep,
    const taskref           remote_task,
    dart_global_unit_t      origin)
{
  if (rdep->dep.type != DART_DEP_IN) {
    DART_LOG_ERROR("Remote dependencies with type other than DART_DEP_IN are not supported!");
    return DART_ERR_INVAL;
  }
#if 0
  int slot = hash_gptr(rdep->dep.gptr);
  for (dart_dephash_elem_t *elem = local_deps[slot]; elem != NULL; elem = elem->next) {
    if (elem->taskdep.gptr.addr_or_offs.offset == rdep->dep.gptr.addr_or_offs.offset
        && IS_OUT_DEP(elem->taskdep)) {
      dart_task_t *task = elem->task.local;

      dart_mutex_lock(&(task->mutex));
      if (task->state != DART_TASK_FINISHED) {
        if (task->phase <= rdep->phase) {
          // the local task will resolve this remote dependency
          dart_dephash_elem_t *rs = dephash_allocate_elem(&rdep->dep, remote_task);
          // the taskdep's gptr unit is used to store the origin
          rs->taskdep.gptr.unitid = origin.id;
          rs->phase = rdep->phase;
          DART_STACK_PUSH(task->remote_successor, rs);
          dart_mutex_unlock(&(task->mutex));
        } else {
          dart_mutex_unlock(&(task->mutex));
          // the local task has a direct dependency to remote_task
          dart_tasking_remote_direct_taskdep(origin, task, remote_task);
          int32_t unresolved_deps = DART_INC32_AND_FETCH(
                                      &elem->task.local->unresolved_deps);
          DART_LOG_DEBUG("handle_remote_task: task %p has %i dependencies",
                         task,
                         unresolved_deps);
          continue;
        }
      } else if (task->phase <= rdep->phase) {
        dart_mutex_unlock(&(task->mutex));
        // the task is already finished --> send release immediately
        dart_tasking_remote_release(origin, remote_task, &rdep->dep);
      }
      DART_LOG_DEBUG("Found local task %p to satisfy remote dependency of task %p from origin %i",
        task, remote_task.remote, origin.id);
      return DART_OK;
    }
  }
#endif

  DART_LOG_INFO("Enqueuing remote task %p from unit %i for later resolution",
    remote_task.remote, origin.id);
  // cache this request and resolve it later
  dart_dephash_elem_t *rs = dephash_allocate_elem(&rdep->dep, remote_task);
  dart_mutex_lock(&unhandled_remote_mutex);
  rs->taskdep.gptr.unitid = origin.id;
  rs->phase = rdep->phase;
  DART_STACK_PUSH(unhandled_remote_deps, rs);
  dart_mutex_unlock(&unhandled_remote_mutex);
  return DART_OK;
}


/**
 * Handle the direct task dependency between a local task and
 * it's remote successor
 */
dart_ret_t dart_tasking_datadeps_handle_remote_direct(
    dart_task_t       *local_task,
    taskref            remote_task,
    dart_global_unit_t origin)
{
  bool enqueued = false;
  dart_task_dep_t dep;
  dep.type = DART_DEP_DIRECT;
  dep.gptr = DART_GPTR_NULL;
  dep.gptr.unitid = origin.id;
  DART_LOG_DEBUG("Remote direct task dependency for task %p: %p",
      local_task, remote_task.remote);
  if (local_task->state != DART_TASK_FINISHED) {
    dart_mutex_lock(&(local_task->mutex));
    if (local_task->state != DART_TASK_FINISHED) {
      dart_dephash_elem_t *rs = dephash_allocate_elem(&dep, remote_task);
      DART_STACK_PUSH(local_task->remote_successor, rs);
      enqueued = true;
    }
    dart_mutex_unlock(&(local_task->mutex));
  }

  if (!enqueued) {
    // local task done already --> release immediately
    dart_tasking_remote_release(origin, remote_task, &dep);
  }

  return DART_OK;
}

/**
 * Release remote and local dependencies of a local task
 */
dart_ret_t dart_tasking_datadeps_release_local_task(
    dart_task_t   *task)
{
  release_remote_dependencies(task);

  // release local successors
  task_list_t *tl = task->successor;
  while (tl != NULL) {
    task_list_t *tmp = tl->next;
    int32_t unresolved_deps = DART_DEC32_AND_FETCH(&tl->task->unresolved_deps);
    DART_LOG_DEBUG("release_local_task: task %p has %i dependencies left", tl->task, unresolved_deps);

    if (unresolved_deps < 0) {
      DART_LOG_ERROR("release_local_task: task %p has negative number of dependencies:  %i", tl->task, unresolved_deps);
    } else if (unresolved_deps == 0) {
      dart__base__tasking__enqueue_runnable(tl->task);
    }

    dart_tasking_tasklist_deallocate_elem(tl);

    tl = tmp;
  }

  return DART_OK;
}

dart_ret_t dart_tasking_datadeps_release_remote_dep(
  dart_task_t *local_task)
{
  // block the release of the task if it's not to be executed yet
  dart_mutex_lock(&deferred_remote_mutex);
  if (local_task->phase > dart__base__tasking__phase_bound()) {
    // dummy dependency
    dart_task_dep_t dep = {
        .gptr = DART_GPTR_NULL,
        .type = DART_DEP_DIRECT
    };
    taskref ref = {.local = local_task};
    dart_dephash_elem_t *dr = dephash_allocate_elem(&dep, ref);
    DART_STACK_PUSH(deferred_remote_releases, dr);
    DART_LOG_DEBUG("release_remote_dep : Defering release of task %p "
                   "with remote dep from phase %lu",
                   local_task, local_task->phase);
  } else {
    // immediately release the task
    int unresolved_deps = DART_DEC32_AND_FETCH(&local_task->unresolved_deps);
    DART_LOG_DEBUG("release_remote_dep : Task %p with remote dep has %i "
                   "unresolved dependencies left", local_task, unresolved_deps);
    if (unresolved_deps < 0) {
      DART_LOG_ERROR("ERROR: task %p with remote dependency does not seem to "
                     "have unresolved dependencies!", local_task);
    } else if (unresolved_deps == 0) {
      // enqueue as runnable
      dart__base__tasking__enqueue_runnable(local_task);
    }
  }
  dart_mutex_unlock(&deferred_remote_mutex);
  return DART_OK;
}

dart_ret_t dart_tasking_datadeps_end_phase(uint64_t phase)
{
  // nothing to be done for now
  return DART_OK;
}


/**
 * Send direct dependency requests for tasks that have to block until the
 * remote dependency \c remotedep is executed, i.e., local OUT|INOUT tasks
 * cannot run before remote IN dependencies have been executed.
 */
static dart_ret_t
send_direct_dependencies(const dart_dephash_elem_t *remotedep)
{
  // nothing to do for direct task dependencies
  if (remotedep->taskdep.type == DART_DEP_DIRECT)
    return DART_OK;

  int slot = hash_gptr(remotedep->taskdep.gptr);
  for (dart_dephash_elem_t *elem = local_deps[slot];
       elem != NULL;
       elem = elem->next) {

    /**
     * if the task has no dependencies anymore it is already (being) executed
     * this is also the last task to consider since previous tasks will have
     * been released as well
     */
    // if the task has no dependencies anymore it is already (being) executed
    // this is also the last task to consider since previous tasks will have
    // been released as well
    if (DART_FETCH32(&elem->task.local->unresolved_deps) == 0) {
      DART_LOG_TRACE("send_direct_dependencies: task %p has no pending "
                     "dependencies, skipping.", elem->task.local);
      break;
    }

    if (elem->taskdep.gptr.addr_or_offs.addr ==
            remotedep->taskdep.gptr.addr_or_offs.addr
        && IS_OUT_DEP(elem->taskdep)) {

      DART_LOG_DEBUG("send_direct_dependencies: task %p has direct dependency "
                     "to %p", elem->task.local, remotedep->task);
      int ret = dart_tasking_remote_direct_taskdep(
                  DART_GLOBAL_UNIT_ID(remotedep->taskdep.gptr.unitid),
                  elem->task.local,
                  remotedep->task);
      if (ret != DART_OK) {
        DART_LOG_ERROR("send_direct_dependencies ! Failed to send direct "
                       "dependency request for task %p",
                       elem->task.local);
        return ret;
      }

      // this task now needs to wait for the remote task to complete
      int32_t unresolved_deps = DART_INC32_AND_FETCH(
                                  &elem->task.local->unresolved_deps);
      DART_LOG_DEBUG("send_direct_dependencies: task %p has %i dependencies",
                     elem->task.local,
                     unresolved_deps);
    }
  }

  return DART_OK;
}


/**
 * Release the remote dependencies of \c task.
 * Also registers direct task dependencies for tasks dependent on \c task.
 */
static dart_ret_t release_remote_dependencies(dart_task_t *task)
{
  DART_LOG_TRACE("Releasing remote dependencies for task %p (rs:%p)",
                 task, task->remote_successor);
  dart_dephash_elem_t *rs = task->remote_successor;
  while (rs != NULL) {
    dart_dephash_elem_t *tmp = rs;
    rs = rs->next;

    // before sending the release we send direct task dependencies for
    // local tasks dependening on this task
//    send_direct_dependencies(tmp);

    // send the release
    dart_tasking_remote_release(
        DART_GLOBAL_UNIT_ID(tmp->taskdep.gptr.unitid),
        tmp->task,
        &tmp->taskdep);
    dephash_recycle_elem(tmp);
  }
  task->remote_successor = NULL;
  return DART_OK;
}
