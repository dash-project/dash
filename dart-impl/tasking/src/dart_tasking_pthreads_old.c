#if 0
//#if defined(DAT_TASKING_PTHREADS)

#include <dash/dart/base/logging.h>
#include <dash/dart/if/dart_tasking.h>
#include <dash/dart/if/dart_active_messages.h>
#include <dash/dart/base/hwinfo.h>
#include <dash/dart/tasking/dart_tasking_priv.h>
#include <dash/dart/tasking/dart_tasking_ayudame.h>

#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h>
#include <dlfcn.h>


typedef void (dart_task_action_t) (void *);

static pthread_cond_t task_avail_cond = PTHREAD_COND_INITIALIZER;
static pthread_cond_t tasks_done_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t thread_pool_mutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_key_t tpd_key;

static dart_amsgq_t amsgq;

static pthread_t *thread_pool;

static bool processing = false;
static bool initialized = false;

struct remote_data_dep {
  dart_gptr_t gptr;
  dart_unit_t runit;
  task_t     *rtask; // pointer to a task on the origin unit, do not dereference remotely!
};

struct remote_task_dep {
  task_t     *task;
  task_t     *followup;
  dart_unit_t runit;
};

typedef struct {
  task_t *current_task;
  int     thread_id;
} tpd_t;


#define DART_RTASK_QLEN 256

// the queue of runnable tasks
static task_t *runq_head = NULL;
static task_t *runq_tail = NULL;

// the queue of remote dependency requests waiting for release
static task_t *rdepq_head = NULL;

// free-list for tasks
static task_t *free_tasks = NULL;

// list of tasks that are dependent
static task_list_t *dep_graph = NULL;


// true if threads should process tasks. Set to false to quit parallel processing
static bool parallel;

static int num_threads;
static int threads_running = 0;

static inline task_t * deqeue_runnable(void);
static inline void enqeue_runnable(task_t *task);

static inline task_t * allocate_task();

static inline void deallocate_task(task_t *t);

static void release_task(task_t *task);

static void destroy_tsd(void *tsd);

static void wait_for_work();

static void set_current_task(task_t *t);
static task_t * get_current_task();

static void release_remote_dependency(void *data);
static void enqueue_from_remote(void *data);
static void remote_dependency_request(task_t *task, dart_gptr_t gptr);

/**
 * Take a task from the task queue and execute it.
 * NOTE: threads have to hold the thread_pool_mutex before entering here.
 * TODO: Implement more fine-grained locking.
 */
static void handle_task()
{
  // stop immediately if we are not yet processing tasks
  if (!processing)
    return;
  // remove from runnable queue and execute
  task_t *task = deqeue_runnable();
  set_current_task(task);
  if (task != NULL)
  {
    DART_LOG_INFO("Thread %i executing task %p", dart__base__tasking__thread_num(), task);
    pthread_mutex_unlock(&thread_pool_mutex);

    dart_task_action_t *fn = task->fn;
    void *data = task->data;

    //invoke the task function
    fn(data);

    pthread_mutex_lock(&thread_pool_mutex);
    release_task(task);
    deallocate_task(task);
  }
}

/**
 * Adds the two timespecs
 */
static inline struct timespec add_timespec(const struct timespec a, const struct timespec b)
{
  struct timespec res = a;
  res.tv_sec  += b.tv_sec;
  res.tv_nsec += b.tv_nsec;
  res.tv_sec  += res.tv_nsec / (long int)1E9;
  res.tv_nsec %= (long int)1E9;
  return res;
}

static void* thread_main(void *data)
{
  tpd_t *tpd = (tpd_t*)data;
  pthread_setspecific(tpd_key, tpd);

  // this is only done once upon thread creation
  pthread_mutex_lock(&thread_pool_mutex);
  threads_running++;
  pthread_mutex_unlock(&thread_pool_mutex);

  // enter work loop
  while (parallel) {
    // look for incoming remote tasks and responses
    dart_amsg_process(amsgq);
    pthread_mutex_lock(&thread_pool_mutex);
    if (runq_head == NULL) {
      threads_running--;
      if (threads_running == 0) {
        // signal that all threads are done
        pthread_cond_signal(&tasks_done_cond);
      }
      wait_for_work();
      threads_running++;
    }
    handle_task();
    pthread_mutex_unlock(&thread_pool_mutex);
  }

  pthread_mutex_lock(&thread_pool_mutex);
  threads_running--;
  pthread_mutex_unlock(&thread_pool_mutex);

  return NULL;
}

