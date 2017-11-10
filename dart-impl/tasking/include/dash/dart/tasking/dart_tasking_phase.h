#ifndef DART__BASE__PHASE__TASKING_H__
#define DART__BASE__PHASE__TASKING_H__

#include <dash/dart/if/dart_tasking.h>
#include <dash/dart/if/dart_team_group.h>

void
dart__tasking__phase_advance();

dart_taskphase_t
dart__tasking__phase_current();

bool
dart__tasking__phase_is_runnable(dart_taskphase_t phase);

void
dart__tasking__phase_set_runnable(dart_taskphase_t phase);

dart_taskphase_t
dart__tasking__phase_runnable();

dart_ret_t
dart__tasking__phase_resync(dart_team_t team);


#endif // DART__BASE__PHASE__TASKING_H__
