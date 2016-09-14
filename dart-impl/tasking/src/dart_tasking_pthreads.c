#include <dash/dart/if/dart_tasking.h>
#include <dash/dart/if/dart_active_messages.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/base/hwinfo.h>
#include <dash/dart/tasking/dart_tasking_priv.h>

#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h>


typedef void (tfunc_t) (void *);
typedef struct task_list task_list_t;

static pthread_cond_t task_avail_cond = PTHREAD_COND_INITIALIZER;
static pthread_cond_t tasks_done_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t thread_pool_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t tasks_done_mutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_key_t thr_id_key;

static dart_amsgq_t amsgq;

pthread_t *thread_pool;

typedef struct task_data {
  struct task_data *next; // next entry in the runnable task list
  tfunc_t *fn;
  void *data;
  size_t unresolved_deps;
  dart_task_dep_t *deps;
  size_t ndeps;
  task_list_t *children;
} task_t;

struct task_list {
  task_t *task;
  struct task_list *next; // next entry on the same level of the task graph
};

struct remote_task {
  dart_gptr_t gptr;
  dart_unit_t runit;
};

#define DART_RTASK_QLEN 256

// TODO: access the list through head and tail
static task_t *runq_head = NULL;
static task_t *runq_tail = NULL;

// free-list for tasks
static task_t *free_tasks = NULL;

// list of tasks that are dependent
static task_list_t *dep_graph = NULL;

// free-list for task_list elements
static task_list_t *free_task_list = NULL;

// true if threads should process tasks. Set to false to quit parallel processing
static bool parallel;

static int num_threads;
static int threads_running = 0;

static inline task_t * deqeue_runnable(void);
static inline void enqeue_runnable(task_t *task);

static inline task_list_t * allocate_task_list_elem();
static inline void deallocate_task_list_elem(task_list_t *tl);

static inline task_t * allocate_task();

static inline void deallocate_task(task_t *t);

static void release_task(task_t *task);

static void destroy_tsd(void *tsd);

static void remote_dependency_request(dart_gptr_t gptr);

/**
 * Take a task from the task queue and execute it.
 * NOTE: threads have to hold the thread_pool_mutex before entering here.
 * TODO: Implement more fine-grained locking.
 */
static void handle_task()
{
  // remove from runnable queue and execute
  task_t *task = deqeue_runnable();
  if (task != NULL)
  {
    DART_LOG_INFO("Thread %i executing task %p", dart__base__tasking__thread_num(), task);
    pthread_mutex_unlock(&thread_pool_mutex);

    rfunc_t *fn = task->fn;
    void *data = task->data;

    //invoke the task function
    fn(data);

    pthread_mutex_lock(&thread_pool_mutex);
    release_task(task);
    free(task->deps);
    deallocate_task(task);
  }
}