dart_ret_t
dart__base__tasking__init()
{
  int i;
  parallel = true;

  dart_hwinfo_t hw;
  dart_hwinfo(&hw);
  if (hw.num_cores > 0) {
    num_threads = hw.num_cores * hw.max_threads;
  } else {
    DART_LOG_INFO("Failed to get number of cores! Playing it safe with 2 threads...");
    num_threads = 2;
  }
  num_threads=1;
  DART_LOG_INFO("Creating %i threads", num_threads);

  dart_amsg_init();

  // set up the active message queue
  amsgq  = dart_amsg_openq(sizeof(struct remote_data_dep) * DART_RTASK_QLEN, DART_TEAM_ALL);
  DART_LOG_INFO("Created active message queue for remote tasking (%p)", amsgq);

  pthread_key_create(&tpd_key, &destroy_tsd);
  // set master thread id
  tpd_t *tpd = malloc(sizeof(tpd_t));
  tpd->thread_id = 0;
  tpd->current_task = NULL;
  pthread_setspecific(tpd_key, tpd);

  thread_pool = malloc(sizeof(pthread_t) * num_threads);
  for (i = 0; i < num_threads-1; i++)
  {
    tpd = malloc(sizeof(tpd_t));
    tpd->thread_id = i + 1; // 0 is reserved for master thread
    tpd->current_task = NULL;
    int ret = pthread_create(&thread_pool[i], NULL, &thread_main, tpd);
    if (ret != 0) {
      DART_LOG_ERROR("Failed to create thread %i of %i!", (i+1), num_threads);
    }
  }

  dart__tasking__ayudame_init();

  initialized = true;

  return DART_OK;
}

int
dart__base__tasking__thread_num()
{
  return (initialized ? ((tpd_t*)pthread_getspecific(tpd_key))->thread_id : 0);
}

int
dart__base__tasking__num_threads()
{
  return (initialized ? num_threads : 1);
}

dart_ret_t
dart__base__tasking__fini()
{
  int i;

  parallel = false;

  for (i = 0; i < num_threads -1; i++) {
    pthread_cancel(thread_pool[i]);
    // TODO: Fix thread teardown!
//    pthread_join(thread_pool[i], (void**)NULL);
  }

  while (free_tasks != NULL) {
    task_t *next = free_tasks->next;
    free_tasks->next = NULL;
    free(free_tasks);
    free_tasks = next;
  }

  dart_amsg_closeq(amsgq);

  dart__tasking__ayudame_fini();

  initialized = false;

  return DART_OK;
}

static task_t * find_dependency_in_list(task_list_t *parent, const dart_task_dep_t *dep, dart_task_deptype_t deptype);
static inline task_t * find_dependency(task_t *task, const dart_task_dep_t *dep, dart_task_deptype_t deptype)
{
  size_t i;
  task_t *t = NULL;

  if (task != NULL)
  {
    // does this task satisfy the dependency?
    for (i = 0; i < task->ndeps; i++) {
      if ((task->deps[i].type == deptype || task->deps[i].type == DART_DEP_INOUT || (deptype == DART_DEP_OUT && task->deps[i].type == DART_DEP_IN))
//          && DART_GPTR_EQUAL(task->deps[i].gptr, dep->gptr)) {
          && task->deps[i].gptr.addr_or_offs.offset == dep->gptr.addr_or_offs.offset
          && task->deps[i].gptr.unitid == dep->gptr.unitid) {
        // this task will resolve the dependency
        t = task;
        break;
      }
    }

    if (t == NULL) {
      t = find_dependency_in_list(task->successor, dep, deptype);
    }
  }
  return t;
}

