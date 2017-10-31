#ifndef DART__TASKING__INTERNAL_AYURDAME_H__
#define DART__TASKING__INTERNAL_AYURDAME_H__


// TODO: This should come from the build system
#define HAVE_AYUDAME

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_AYUDAME

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