/**
 * Adds the time specified in b to a, i.e., a += b;
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
  int *thread_id = (int*)data;
  pthread_setspecific(thr_id_key, thread_id);

  // this is only done once upon thread creation
  pthread_mutex_lock(&thread_pool_mutex);
  threads_running++;
  pthread_mutex_unlock(&thread_pool_mutex);

  // enter work loop
  while (parallel) {
    // look for remote tasks coming in
    dart_amsg_process(amsgq);
    pthread_mutex_lock(&thread_pool_mutex);
    if (runq_head == NULL) {
      threads_running--;
      if (threads_running == 0) {
        // signal that all threads are done
        pthread_cond_signal(&tasks_done_cond);
      }
      DART_LOG_INFO("Thread %i waiting for signal", dart__base__tasking__thread_num());
      if (dart__base__tasking__thread_num() == (dart__base__tasking__num_threads() - 1)) {
        // the last thread uses a timed wait to ensure progress on remote requests coming in
        int ret;
        struct timespec time;
        clock_gettime(CLOCK_MONOTONIC, &time);
        struct timespec timeout = {0, 1E6};
        struct timespec abstimeout = add_timespec(time, timeout);
        ret = pthread_cond_timedwait(&task_avail_cond, &thread_pool_mutex, &abstimeout);
        if (ret == ETIMEDOUT) {
          DART_LOG_INFO("Thread %i woken up after %f ms", dart__base__tasking__thread_num(),
              ((double)timeout.tv_sec * 1E3) + ((double)timeout.tv_nsec / 1E6));
        } else {
          DART_LOG_INFO("Thread %i received signal", dart__base__tasking__thread_num());
        }
      } else {
        pthread_cond_wait(&task_avail_cond, &thread_pool_mutex);
        DART_LOG_INFO("Thread %i received signal", dart__base__tasking__thread_num());
      }
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

  // set up the active message queue
  amsgq = dart_amsg_openq(sizeof(struct remote_task) * DART_RTASK_QLEN, DART_TEAM_ALL);
  DART_LOG_INFO("Created active message queue (%p)", amsgq);

  pthread_key_create(&thr_id_key, &destroy_tsd);
  // set master thread id
  int *thread_id = malloc(sizeof(int));
  *thread_id = 0;
  pthread_setspecific(thr_id_key, thread_id);

  thread_pool = malloc(sizeof(pthread_t) * num_threads);
  for (i = 0; i < num_threads-1; i++)
  {
    thread_id = malloc(sizeof(int));
    *thread_id = i + 1; // 0 is reserved for master thread
    int ret = pthread_create(&thread_pool[i], NULL, &thread_main, thread_id);
    if (ret != 0) {
      DART_LOG_ERROR("Failed to create thread %i of %i!", (i+1), num_threads);
    }
  }

  return DART_OK;
}

int
dart__base__tasking__thread_num()
{
  return *(int*)(pthread_getspecific(thr_id_key));
}

int
dart__base__tasking__num_threads()
{
  return num_threads;
}

dart_ret_t
dart__base__tasking__fini()
{
  int i;

  parallel = false;
  pthread_cond_broadcast(&task_avail_cond);

  for (i = 0; i < num_threads; i++) {
    pthread_join(thread_pool[i], (void**)NULL);
  }

  while (free_tasks != NULL) {
    task_t *next = free_tasks->next;
    free_tasks->next = NULL;
    free(free_tasks);
    free_tasks = next;
  }

  while (free_task_list != NULL) {
    task_list_t *next = free_task_list->next;
    free_task_list->next = NULL;
    free(free_task_list);
    free_task_list = next;
  }

  dart_amsg_closeq(amsgq);

  return DART_OK;
}

static task_t * find_parent_in_list(task_list_t *parent, const dart_task_dep_t *dep);
static inline task_t * find_parent(task_t *parent, const dart_task_dep_t *dep)
{
  size_t i;
  task_t *t = NULL;

  if (parent != NULL)
  {
    // does this task satisfy the dependency?
    for (i = 0; i < parent->ndeps; i++) {
      if ((parent->deps[i].type == DART_DEP_OUT || parent->deps[i].type == DART_DEP_INOUT)
          && DART_GPTR_EQUAL(parent->deps[i].gptr, dep->gptr)) {
        // this parent will resolve the dependency
        t = parent;
      } else {
        DART_LOG_INFO("Ignoring task with DEP (dart_gptr = {addr=%p, seg=%i, unit=%i}})",
            parent->deps[i].gptr.addr_or_offs.addr, parent->deps[i].gptr.segid, parent->deps[i].gptr.unitid);
      }
    }

    if (t == NULL) {
      t = find_parent_in_list(parent->children, dep);
    }
  }
  return t;
}

static task_t * find_parent_in_list(task_list_t *list, const dart_task_dep_t *dep)
{
  task_t *task = NULL;
  if (list != NULL) {
    // look at the task and it's children first
    task = find_parent(list->task, dep);

    if (task == NULL) {
      task = find_parent_in_list(list->next, dep);
    }
  }
  return task;
}

dart_ret_t
dart__base__tasking__create_task(void (*fn) (void *), void *data, dart_task_dep_t *deps, size_t ndeps)
{
  dart_unit_t myid;
  size_t i;
  bool in_dep = false;
  pthread_mutex_lock(&thread_pool_mutex);

  dart_myid(&myid);

  task_t *task = allocate_task();

  task->data = data;
  task->fn = fn;
  task->ndeps = ndeps;
  task->deps = malloc(sizeof(dart_task_dep_t) * ndeps);
  memcpy(task->deps, deps, sizeof(dart_task_dep_t) * ndeps);

  if (ndeps > 0) {
    for (i = 0; i < ndeps; i++) {
      if (deps[i].type == DART_DEP_IN || deps[i].type == DART_DEP_INOUT) {
        // put the task into the dependency graph

        if (deps[i].gptr.unitid != myid) {
          // create a dependency request at the remote unit
          remote_dependency_request(deps[i].gptr);
          task->unresolved_deps++;
          // enqueue this task at the top level of the task graph
          task_list_t *tl = allocate_task_list_elem();
          tl->task = task;
          tl->next = dep_graph;
          dep_graph = tl;

          in_dep = true;

        } else {

          task_t *parent = find_parent_in_list(dep_graph, &deps[i]);
          if (parent != NULL) {
            // enqueue as child
            task_list_t *tl = allocate_task_list_elem();
            tl->task = task;
            tl->next = parent->children;
            parent->children = tl;

            in_dep = true;
            task->unresolved_deps++;
            DART_LOG_INFO("Task %p depends on task %p", task, parent);
          } else {
            // Do nothing, consider the parent as solved already
            DART_LOG_INFO("Could not find a parent for task with IN dep (dart_gptr = {addr=%p, seg=%i, unit=%i}})",
                deps[i].gptr.addr_or_offs.addr, deps[i].gptr.segid, deps[i].gptr.unitid);
          }
        }
      } else if (deps[i].type == DART_DEP_OUT) {
        DART_LOG_INFO("Found task %p with OUT dep (dart_gptr = {addr=%p, seg=%i, unit=%i}})", task,
            deps[i].gptr.addr_or_offs.addr, deps[i].gptr.segid, deps[i].gptr.unitid);
      }
    }
    if (!in_dep) {
      // no parent task required
      // -> enqueue on top level of dependency graph
      task_list_t *tl = allocate_task_list_elem();
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

dart_ret_t
dart__base__tasking__task_complete()
{
  while (dep_graph != NULL || runq_head != NULL ){
    // look for remote tasks coming in
    dart_amsg_process(amsgq);
    // participate in the task completion
    pthread_mutex_lock(&thread_pool_mutex);
    handle_task();
    if (dep_graph != NULL || runq_head != NULL) {
      pthread_mutex_unlock(&thread_pool_mutex);
      continue;
    }
    if (threads_running > 0) {
      // threads are still running, wait for them to signal completion
      pthread_mutex_unlock(&thread_pool_mutex);

      pthread_mutex_lock(&tasks_done_mutex);
      DART_LOG_INFO("Master: waiting for all tasks to finish");
      pthread_cond_wait(&tasks_done_cond, &tasks_done_mutex);
      DART_LOG_INFO("Master: all tasks finished");
      pthread_mutex_unlock(&tasks_done_mutex);
    } else {
      DART_LOG_INFO("Master: no active tasks running");
      pthread_mutex_unlock(&thread_pool_mutex);
      break;
    }
  }
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
  }
  if (runq_head == NULL) {
    runq_tail = NULL;
  }

  DART_LOG_INFO("Dequeued task %p from runnable list (fn=%p, data=%p)", task, task->fn, task->data);
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

static inline task_list_t * allocate_task_list_elem(){
  task_list_t *tl = NULL;
  if (free_task_list != NULL) {
    tl = free_task_list;
    free_task_list = free_task_list->next;
  } else {
    tl = calloc(1, sizeof(task_list_t));
  }
  return tl;
}

static inline void deallocate_task_list_elem(task_list_t *tl) {
  tl->task = NULL;
  tl->next = free_task_list;
  free_task_list = tl;
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
    deallocate_task_list_elem(removed_tl);
  }
}

static void release_task(task_t *task)
{
  // first release the task's dependent children
  task_list_t *child = task->children;
  while (child != NULL)
  {
    child->task->unresolved_deps--;
    if (child->task->unresolved_deps == 0) {
      // enqueue in runnable list
      enqeue_runnable(child->task);
      // enqueue the child at the top level of the dependency graph
      task_list_t *next = child->next;
      child->next = dep_graph;
      dep_graph = child;
      DART_LOG_INFO("Enqueued child task %p into dependency graph top level", child->task);
      // advance
      child = next;
    } else {
      // enqeue this task_list element in the list of free task_list elements
      task_list_t *next = child->next;
      deallocate_task_list_elem(child);
      // advance
      child = next;
    }
  }

  // remove the task itself from the task dependency graph
  remove_from_depgraph(task);
}

static void destroy_tsd(void *tsd)
{
  free(tsd);
}

/**
 * Remote tasking data structures and functionality
 */