/**
 * TODO: This is broken, somehow, as it does not find all relevant dependencies.
 * IDEA: Similar to LLVM, use a hash map to match dependencies.
 *       Executed tasks remain in the hash map until their phase is over (once we introduced phases).
 *       An IN dependency is served by the last task that has a matching OUT dependency.
 *       An OUT dependency is dependent on all IN/INOUT tasks with a matching dependency.
 */
static task_t * find_dependency_in_list(task_list_t *list, const dart_task_dep_t *dep, dart_task_deptype_t deptype)
{
  task_t *task = NULL;
  if (list != NULL) {
    // look at the task and it's dependents first
    task = find_dependency(list->task, dep, deptype);

    if (task == NULL) {
      task = find_dependency_in_list(list->next, dep, deptype);
    }
  }
  return task;
}

dart_ret_t
dart__base__tasking__create_task(void (*fn) (void *), void *data, size_t data_size, dart_task_dep_t *deps, size_t ndeps)
{
  dart_unit_t myid;
  size_t i;
  bool has_dep = false;
  pthread_mutex_lock(&thread_pool_mutex);

  dart_myid(&myid);

  /**
   * Set up the task data structure
   */
  task_t *task = allocate_task();
  task->fn = fn;
  task->parent = get_current_task();
  task->ndeps = ndeps;
  task->deps = malloc(sizeof(dart_task_dep_t) * ndeps);
//  memcpy(task->deps, deps, sizeof(dart_task_dep_t) * ndeps);
  // store dependencies using absolute addresses
  for (i = 0; i < ndeps; i++) {
    task->deps[i].type = deps[i].type;
    task->deps[i].gptr.flags = deps[i].gptr.flags;
    task->deps[i].gptr.segid = 0;
    task->deps[i].gptr.unitid = deps[i].gptr.unitid;
    if (dart_gptr_getoffset(deps[i].gptr, &(task->deps[i].gptr.addr_or_offs.offset)) != DART_OK) {
      DART_LOG_ERROR("Failed to get offset for gptr={f=%i, s=%i, u=%i, o=%p}", deps[i].gptr.flags, deps[i].gptr.segid, deps[i].gptr.unitid, deps[i].gptr.addr_or_offs.addr);
      task->deps[i].gptr.addr_or_offs.offset = 0;
    }
  }

  dart__tasking__ayudame_create_task(task, task->parent);

  if (data != NULL && data_size > 0) {
    task->data_size = data_size;
    task->data = malloc(data_size);
    memcpy(task->data, data, data_size);
  } else {
    task->data = data;
    task->data_size = 0;
  }

  /**
   * Handle dependencies
   */
  if (ndeps > 0) {
    for (i = 0; i < ndeps; i++) {
//      if (task->deps[i].type == DART_DEP_IN || task->deps[i].type == DART_DEP_INOUT) {
        // put the task into the dependency graph

        if (task->deps[i].gptr.unitid != myid) {

          if (task->deps[i].type == DART_DEP_IN) {
            // create a dependency request at the remote unit
            remote_dependency_request(task, task->deps[i].gptr);
            task->unresolved_deps++;
            // enqueue this task at the top level of the task graph
            task_list_t *tl = dart_tasking_tasklist_allocate_elem();
            tl->task = task;
            tl->next = dep_graph;
            dep_graph = tl;

            has_dep = true;
          } else {
            DART_LOG_ERROR("Found task %p with remote OUT|INOUT dependency which is currently not supported!", task);
            return DART_ERR_INVAL;
          }
        } else {

          task_t *dependee = find_dependency_in_list(dep_graph, &task->deps[i], DART_DEP_OUT);
          if (dependee != NULL) {
            // enqueue as dependant of this task
            task_list_t *tl = dart_tasking_tasklist_allocate_elem();
            tl->task = task;
            tl->next = dependee->successor;
            dependee->successor = tl;

            has_dep = true;
            task->unresolved_deps++;

            dart__tasking__ayudame_add_dependency(dependee, task);
            DART_LOG_INFO("Task %p depends on task %p", task, dependee);
          } else {
            DART_LOG_INFO("Could not find a dependee for dependency (dart_gptr = {addr=%p, seg=%i, unit=%i}}) on task %p",
                task->deps[i].gptr.addr_or_offs.addr, task->deps[i].gptr.segid, task->deps[i].gptr.unitid, task);
          }
        }
//      } else if (task->deps[i].type == DART_DEP_OUT) {
//        DART_LOG_INFO("Found task %p with OUT dep (dart_gptr = {addr=%p, seg=%i, unit=%i}})", task,
//            task->deps[i].gptr.addr_or_offs.addr, task->deps[i].gptr.segid, task->deps[i].gptr.unitid);
//      }
    }
    if (!has_dep) {
      // no direct dependency found
      // -> enqueue on top level of dependency graph
      DART_LOG_INFO("Adding task %p to global task dependency graph", task);
      task_list_t *tl = dart_tasking_tasklist_allocate_elem();
      tl->task = task;
      tl->next = dep_graph;
      dep_graph = tl;
      enqeue_runnable(task);
    }
  } else {
    // no dependencies
    enqeue_runnable(task);
  }

  pthread_mutex_unlock(&thread_pool_mutex);

  return DART_OK;
}

