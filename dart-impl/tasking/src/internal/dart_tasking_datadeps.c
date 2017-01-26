
#include <dash/dart/base/logging.h>
#include <dash/dart/base/mutex.h>
#include <dash/dart/base/atomic.h>
#include <dash/dart/if/dart_tasking.h>
#include <dash/dart/tasking/dart_tasking_priv.h>
#include <dash/dart/tasking/dart_tasking_tasklist.h>
#include <dash/dart/tasking/dart_tasking_datadeps.h>
#include <dash/dart/tasking/dart_tasking_remote.h>
#include <dash/dart/tasking/dart_tasking_taskqueue.h>

#define DART_DEPHASH_SIZE 1024
typedef struct dart_dephash_elem dart_dephash_elem_t;


/**
 * Management of task data dependencies using a hash map that maps pointers to tasks.
 * The hash map implementation is taken from dart_segment.
 * The hash uses the absolute local address stored in the gptr since that is used
 * throughout the task handling code.
 */

#define IS_OUT_DEP(taskdep) (((taskdep).type == DART_DEP_OUT || (taskdep).type == DART_DEP_INOUT))

struct dart_dephash_elem {
  dart_dephash_elem_t *next;
  union taskref        task;
  dart_task_dep_t      taskdep;
};

static dart_dephash_elem_t *local_deps[DART_DEPHASH_SIZE];
static dart_dephash_elem_t *freelist_head = NULL;
static dart_mutex_t         local_deps_mutex;


static dart_ret_t release_remote_dependencies(dart_task_t *task);

static inline int hash_gptr(dart_gptr_t gptr)
{
  /*
   * Use the upper 61 bit of the pointer since we assume that pointers are 8-byte aligned.
   * */
  return ((gptr.addr_or_offs.offset >> 3) % DART_DEPHASH_SIZE);
}

/**
 * @brief Initialize the data dependency management system.
 */
dart_ret_t dart_tasking_datadeps_init()
{
  memset(local_deps, 0, sizeof(dart_dephash_elem_t*) * DART_DEPHASH_SIZE);

  dart_mutex_init(&local_deps_mutex);

  return dart_tasking_remote_init();
}

dart_ret_t dart_tasking_datadeps_fini()
{
  return dart_tasking_remote_fini();
}

/**
 * @brief Check for new remote task dependency requests coming in
 */
dart_ret_t dart_tasking_datadeps_progress()
{
  return dart_tasking_remote_progress();
}

/**
 * @brief Allocate a new element for the dependency hash, possibly from a free-list
 */
static dart_dephash_elem_t * dephash_allocate_elem(const dart_task_dep_t *dep, taskref task)
{
  // take an element from the free list if possible
  dart_dephash_elem_t *elem = NULL;
  if (freelist_head != NULL) {
    dart_mutex_lock(&local_deps_mutex);
    if (freelist_head != NULL) {
      elem = freelist_head;
      freelist_head = freelist_head->next;
      elem->next = NULL;
    }
    dart_mutex_unlock(&local_deps_mutex);
  }

  if (elem == NULL){
    elem = calloc(1, sizeof(dart_dephash_elem_t));
  }

  elem->task = task;
  elem->taskdep = *dep;

  return elem;
}


/**
 * @brief Allocate a new element for the dependency hash, possibly from a free-list
 */
static void dephash_recycle_elem(dart_dephash_elem_t *elem)
{
  // TODO: this needs locking
  if (elem != NULL) {
    dart_mutex_lock(&local_deps_mutex);
    elem->next = freelist_head;
    freelist_head = elem;
    dart_mutex_unlock(&local_deps_mutex);
  }
}

/**
 * @brief Add a task with dependency to the local dependency hash table.
 */
static dart_ret_t dephash_add_local(const dart_task_dep_t *dep, taskref task)
{
  dart_dephash_elem_t *elem = dephash_allocate_elem(dep, task);
  // put the new entry at the beginning of the list
  int slot = hash_gptr(dep->gptr);
  dart_mutex_lock(&local_deps_mutex);
  elem->next = local_deps[slot];
  local_deps[slot] = elem;
  dart_mutex_unlock(&local_deps_mutex);

  return DART_OK;
}

/**
 * @brief Find all tasks this task depends on and add the task to the dependency hash table.
 *        All latest tasks are considered up to the first task with OUT|INOUT dependency.
 */
dart_ret_t dart_tasking_datadeps_handle_task(dart_task_t *task, dart_task_dep_t *deps, size_t ndeps)
{
  dart_global_unit_t myid;
  dart_myid(&myid);
  for (size_t i = 0; i < ndeps; i++) {
    dart_task_dep_t *dep = &deps[i];
    int slot = hash_gptr(dep->gptr);

    if (dep->gptr.unitid != myid.id) {
      dart_tasking_remote_datadep(dep, task);
    } else {
      /*
       * iterate over all dependent tasks until we find the first task with OUT|INOUT dependency on the same pointer
       */
      for (dart_dephash_elem_t *elem = local_deps[slot]; elem != NULL; elem = elem->next)
      {
        if (elem->taskdep.gptr.addr_or_offs.addr == dep->gptr.addr_or_offs.addr
            && dep->gptr.unitid == myid.id
            && elem->task.local != task) {
          if (IS_OUT_DEP(*dep) || (dep->type == DART_DEP_IN && IS_OUT_DEP(elem->taskdep))){
            // OUT dependencies have to wait for all previous dependencies
            dart_mutex_lock(&(elem->task.local->mutex));
            dart_tasking_tasklist_prepend(&(elem->task.local->successor), task);
            dart_mutex_lock(&(elem->task.local->mutex));
            DART_FETCH_AND_INC32(&task->unresolved_deps);
          }
          if (IS_OUT_DEP(elem->taskdep)) {
            // we can stop at the first OUT|INOUT dependency
            break;
          }
        }
      }

      taskref tr;
      tr.local = task;
      // add this task to the hash table
      dephash_add_local(dep, tr);
    }
  }

  return DART_OK;
}

