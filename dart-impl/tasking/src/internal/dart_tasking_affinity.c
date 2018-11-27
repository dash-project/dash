#define _GNU_SOURCE

#include <dash/dart/base/logging.h>

#include <dash/dart/tasking/dart_tasking_affinity.h>

#include <dash/dart/base/env.h>
#include <dash/dart/tasking/dart_tasking_envstr.h>


#include <stdio.h>
#include <stdlib.h>

static bool             print_binding = false;

#ifdef DART_ENABLE_HWLOC
#include <hwloc.h>


static hwloc_topology_t topology;
static hwloc_cpuset_t   ccpuset;
static hwloc_cpuset_t   cpuset_used; // << set of used CPUS

void
dart__tasking__affinity_init()
{
  hwloc_topology_init(&topology);
  hwloc_topology_load(topology);
  ccpuset = hwloc_bitmap_alloc();
  hwloc_get_cpubind(topology, ccpuset, HWLOC_CPUBIND_PROCESS);
  cpuset_used = hwloc_bitmap_dup(ccpuset);

#ifdef DART_ENABLE_LOGGING
  // force printing of binding if logging is enabled
  print_binding = true;
#else
  print_binding = dart__base__env__bool(
                    DART_THREAD_AFFINITY_VERBOSE_ENVSTR, false);
#endif // DART_ENABLE_LOGGING


  if (print_binding) {
    DART_LOG_INFO_ALWAYS(
      "Using hwloc to set affinity (print: %d)", print_binding);
    int num_cpus = hwloc_bitmap_weight(ccpuset);
    size_t len = num_cpus * 8;
    char* buf = malloc(sizeof(char) * len);
    unsigned int entry = hwloc_bitmap_first(ccpuset);
    size_t pos = snprintf(buf, len, "%d", entry);
    for (entry  = hwloc_bitmap_next(ccpuset, entry);
         entry <= hwloc_bitmap_last(ccpuset);
         entry  = hwloc_bitmap_next(ccpuset, entry))
    {
      pos += snprintf(buf+pos, len - pos, ", %d", entry);
      if (pos >= len) break;
    }
    DART_LOG_INFO_ALWAYS("Allocated CPU set (size %d): {%s}", num_cpus, buf);
    free(buf);
  }
}

void
dart__tasking__affinity_fini()
{
  hwloc_topology_destroy(topology);
  hwloc_bitmap_free(ccpuset);
}

int
dart__tasking__affinity_set(pthread_t pthread, int dart_thread_id)
{
  //hwloc_const_cpuset_t ccpuset = hwloc_topology_get_allowed_cpuset(topology);
  hwloc_cpuset_t cpuset = hwloc_bitmap_alloc();
  int cnt = -1;
  // iterate over bitmap, round-robin thread-binding
  int entry;
  do {
    for (entry  = hwloc_bitmap_first(ccpuset);
         entry != -1;
         entry  = hwloc_bitmap_next(ccpuset, entry))
    {
      ++cnt;
      if (cnt == dart_thread_id) break;
    }
  } while (cnt != dart_thread_id);

  hwloc_bitmap_set(cpuset, entry);

  if (print_binding)
  {
    char *bitstring;
    hwloc_bitmap_asprintf(&bitstring, cpuset);
    DART_LOG_INFO_ALWAYS("Binding thread %d to CPU %d [%s]",
                         dart_thread_id, entry, bitstring);
    free(bitstring);
  }

  if (0 != hwloc_set_thread_cpubind(topology, pthread,
                                    cpuset, HWLOC_CPUBIND_STRICT)) {
    // try again with no flags
    hwloc_set_thread_cpubind(topology, pthread, cpuset, 0);
  }

  hwloc_bitmap_free(cpuset);

  return entry;
}