/**
 * This function is only called by the master thread and
 * starts the actual processing of tasks.
 */
dart_ret_t
dart__base__tasking__task_complete()
{
  // wake up all threads and start processing
  processing = true;
  if (dep_graph != NULL || runq_head != NULL ) {
    pthread_cond_broadcast(&task_avail_cond);
  }

  while (dep_graph != NULL || runq_head != NULL || threads_running > 0){
    // look for remote tasks coming in
//    dart_amsg_process(requestq);
    // look for responses coming in
//    dart_amsg_process(responseq);
    // participate in the task completion
    pthread_mutex_lock(&thread_pool_mutex);
    handle_task();
//    if (dep_graph != NULL || runq_head != NULL) {
//      pthread_mutex_unlock(&thread_pool_mutex);
//      continue;
//    }
//    if (threads_running > 0) {
//      // threads are still running, wait for them to signal completion
//      pthread_mutex_unlock(&thread_pool_mutex);
//
//      pthread_mutex_lock(&tasks_done_mutex);
//      DART_LOG_INFO("Master: waiting for all tasks to finish");
//      pthread_cond_wait(&tasks_done_cond, &tasks_done_mutex);
//      DART_LOG_INFO("Master: all tasks finished");
//      pthread_mutex_unlock(&tasks_done_mutex);
//    } else {
//      DART_LOG_INFO("Master: no active tasks running");
//
//    }
    pthread_mutex_unlock(&thread_pool_mutex);
    // look for requests and responses coming in
    dart_amsg_process(amsgq);
  }
  // stop processing until we hit this function again
  processing = false;
  return DART_OK;
}

/**
 * Private functions -- all of them have to be protected by a mutex!
 */

static inline task_t * deqeue_runnable(void) {
  task_t *task = NULL;
  if (runq_head != NULL) {
    task = runq_head;
    runq_head = runq_head->next;
    DART_LOG_INFO("Dequeued task %p from runnable list (fn=%p, data=%p)", task, task->fn, task->data);
  }
  if (runq_head == NULL) {
    runq_tail = NULL;
  }

  return task;
}

static inline void enqeue_runnable(task_t *task)
{
  // todo: append this task to the tail
  DART_LOG_INFO("Enqueuing task %p into runnable list (fn=%p, data=%p)", task, task->fn, task->data);
  task->next = NULL;
  if (runq_tail != NULL) {
    runq_tail->next = task;
  }
  runq_tail = task;

  if (runq_head == NULL) {
    runq_head = runq_tail;
  }

  // wake up a thread (if anyone is waiting)
  pthread_cond_signal(&task_avail_cond);

}

static inline task_t * allocate_task() {
  task_t *t = NULL;
  if (free_tasks != NULL) {
    t = free_tasks;
    free_tasks = free_tasks->next;
  } else {
    t = calloc(1, sizeof(task_t));
  }
  return t;
}

