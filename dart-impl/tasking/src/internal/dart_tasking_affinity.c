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

void
dart__tasking__affinity_init()
{
  hwloc_topology_init(&topology);
  hwloc_topology_load(topology);
  ccpuset = hwloc_bitmap_alloc();
  hwloc_get_cpubind(topology, ccpuset, HWLOC_CPUBIND_PROCESS);

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

void
dart__tasking__affinity_set(pthread_t pthread, int dart_thread_id)
{
  //hwloc_const_cpuset_t ccpuset = hwloc_topology_get_allowed_cpuset(topology);
  hwloc_cpuset_t cpuset = hwloc_bitmap_alloc();
  int cnt = 0;
  // iterate over bitmap, round-robin thread-binding
  int entry;
  do {
    for (entry  = hwloc_bitmap_first(ccpuset);
         entry != hwloc_bitmap_last(ccpuset);
         entry  = hwloc_bitmap_next(ccpuset, entry))
    {
      if (cnt == dart_thread_id) break;
      cnt++;
    }
  } while (cnt != dart_thread_id);

  //printf("Binding thread %d to CPU %d\n", dart_thread_id, entry);
  if (print_binding)
  {
    DART_LOG_INFO_ALWAYS("Binding thread %d to CPU %d", dart_thread_id, entry);
  }
  hwloc_bitmap_set(cpuset, entry);

  hwloc_set_thread_cpubind(topology, pthread, cpuset, 0);

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
dart__tasking__affinity_fini()
{
 // nothing to do
}

#endif // DART_ENABLE_HWLOC
