#include <dash/dart/if/dart_tasking.h>
#include <dash/dart/if/dart_communication.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/base/assert.h>
#include <dash/dart/base/env.h>
#include <dash/dart/base/atomic.h>
#include <dash/dart/tasking/dart_tasking_phase.h>
#include <dash/dart/tasking/dart_tasking_priv.h>

static dart_taskphase_t creation_phase = DART_PHASE_FIRST;
static dart_taskphase_t runnable_phase = DART_PHASE_FIRST;
static dart_taskphase_t max_active_phases = INT_MAX;
static dart_taskphase_t max_active_phases_mod = INT_MAX;
static dart_taskphase_t num_active_phases = 0;
static int32_t *phase_task_counts = NULL;
static size_t num_units;


void
dart__tasking__phase_advance()
{
  static dart_taskphase_t matching_interval = INT_MIN;
  static dart_taskphase_t phases_remaining  = INT_MAX;
  static dart_mutex_t     allocation_mutex  = DART_MUTEX_INITIALIZER;
  if (matching_interval == INT_MIN) {
    dart__base__mutex_lock(&allocation_mutex);
    dart_team_size(DART_TEAM_ALL, &num_units);
    if (num_units == 1) {
      // disable matching
      matching_interval = -1;
    }
    if (matching_interval == INT_MIN)
    {
      matching_interval = dart__base__env__number(
                            DART_MATCHING_INTERVAL_ENVSTR, -1);
      if (matching_interval > 0) {
        phases_remaining = matching_interval;
        DART_LOG_TRACE("Intermediate task matching enabled: interval %d",
                      matching_interval);
        max_active_phases = dart__base__env__number(
                              DART_MATCHING_PHASE_MAX_ACTIVE_ENVSTR,
                              3*matching_interval);
        if (max_active_phases > -1) {
          if (max_active_phases < matching_interval) {
            DART_LOG_WARN(
              "The number of max active phases (%d) is set smaller than the matching "
              "interval (%d), which will likely lead to a deadlock. Adjusting to 2x "
              "the matching interval.", max_active_phases, matching_interval);
            max_active_phases = 2*matching_interval;
          }
          // give us some wiggle-room in case not all phases create tasks
          max_active_phases_mod = 1.2*max_active_phases;
          phase_task_counts = calloc(sizeof(*phase_task_counts), max_active_phases_mod);
        }
      } else {
        DART_LOG_TRACE("Intermediate task matching disabled");
      }
    }
    dart__base__mutex_unlock(&allocation_mutex);
  }

  // no need to handle phases if there is only one unit
  if (num_units == 1) {
    return;
  }

  if (--phases_remaining == 0) {
    DART_LOG_TRACE("Performing intermediate matching");
    dart__tasking__perform_matching(creation_phase);
    phases_remaining = matching_interval;
  }

  ++creation_phase;
  if (phase_task_counts != NULL) {
    // contribute to task execution until we are free to create tasks again
    int entry = creation_phase%max_active_phases_mod;
    while (DART_FETCH32(&num_active_phases) == max_active_phases
        || DART_FETCH32(&phase_task_counts[entry]) > 0) {
      /*
      DART_LOG_TRACE("Waiting for phase %d to become available "
                    "(entry: %d, active phases: %d, in phase %d: %d)",
                    creation_phase, entry, num_active_phases,
                    creation_phase, phase_task_counts[entry]);
      */
      dart__tasking__yield(0);
    }

    DART_ASSERT_MSG(
      DART_FETCH32(&phase_task_counts[entry]) == 0,
      "Active tasks in new phase %d are %d, entry %d should be zero",
      creation_phase,
      DART_FETCH32(&phase_task_counts[entry]),
      entry);
  }
}

dart_taskphase_t
dart__tasking__phase_current()
{
  return creation_phase;
}

bool
dart__tasking__phase_is_runnable(dart_taskphase_t phase)
{
  return (DART_PHASE_ANY == phase ||
          DART_PHASE_ANY == runnable_phase ||
          phase <= runnable_phase);
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

void
dart__tasking__phase_reset()
{
  creation_phase = DART_PHASE_FIRST;
  runnable_phase = DART_PHASE_FIRST;
}

void
dart__tasking__phase_add_task()
{
  if (NULL != phase_task_counts && creation_phase >= 0) {
    int32_t val = DART_FETCH_AND_INC32(
                    &phase_task_counts[creation_phase%max_active_phases_mod]);
    if (val == 0) {
      DART_LOG_TRACE("Phase %d saw its first task!", creation_phase);
      DART_INC_AND_FETCH32(&num_active_phases);
    }
  }
}

void
dart__tasking__phase_take_task(dart_taskphase_t phase)
{
  if (NULL != phase_task_counts && phase >= 0) {
    int32_t val = DART_DEC_AND_FETCH32(
                    &phase_task_counts[phase%max_active_phases_mod]);
    DART_LOG_TRACE("Phase %d has %d tasks active!", phase, val);
    if (val == 0) {
      DART_LOG_TRACE("Phase %d is ready for re-use!", phase);
      DART_DEC_AND_FETCH32(&num_active_phases);
    }
  }
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
