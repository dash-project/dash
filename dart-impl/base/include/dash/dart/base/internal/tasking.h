#ifndef DART__BASE__INTERNAL__TASKING_H__
#define DART__BASE__INTERNAL__TASKING_H__

dart_ret_t
dart__base__tasking__init();

int
dart__base__tasking__thread_num();

int
dart__base__tasking__num_threads();

dart_ret_t
dart__base__tasking__create_task(void (*fn) (void *), void *data, dart_task_dep_t *deps, size_t ndeps);

dart_ret_t
dart__base__tasking__task_complete();

dart_ret_t
dart__base__tasking__fini();


#endif /* DART__BASE__INTERNAL__TASKING_H__ */