static inline void deallocate_task(task_t *t) {
  if (t->data_size > 0) {
    free(t->data);
  }
  free(t->deps);
  memset(t, 0, sizeof(task_t));
  t->next = free_tasks;
  free_tasks = t;
}

static void remove_from_depgraph(task_t *task)
{
  // TODO: this is ugly af, come up with something cleaner!
  task_list_t *removed_tl = NULL;
  task_list_t **tl = &dep_graph;
  while (*tl != NULL) {
    if ((*tl)->task == task) {
      removed_tl = *tl;
      (*tl) = (*tl)->next;
      DART_LOG_INFO("Removing task %p from dependency graph", task);
      break;
    }
    tl = &((*tl)->next);
  }
  if (removed_tl != NULL) {
    dart_tasking_tasklist_deallocate_elem(removed_tl);
  }
}


static void
request_direct_task_dependency(void *data) {
  dart_unit_t myid;
  dart_myid(&myid);
  struct remote_task_dep *taskdep = (struct remote_task_dep *)data;

  pthread_mutex_lock(&thread_pool_mutex);

  DART_LOG_INFO("Received direct task dependency request %p -> %p from unit %i", taskdep->task, taskdep->followup, taskdep->runit);
  task_list_t *list = dart_tasking_tasklist_allocate_elem();
  list->task = taskdep->followup;
  list->next = taskdep->task->remote_successor;
  list->unit = taskdep->runit;
  taskdep->task->remote_successor = list;
  pthread_mutex_unlock(&thread_pool_mutex);
}

static void
release_direct_task_dependency(void *data)
{
  struct remote_task_dep *taskdep = (struct remote_task_dep *)data;
  pthread_mutex_lock(&thread_pool_mutex);
  DART_LOG_INFO("Received direct task dependency release for task %p from unit %i", taskdep->task, taskdep->runit);

  taskdep->task->unresolved_deps--;

  if(taskdep->task->unresolved_deps == 0){
    enqeue_runnable(taskdep->task);
  }

  pthread_mutex_unlock(&thread_pool_mutex);
}

