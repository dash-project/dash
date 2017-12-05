#ifndef DART_TASKING_AFFINITY_H_
#define DART_TASKING_AFFINITY_H_

#ifdef DART_ENABLE_HWLOC
#include <hwloc.h>

static void
set_thread_affinity(pthread_t pthread, int dart_thread_id)
{
  // first: bind the thread
  hwloc_topology_t topology;
  hwloc_topology_init(&topology);
  hwloc_topology_load(topology);

  hwloc_const_cpuset_t ccpuset = hwloc_topology_get_allowed_cpuset(topology);
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

  printf("Binding thread %d to CPU %d\n", dart_thread_id, entry);
  hwloc_bitmap_set(cpuset, entry);

  hwloc_set_thread_cpubind(topology, pthread, cpuset, 0);

  hwloc_bitmap_free(cpuset);
}
#else
static void
set_thread_affinity(pthread_t pthread, int dart_thread_id)
{
  if (dart_thread_id == 0) {
    DART_LOG_INFO("Not pinning threads due to missing hwloc support!");
  }
}

#endif // DART_ENABLE_HWLOC


#endif // DART_TASKING_AFFINITY_H_
