#ifdef DART_TASKING_LLVM

#include <kmp.h>

#include <dash/dart/base/logging.h>
#include <dash/dart/if/dart_tasking.h>
#include <dash/dart/if/dart_active_messages.h>

struct taskwrap_data {

  void (*fn)(void*);
  void  *data;
  size_t data_size;

};

dart_ret_t
dart__base__tasking__init()
{
  // nothing to do
  // TODO: where (and how) to call the __kmpc_fork_call?
  return DART_OK;
}

dart_ret_t
dart__base__tasking__fini()
{
  // nothing to do
  return DART_OK;
}

int
dart__base__tasking__thread_num()
{
  return __kmpc_global_thread_num(NULL);
}

int
dart__base__tasking__num_threads()
{
  return __kmpc_global_num_threads(NULL);
}

static kmp_int32 task_routine_entry(kmp_int32 gtid, void *data)
{
  kmp_task_t *task = (kmp_task_t *)data;
  struct taskwrap_data *twd = (struct taskwrap_data *)task->shareds;

  // invoke the action
  twd->fn(twd->data);

  // cleanup
  if (twd->data_size) {
    free(twd->data);
    twd->data = 0;
    twd->data_size = 0;
  }
  free(twd);

  return 0;
}

dart_ret_t
dart__base__tasking__create_task(void (*fn) (void *), void *data_arg, size_t data_size, dart_task_dep_t *deps, size_t ndeps)
{
  dart_unit_t myid;
  dart_myid(&myid);

  for (int i = 0; i < ndeps; i++) {
    if (deps[i]->gptr.unitid != myid) {
      DART_LOG_ERROR("Remote dependencies not support with the LLVM runtime and will be ignored!");
      // TODO: handle remote dependencies
    }
  }

  int gtid = dart__base__tasking__thread_num();
  kmp_task_t *task = __kmpc_omp_task_alloc(NULL, gtid, 0, sizeof(kmp_task_t), 0, &task_routine_entry);

  struct taskwrap_data *twd = malloc(sizeof(struct taskwrap_data));

  if (data_size) {
    twd->data = malloc(data_size);
    memcpy(twd->data, data_arg, data_size);
  } else {
    twd->data = data_arg;
    twd->data_size = 0;
  }

  task->shareds = twd;

  if (ndeps == 0) {
    __kmpc_omp_task(NULL, gtid, task);
  } else {
    int num_local = 0;
    kmp_depend_info_t dep_list[ndeps];
    for (int i = 0; i < ndeps; i++) {
      if (deps[i]->gptr.unitid != myid) {
        continue;
      }
      dep_list[num_local]->base_addr = deps[i]->gptr.addr_or_offs.addr;
      dep_list[num_local]->len = 1;
      dep_list[num_local]->flags.in  = (deps[i]->type == DART_DEP_IN  || deps[i]->type == DART_DEP_INOUT) ? 1 : 0;
      dep_list[num_local]->flags.out = (deps[i]->type == DART_DEP_OUT || deps[i]->type == DART_DEP_INOUT) ? 1 : 0;
      num_local++;
    }
    __kmpc_omp_task_with_deps( NULL, gtid, task, num_local, dep_list, 0, NULL );
  }

  return DART_OK;
}

/**
 * This function is only called by the master thread and
 * starts the actual processing of tasks.
 */
dart_ret_t
dart__base__tasking__task_complete()
{
  __kmpc_omp_taskwait(NULL, dart__base__tasking__thread_num());
  return DART_OK;
}

#endif // DART_TASKING_LLVM
