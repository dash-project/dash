
#include <dash/dart/if/dart_tasking.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/tasking/dart_tasking_phase.h>

static dart_taskphase_t creation_phase = DART_PHASE_FIRST;
static dart_taskphase_t runnable_phase = DART_PHASE_FIRST;

void
dart__tasking__phase_advance()
{
  ++creation_phase;
}

dart_taskphase_t
dart__tasking__phase_current()
{
  return creation_phase;
}

bool
dart__tasking__phase_is_runnable(dart_taskphase_t phase)
{
  return (runnable_phase == DART_PHASE_ANY || phase <= runnable_phase);
}

void
dart__tasking__phase_set_runnable(dart_taskphase_t phase)
{
  if (phase == DART_PHASE_ANY) {
    DART_LOG_TRACE("Marking all phases as runnable");
  } else {
    DART_LOG_TRACE("Marking phases up to %d as runnable", phase);
  }
  runnable_phase = phase;
}

dart_taskphase_t
dart__tasking__phase_runnable()
{
  return runnable_phase;
}