static void release_task(task_t *task)
{
  // TODO: we cannot release local dependencies with OUT dependencies matching remote IN dependencies
  //       to avoid overwriting the same data elements. We have to wait for a signal that the remote
  //       task actually finished. Instead of going through data dependencies again, we could directly
  //       define a task-dependency.

  dart_unit_t myid;
  dart_myid(&myid);

  if (rdepq_head == NULL) {
    DART_LOG_INFO("release_task: rdepq_head is NULL");
  }

  // release remote dependency requests
  int i;
  for (i = 0; i < task->ndeps; i++) {
    if (task->deps[i].type == DART_DEP_IN) {
      continue;
    }
    task_t *rt = rdepq_head;
    task_t *rt_prev = rdepq_head;
    while (rt != NULL)
    {
      DART_LOG_INFO("release_task: comparing gptr={u=%i, s=%i, f=%i, o=%p} and gptr={u=%i, s=%i, f=%i, o=%p} (%p and %p)",
        rt->deps[0].gptr.unitid,
        rt->deps[0].gptr.segid,
        rt->deps[0].gptr.flags,
        rt->deps[0].gptr.addr_or_offs.addr,
        task->deps[i].gptr.unitid,
        task->deps[i].gptr.segid,
        task->deps[i].gptr.flags,
        task->deps[i].gptr.addr_or_offs.addr);
      if (task->deps[i].gptr.unitid == rt->deps[0].gptr.unitid
          && task->deps[i].gptr.addr_or_offs.offset == rt->deps[0].gptr.addr_or_offs.offset) {
        if (rt == rdepq_head) {
          rdepq_head = rt->next;
        } else {
          rt_prev->next = rt->next;
        }
        // enqeue_runnable(rt);

        // send the release in-place
        dart_task_action_t *fn = rt->fn;
        void *data = rt->data;

        // send direct task dependency requests for all relevant tasks before sending the actual release
        struct remote_data_dep *datadep = (struct remote_data_dep *)data;

        DART_LOG_INFO("Sending direct dependency requests for successors of task %p starting at successor %p", task, task->successor);

        task_list_t *child = task->successor;
        while (child != NULL)
        {
          task_list_t *next = child->next;
          DART_LOG_INFO("Succesor task %p has %i dependencies", child->task, child->task->ndeps);
          for (int i = 0; i < child->task->ndeps; i++) {
            if (child->task->deps[i].gptr.addr_or_offs.addr == datadep->gptr.addr_or_offs.addr
                || child->task->deps[i].gptr.unitid == datadep->gptr.unitid) {
              int ret;
              // send a direct task dependency request
              // TODO: This code is not executed?!
              struct remote_task_dep taskdep;
              taskdep.task     = datadep->rtask;
              taskdep.followup = child->task;
              child->task->unresolved_deps++;
              taskdep.runit    = myid;
              DART_LOG_INFO("Sending direct task dependency request %p -> %p to unit %i", taskdep.task, taskdep.followup, datadep->runit);
              while ((ret = dart_amsg_trysend(datadep->runit, amsgq, &request_direct_task_dependency, &taskdep, sizeof(taskdep))) != DART_OK) {
                if (ret != DART_ERR_AGAIN) {
                  DART_LOG_ERROR("Failed to send direct task dependency to unit %i", datadep->runit);
                  break;
                }
              }
              break;
            }
          }
          child = next;
        }

        //invoke the task that releases the remote data dependency
        fn(data);
        deallocate_task(rt);
        break;
      }
      rt_prev = rt;
      rt = rt->next;
    }
  }

  // release direct task dependencies
  task_list_t *child = task->remote_successor;
  while (child != NULL)
  {
    task_list_t *next = child->next;
    int ret;
    // send a direct task dependency
    struct remote_task_dep taskdep;
    taskdep.task = child->task;
    taskdep.followup = NULL;
    DART_LOG_INFO("Sending direct task dependency release for task %p to unit %i", taskdep.task, child->unit);
    while ((ret = dart_amsg_trysend(child->unit, amsgq, &release_direct_task_dependency, &taskdep, sizeof(taskdep))) != DART_OK) {
      if (ret != DART_ERR_AGAIN) {
        DART_LOG_ERROR("Failed to send direct task dependency to unit %i", child->unit);
        break;
      }
    }
    child = next;
  }

  // release the task's local dependents
  child = task->successor;
  while (child != NULL)
  {
    task_list_t *next = child->next;
    child->task->unresolved_deps--;
    if (child->task->unresolved_deps == 0) {
      // enqueue in runnable list
      enqeue_runnable(child->task);
      // enqueue the child at the top level of the dependency graph
      child->next = dep_graph;
      dep_graph = child;
      DART_LOG_INFO("Enqueued child task %p into dependency graph top level", child->task);
      // advance
    } else {
      // enqeue this task_list element in the list of free task_list elements
      dart_tasking_tasklist_deallocate_elem(child);
      // advance
    }
    child = next;
  }

  // remove the task itself from the task dependency graph
  remove_from_depgraph(task);
  dart__tasking__ayudame_close_task(task);
}

static void destroy_tsd(void *tsd)
{
  free(tsd);
}

static void wait_for_work()
{
  int ret;
//  DART_LOG_INFO("Thread %i waiting for signal", dart__base__tasking__thread_num());
//  if (dart__base__tasking__thread_num() == (dart__base__tasking__num_threads() - 1)) {
//    // the last thread uses a timed wait to ensure progress on remote requests coming in
//    struct timespec time;
//    clock_gettime(CLOCK_MONOTONIC, &time);
//    struct timespec timeout = {0, 1E6};
//    struct timespec abstimeout = add_timespec(time, timeout);
//    ret = pthread_cond_timedwait(&task_avail_cond, &thread_pool_mutex, &abstimeout);
//    if (ret != ETIMEDOUT) {
//      DART_LOG_INFO("Thread %i received signal", dart__base__tasking__thread_num());
//    }
//  } else
  {
    pthread_cond_wait(&task_avail_cond, &thread_pool_mutex);
    DART_LOG_INFO("Thread %i received signal", dart__base__tasking__thread_num());
  }
}

