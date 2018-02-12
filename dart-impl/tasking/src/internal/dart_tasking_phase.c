#include <dash/dart/if/dart_tasking.h>
#include <dash/dart/if/dart_communication.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/base/assert.h>
#include <dash/dart/base/env.h>
#include <dash/dart/tasking/dart_tasking_phase.h>
#include <dash/dart/tasking/dart_tasking_priv.h>

static dart_taskphase_t creation_phase = DART_PHASE_FIRST;
static dart_taskphase_t runnable_phase = DART_PHASE_FIRST;

void
dart__tasking__phase_advance()
{
  static dart_taskphase_t matching_interval = INT_MIN;
  static dart_taskphase_t phases_remaining = INT_MAX;
  if (matching_interval == INT_MIN) {
    matching_interval = dart__base__env__number(
                          DART_MATCHING_FREQUENCY_ENVSTR, -1);
    if (matching_interval > 0) {
      phases_remaining = matching_interval;
    }
  }
//  if (matching_interval > 0 && creation_phase > 0 && creation_phase % matching_interval == 0) {
  if (--phases_remaining == 0) {
    dart__tasking__perform_matching(dart__tasking__current_thread(),
                                    creation_phase);
    phases_remaining = matching_interval;
  }
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

dart_ret_t
dart__tasking__phase_resync(dart_team_t team)
{
  dart_taskphase_t max_phase;

  int ret = dart_allreduce(
      &creation_phase,
      &max_phase,
      1, DART_TYPE_INT,
      DART_OP_MAX,
      team);
  if (ret != DART_OK) {
    return ret;
  }

  dart_taskphase_t new_phase = ++max_phase;
  DART_ASSERT_MSG(new_phase >= creation_phase, "Phase overflow detected!");
  creation_phase = new_phase;

  return DART_OK;
}