/**
 * Look for the latest task that satisfies \c dep of a remote task pointed to by \c rtask and add it to the remote successor list.
 * Note that \c dep has to be a IN dependency.
 */
dart_ret_t dart_tasking_datadeps_handle_remote_task(const dart_task_dep_t *dep, const taskref remote_task, dart_global_unit_t origin)
{
  if (dep->type != DART_DEP_IN) {
    DART_LOG_ERROR("Remote dependencies with type other than DART_DEP_IN are not supported!");
    return DART_ERR_INVAL;
  }

  int slot = hash_gptr(dep->gptr);
  for (dart_dephash_elem_t *elem = local_deps[slot]; elem != NULL; elem = elem->next) {
    if (elem->taskdep.gptr.addr_or_offs.offset == dep->gptr.addr_or_offs.offset
        && IS_OUT_DEP(elem->taskdep)) {
      dart_dephash_elem_t *rs = dephash_allocate_elem(dep, remote_task);
      // the taskdep's gptr unit is used to store the origin
      elem->taskdep.gptr.unitid = origin.id;
      dart_mutex_lock(&(elem->task.local->mutex));
      rs->next = elem->task.local->remote_successor;
      elem->task.local->remote_successor = rs;
      dart_mutex_unlock(&(elem->task.local->mutex));

      return DART_OK;
    }
  }

  DART_LOG_ERROR("Cannot find local task that satisfies remote dependency %p for task %p", dep->gptr.addr_or_offs.addr, remote_task.remote);
  return DART_ERR_NOTFOUND;
}


/**
 * @brief Handle the direct task dependency between a local task and it's remote successor
 */
dart_ret_t dart_tasking_datadeps_handle_remote_direct(dart_task_t *local_task, taskref remote_task, dart_global_unit_t origin)
{
  dart_task_dep_t dep;
  dep.type = DART_DEP_DIRECT;
  dep.gptr = DART_GPTR_NULL;
  dep.gptr.unitid = origin.id;
  dart_dephash_elem_t *rs = dephash_allocate_elem(&dep, remote_task);
  dart_mutex_lock(&(local_task->mutex));
  rs->next = local_task->remote_successor;
  local_task->remote_successor = rs;
  dart_mutex_unlock(&(local_task->mutex));

  return DART_OK;
}

/**
 * @brief Release remote and local dependencies of a local task
 */
dart_ret_t dart_tasking_datadeps_release_local_task(dart_thread_t *thread, dart_task_t *task)
{
  release_remote_dependencies(task);

  // release local successors
  task_list_t *tl = task->successor;
  while (tl != NULL) {
    task_list_t *tmp = tl->next;
    int unresolved_deps = DART_FETCH_AND_DEC32(&tl->task->unresolved_deps);

    if (unresolved_deps == 0) {
      dart_tasking_taskqueue_push(&thread->queue, task);
    }

    dart_tasking_tasklist_deallocate_elem(tl);

    tl = tmp;
  }

  return DART_OK;
}


/**
 * @brief Send direct dependency requests for tasks that have to block until the remote
 *        dependency \c remotedep is executed, i.e., local OUT|INOUT tasks cannot run before
 *        remote IN dependencies have been executed.
 */
static dart_ret_t send_direct_dependencies(dart_dephash_elem_t *remotedep)
{
  int slot = hash_gptr(remotedep->taskdep.gptr);

  // nothing to do for direct task dependencies
  if (remotedep->taskdep.type == DART_DEP_DIRECT)
    return DART_OK;

  for (dart_dephash_elem_t *elem = local_deps[slot]; elem != NULL; elem = elem->next) {

    // if the task has no dependencies anymore it is already (being) executed
    // this is also the last task to consider since previous tasks will have been release as well
    if (elem->task.local->unresolved_deps == 0)
      break;

    if (elem->taskdep.gptr.addr_or_offs.addr == remotedep->taskdep.gptr.addr_or_offs.addr && IS_OUT_DEP(elem->taskdep)) {

      int ret = dart_tasking_remote_direct_taskdep(DART_GLOBAL_UNIT_ID(remotedep->taskdep.gptr.unitid),
                                                   elem->task.local, remotedep->task);
      if (ret != DART_OK)
        return ret;

      // this task now needs to wait for the remote task to complete
      DART_FETCH_AND_INC32(&elem->task.local->unresolved_deps);
    }
  }

  return DART_OK;
}


/**
 * Release the remote dependencies of \c task.
 */
static dart_ret_t release_remote_dependencies(dart_task_t *task)
{
  dart_dephash_elem_t *rs = task->remote_successor;
  while (rs != NULL) {
    dart_dephash_elem_t *tmp = rs->next;

    // before sending the release we send direct task dependencies for local tasks
    send_direct_dependencies(rs);

    // send the release
    dart_tasking_remote_release(DART_GLOBAL_UNIT_ID(rs->taskdep.gptr.unitid), rs->task, &rs->taskdep);
    dephash_recycle_elem(rs);
    rs = tmp;
  }

  return DART_OK;
}