void
dart__tasking__affinity_set_utility(pthread_t pthread, int dart_thread_id)
{
  dart__unused(dart_thread_id);
  hwloc_cpuset_t cpuset = hwloc_bitmap_alloc();
  int nnumanodes = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_NUMANODE);
  DART_LOG_INFO_ALWAYS("Found %d numa nodes", nnumanodes);
  int entry = -1;
  if (nnumanodes > 0) {
    for (int i = 0; i < nnumanodes;++i) {
      hwloc_obj_t obj;
      obj = hwloc_get_obj_by_type(topology, HWLOC_OBJ_NUMANODE, i);
      if (!hwloc_bitmap_iszero(obj->allowed_cpuset)) {
        hwloc_bitmap_and(cpuset, obj->allowed_cpuset, ccpuset);
        int last_cpu = hwloc_bitmap_last(cpuset);
        if (last_cpu != -1) {
          // bind the thread to the last thread on that socket
          entry = last_cpu;
          break;
        } else {
          DART_LOG_INFO_ALWAYS("Numa node %d has no allowed CPUs", i);
        }
      }
    }
  } else if (nnumanodes == 0) {
    entry = hwloc_bitmap_last(ccpuset);
  } else {
    DART_LOG_ERROR("Call to hwloc_get_nbobjs_by_type failed!");
  }

  if (entry != -1) {
    hwloc_bitmap_only(cpuset, entry);
    hwloc_set_thread_cpubind(topology, pthread, cpuset, 0);
    if (print_binding)
    {
      DART_LOG_INFO_ALWAYS("Binding utility thread to CPU %d",
                            entry);
    }
  }
  hwloc_bitmap_free(cpuset);
}


#else // DART_ENABLE_HWLOC

/*
void
dart__tasking__affinity_init()
{

}
*/

static cpu_set_t set;

#include <sched.h>
#include <pthread.h>

void
dart__tasking__affinity_init()
{
  CPU_ZERO(&set);
  sched_getaffinity(getpid(), sizeof(set), &set);

#ifdef DART_ENABLE_LOGGING
  // force printing of binding if logging is enabled
  print_binding = true;
#else
  print_binding = dart__base__env__bool(
                    DART_THREAD_AFFINITY_VERBOSE_ENVSTR, false);
#endif // DART_ENABLE_LOGGING


  if (print_binding) {
    DART_LOG_INFO_ALWAYS(
      "Using pthread_setaffinity_np to set affinity (print: %d)", print_binding);
    int count = 0;
    int num_cpus = CPU_COUNT(&set);
    size_t len = num_cpus * 8;
    char* buf = malloc(sizeof(char) * len);
    int pos = 0;
    unsigned int entry = 0;
    while (count < num_cpus)
    {
      while (!CPU_ISSET(entry, &set)) ++entry;
      pos += snprintf(buf+pos, len - pos, "%d, ", entry);
      if (pos >= len) break;
      ++entry;
      ++count;
    }
    if (pos > 2) buf[pos-2] = '\0';
    DART_LOG_INFO_ALWAYS("Allocated CPU set (size %d): {%s}", num_cpus, buf);
    free(buf);
  }

}


void
dart__tasking__affinity_set(pthread_t pthread, int dart_thread_id)
{
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  int num_cpus = CPU_COUNT(&set);
  unsigned int entry = 0;
  int count = 0;

  do {
    if (CPU_ISSET(entry, &set))
    {
      if (count == dart_thread_id) break;
      count++;
    }
    entry = (entry+1) % CPU_SETSIZE;
  } while (1);

  if (print_binding)
  {
    DART_LOG_INFO_ALWAYS("Binding thread %d to CPU %d", dart_thread_id, entry);
  }

  CPU_SET(entry, &cpuset);

  pthread_setaffinity_np(pthread, sizeof(cpu_set_t), &cpuset);
}

void
dart__tasking__affinity_set_utility(pthread_t pthread, int dart_thread_id)
{
  dart__unused(dart_thread_id);
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  int num_cpus = CPU_COUNT(&set);
  unsigned int entry = 0;
  int count = 0;

  // bind the thread to the last CPU
  for (int i = 0; i < num_cpus; ++i) {
    if (CPU_ISSET(i, &set)) {
      entry = i;
    }
  }

  if (print_binding)
  {
    DART_LOG_INFO_ALWAYS("Binding utility thread to CPU %d", entry);
  }

  CPU_SET(entry, &cpuset);

  pthread_setaffinity_np(pthread, sizeof(cpu_set_t), &cpuset);
}

void
dart__tasking__affinity_fini()
{
 // nothing to do
}

#endif // DART_ENABLE_HWLOC