static void set_current_task(task_t *t)
{
  ((tpd_t*)pthread_getspecific(tpd_key))->current_task = t;
}
static task_t * get_current_task()
{
  return ((tpd_t*)pthread_getspecific(tpd_key))->current_task;
}

/**
 * Remote tasking data structures and functionality
 */

static void release_remote_dependency(void *data)
{
  struct remote_data_dep *response = (struct remote_data_dep *)data;
  // remote dependent tasks are all enqeued at the top level of the dependency graph
  pthread_mutex_lock(&thread_pool_mutex);

  DART_LOG_INFO("Received remote dependency release from unit %i for task %p (segid=%i, offset=%p)",
    response->gptr.unitid, response->rtask, response->gptr.segid, response->gptr.addr_or_offs.offset);
  if (response->rtask->unresolved_deps > 0)
  {
    int i;
    for (i = 0; i < response->rtask->ndeps; i++) {
      if (DART_GPTR_EQUAL(response->rtask->deps[i].gptr, response->gptr)) {
        response->rtask->deps[i].type = DART_DEP_RES;
        break;
      }
    }
    response->rtask->unresolved_deps--;
  } else {
    DART_LOG_ERROR("ERROR: task with remote dependency does not seem to have unresolved dependencies!");
  }

  if (response->rtask->unresolved_deps == 0) {
    // release this task
    enqeue_runnable(response->rtask);
  }
//  task_list_t *tl = dep_graph;
//  while(tl != NULL) {
//    size_t i;
//    for (i = 0; i < tl->task->ndeps; i++) {
//      if (tl->task->deps[i].type == DART_DEP_IN && DART_GPTR_EQUAL(tl->task->deps[i].gptr, rtask->gptr)) {
//        // release this dependency
//        if (tl->task->unresolved_deps > 0) {
//          tl->task->unresolved_deps--;
//        } else {
//          DART_LOG_ERROR("ERROR: task with remote dependency does not seem to have unresolved dependencies!");
//        }
//        if (tl->task->unresolved_deps == 0) {
//          // release this task
//          enqeue_runnable(tl->task);
//        }
//      }
//    }
//    tl = tl->next;
//  }
  pthread_mutex_unlock(&thread_pool_mutex);
}

static void send_release(void *data)
{
  struct remote_data_dep *rtask = (struct remote_data_dep *)data;

  while (1) {
    dart_ret_t ret;
    ret = dart_amsg_trysend(rtask->runit, amsgq, &release_remote_dependency, rtask, sizeof(*rtask));
    if (ret == DART_OK) {
      // the message was successfully sent
      DART_LOG_INFO("Sent remote dependency release to unit %i (segid=%i, offset=%p, fn=%p)",
          rtask->runit, rtask->gptr.segid, rtask->gptr.addr_or_offs.offset, &enqueue_from_remote);
      break;
    } else  if (ret == DART_ERR_AGAIN) {
      // cannot be sent at the moment, just try again
      // TODO: anything more sensible to do here?
      continue;
    } else {
      DART_LOG_ERROR("Failed to send active message to unit %i", rtask->gptr.unitid);
      break;
    }
  }

  free(data);
}

static void
enqueue_from_remote(void *data)
{
  struct remote_data_dep *rtask = (struct remote_data_dep *)data;
  struct remote_data_dep *response = malloc(sizeof(*response));
  response->gptr = rtask->gptr;
  response->runit = rtask->runit;
  response->rtask = rtask->rtask;
  dart_task_dep_t dep;
  dep.gptr = rtask->gptr;
  dep.type = DART_DEP_IN;
  // we cannot guarantee the order of incoming dependencies so we have to keep them separate for now
  pthread_mutex_lock(&thread_pool_mutex);
  task_t *t = allocate_task();
  t->fn = &send_release;
  t->data = response;
  t->data_size = 0;
  t->ndeps = 1;
  t->deps = malloc(sizeof(dart_task_dep_t));
  memcpy(t->deps, &dep, sizeof(dart_task_dep_t));
  t->next = rdepq_head;
  rdepq_head = t;
  pthread_mutex_unlock(&thread_pool_mutex);

  DART_LOG_INFO("Created task %p for remote dependency request from unit %i (unit=%i, segid=%i, addr=%p)",
                      t, response->runit, rtask->gptr.unitid, rtask->gptr.segid, rtask->gptr.addr_or_offs.addr);
}

