#ifndef DART__TASKING__INTERNAL_AYURDAME_H__
#define DART__TASKING__INTERNAL_AYURDAME_H__

#include <ayu_events.h>

#ifdef AYU_UNKNOWN_NTHREADS

static inline
uint64_t dart__tasking__ayudame_make_globalunique_unit(
  dart_task_t       *task,
  dart_global_unit_t unit)
{
  uint64_t myid64 = unit.id;
  uint64_t task64 = (uint64_t)task;
  uint64_t mask64 = (1UL<<48) - 1;

  return (task64 & mask64) | (myid64 << 48);
}

static inline
uint64_t dart__tasking__ayudame_make_globalunique(dart_task_t *task)
{
  dart_global_unit_t myid;
  dart_myid(&myid);

  return dart__tasking__ayudame_make_globalunique_unit(task, myid);
}

#else

#define dart__tasking__ayudame_make_globalunique(__task) ((uint64_t)__task)
#define dart__tasking__ayudame_make_globalunique_unit(__task, unit) ((uint64_t)__task)

#endif // AYU_UNKNOWN_NTHREADS

// TODO: This should come from the build system
#define HAVE_AYUDAME

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_AYUDAME2

void dart__tasking__ayudame_init();

void dart__tasking__ayudame_fini();

void dart__tasking__ayudame_create_task(void *task, void *parent);

void dart__tasking__ayudame_close_task(void *task);

void dart__tasking__ayudame_add_dependency(void *srctask, void *dsttask);

#else

static inline void dart__tasking__ayudame_init() {}

static inline void dart__tasking__ayudame_fini() {}

static inline void dart__tasking__ayudame_create_task(void *task, void *parent) {}

static inline void dart__tasking__ayudame_close_task(void *task) {}

static inline void dart__tasking__ayudame_add_dependency(void *srctask, void *dsttask) {}

#endif


#ifdef __cplusplus
}
#endif
#endif // DART__TASKING__INTERNAL_AYURDAME_H__
