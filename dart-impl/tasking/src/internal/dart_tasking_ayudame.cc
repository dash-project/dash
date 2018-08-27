
#include <map>

#ifdef DART_ENABLE_AYUDAME2

#include <ayudame.h>
#include <pthread.h>
#include <dash/dart/if/dart_tasking.h>
#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_globmem.h>

#include <dash/dart/tasking/dart_tasking_ayudame.h>

static ayu_client_id_t client_id;

typedef ayu_id_t taskid_t;
typedef ayu_id_t depid_t;

static taskid_t task_id = 0;
static depid_t dependency_id = 0;

static std::map<void*, taskid_t>      task_map;

void dart__tasking__ayudame_init()
{
  if (ayu_event) {
    client_id = get_client_id(AYU_CLIENT_MPI);
  }
}

void dart__tasking__ayudame_fini()
{
  if (ayu_event) {
    ayu_event_data_t data;
    ayu_wipe_data(&data);
    data.common.client_id = client_id;

    ayu_event(AYU_FINISH, data);
  }
}

/**
 * Pass information on a started task
 */
void dart__tasking__ayudame_create_task(void *task, void *parent)
{
  if (ayu_event) {
    char buf[128];
    ayu_event_data_t data;
    ayu_wipe_data(&data);
    data.common.client_id = client_id;
    taskid_t tid;

    if (task_map.find(task) == task_map.end()) {
      tid = task_id;
      task_id++;
      task_map.insert(std::make_pair(task, tid));
    } else {
      tid = task_map[task];
    }

    if (task_map.find(parent) != task_map.end()) {
      data.add_task.scope_id = task_map[parent];
    }

    snprintf(buf, sizeof(buf), "task_%llu", tid);

    data.add_task.task_id = tid;
    data.add_task.task_label = buf;

    ayu_event(AYU_ADDTASK, data);
  }
}

void dart__tasking__ayudame_close_task(void *task)
{
  if (ayu_event) {

    if (task_map.find(task) != task_map.end()) {
      task_map.erase(task);
    }

  }
}

void dart__tasking__ayudame_add_dependency(void *srctask, void *dsttask)
{
  if (ayu_event) {
    char buf[128];
    taskid_t srcid = task_map[srctask];
    taskid_t dstid = task_map[dsttask];


    ayu_event_data_t data;
    ayu_wipe_data(&data);
    data.common.client_id = client_id;
    data.add_dependency.dependency_id = dependency_id++;
    data.add_dependency.from_id = srcid;
    data.add_dependency.to_id = dstid;
    snprintf(buf, sizeof(buf), "dep_%i", data.add_dependency.dependency_id);
    data.add_dependency.dependency_label = buf;

    ayu_event(AYU_ADDDEPENDENCY, data);
  }
}

#endif // DART_ENABLE_AYUDAME2