static void release_remote(void *data)
{
  dart_gptr_t gptr = *(dart_gptr_t*)data;
  // remote dependent tasks are all enqeued at the top level of the dependency graph
  task_list_t *tl = dep_graph;
  while(tl != NULL) {
    size_t i;
    for (i = 0; i < tl->task->ndeps; i++) {
      if (tl->task->deps[i].type == DART_DEP_IN && DART_GPTR_EQUAL(tl->task->deps[i].gptr, gptr)) {
        // release this dependency
        if (tl->task->unresolved_deps > 0) {
          tl->task->unresolved_deps--;
        } else {
          DART_LOG_ERROR("ERROR: task with remote dependency does not seem to have unresolved dependencies!");
        }
        if (tl->task->unresolved_deps == 0) {
          // release this task
          enqeue_runnable(tl->task);
        }
      }
    }
    tl = tl->next;
  }
}

static void send_release(void *data)
{
  dart_ret_t ret;
  dart_amsg_t msg;
  struct remote_task *rtask = (struct remote_task *)data;

  msg.fn = &release_remote;
  msg.data = &(rtask->gptr);
  msg.data_size = sizeof(rtask->gptr);

  while (1) {
    ret = dart_amsg_trysend(rtask->runit, amsgq, &msg);
    if (ret == DART_OK) {
      // the message was successfully sent
      break;
    } else  if (ret == DART_ERR_AGAIN) {
      // cannot be send at the moment, just try again
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
  struct remote_task *rtask = (struct remote_task *)data;
  struct remote_task *response = malloc(sizeof(*response));
  response->gptr = rtask->gptr;
  response->runit = rtask->runit;
  dart_task_dep_t dep;
  dep.gptr = rtask->gptr;
  dep.type = DART_DEP_IN;
  DART_LOG_INFO("Creating task for remote dependency request (unit=%i, segid=%i, offset=%i)",
                      rtask->gptr.unitid, rtask->gptr.segid, rtask->gptr.addr_or_offs.offset);
  dart__base__tasking__create_task(&send_release, response, &dep, 1);
}

static void remote_dependency_request(dart_gptr_t gptr)
{
  dart_ret_t ret;
  dart_amsg_t msg;
  struct remote_task rtask;
  rtask.gptr = gptr;
  dart_myid(&rtask.runit);

  msg.fn = &enqueue_from_remote;
  msg.data = &rtask;
  msg.data_size = sizeof(rtask);

  while (1) {
    ret = dart_amsg_trysend(gptr.unitid, amsgq, &msg);
    if (ret == DART_OK) {
      // the message was successfully sent
      DART_LOG_INFO("Sent remote dependency request (unit=%i, segid=%i, offset=%i)", gptr.unitid, gptr.segid, gptr.addr_or_offs.offset);
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