static void remote_dependency_request(task_t *task, dart_gptr_t gptr)
{
  dart_ret_t ret;
  struct remote_data_dep rtask;

  // gptr has already been converted to offset representation
  rtask.gptr = gptr;

//   We have to compute the absolute address at the target since segment IDs are not
//   guaranteed to be the same on all units.
//  rtask.gptr.unitid = gptr.unitid;
//  rtask.gptr.flags = 0;
//  rtask.gptr.segid = 0;
//  void *addr;
//  dart_gptr_getaddr(gptr, &addr);
//  rtask.gptr.addr_or_offs.addr = addr;
//  DART_LOG_INFO("remote_dependency_request: converted remote dependency gptr={u=%i, s=%i, f=%i, o=%llu} to local address %p",
//    gptr.unitid, gptr.segid, gptr.flags, gptr.addr_or_offs.offset, rtask.gptr.addr_or_offs.addr);

  rtask.rtask = task;
  dart_myid(&rtask.runit);

  while (1) {
    ret = dart_amsg_trysend(gptr.unitid, amsgq, &enqueue_from_remote, &rtask, sizeof(rtask));
    if (ret == DART_OK) {
      // the message was successfully sent
      DART_LOG_INFO("Sent remote dependency request (unit=%i, segid=%i, offset=%p, fn=%p)", gptr.unitid, gptr.segid, gptr.addr_or_offs.offset, &enqueue_from_remote);
      break;
    } else  if (ret == DART_ERR_AGAIN) {
      // cannot be send at the moment, just try again
      // TODO: anything more sensible to do here?
      continue;
    } else {
      DART_LOG_ERROR("Failed to send active message to unit %i", gptr.unitid);
      break;
    }
  }
}

dart_ret_t
dart__base__tasking_sync_taskgraph()
{
  dart_amsg_sync(amsgq);

  return DART_OK;
}

/*
 * Functions for printing the dependency graph
 */
static void print_subgraph(task_list_t *tl, int level)
{
  task_list_t *elem = tl;
  while (elem != NULL)
  {
    int i;
//    Dl_info info;
    // print a preamble
    int indent = level *2;
    for (i = 0; i < indent - 2; i++) {
      printf(" ");
    }
    if (level > 0) {
      printf("|-");
    }

//    dladdr(elem->task->fn, &info);

    // print the task
    printf("task@%p{fn=%p, parent@%p, ndeps=%i, unresolved_deps=%i",
        elem->task, elem->task->fn, elem->task->parent, elem->task->ndeps, elem->task->unresolved_deps);
    if (elem->task->ndeps > 0) {
      printf(", deps={");
      for (i = 0; i < elem->task->ndeps; i++) {
        printf("  {type=%i, gptr={unitid=%i, segid=%i, offset=%p}}", elem->task->deps[i].type,
            elem->task->deps[i].gptr.unitid, elem->task->deps[i].gptr.segid, elem->task->deps[i].gptr.addr_or_offs.offset);
      }
      printf("  }");
    }
    printf(", dependants={");
    task_list_t *dep = elem->task->successor;
    while (dep != NULL) {
      printf("  {t@%p}", dep->task);
      dep = dep->next;
    }
    printf("}");
    printf("}\n");

    // print its children
    if (elem->task->successor) {
      print_subgraph(elem->task->successor, level + 1);
    }

    elem = elem->next;
  }
}

void dart__base__tasking_print_taskgraph()
{
  pthread_mutex_lock(&thread_pool_mutex);

  sync();
  print_subgraph(dep_graph, 0);
  sync();

  pthread_mutex_unlock(&thread_pool_mutex);
}

//#endif // defined(DART_TASKING_PTHREADS)
#endif
