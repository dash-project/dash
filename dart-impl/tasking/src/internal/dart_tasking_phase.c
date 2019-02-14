#include <dash/dart/if/dart_tasking.h>
#include <dash/dart/if/dart_communication.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/base/assert.h>
#include <dash/dart/base/env.h>
#include <dash/dart/base/atomic.h>
#include <dash/dart/tasking/dart_tasking_phase.h>
#include <dash/dart/tasking/dart_tasking_priv.h>

static dart_taskphase_t creation_phase        = DART_PHASE_FIRST;
static dart_taskphase_t runnable_phase        = DART_PHASE_FIRST;
static dart_taskphase_t max_active_phases     = INT_MAX;
static dart_taskphase_t max_active_phases_mod = INT_MAX;
static dart_taskphase_t num_active_phases     = 0;
static dart_taskphase_t num_active_phases_lb  = 0;
static int32_t *phase_task_counts             = NULL;
static dart_taskphase_t matching_interval     = INT_MIN;
static dart_taskphase_t phases_remaining      = INT_MAX;
static float            matching_factor       = 1.0;
/*
 * by default, each phase's is 1 but it increases if previous phase(s)
 * did not see any tasks, i.e., the phase will also release the previous phase(s)
 */
static int32_t *phase_task_weight             = NULL;
static size_t num_units;

static void do_matching()
{
  DART_LOG_TRACE("Performing intermediate matching");
  dart__tasking__perform_matching(creation_phase);
  if (matching_interval < max_active_phases) {
    dart_taskphase_t next_interval = matching_interval * matching_factor;
    if (next_interval > 0) {
      if (next_interval < max_active_phases) {
        matching_interval = next_interval;
      } else {
        matching_interval = max_active_phases;
      }
    } else {
      matching_interval = 1;
    }
    DART_LOG_TRACE("Next matching interval: %d", matching_interval);
  }
  phases_remaining = matching_interval;
}

static void do_init()
{
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

        // if we reach the phase limit, we wait for 50% of them to be processed
        num_active_phases_lb = dart__base__env__number(
                                DART_MATCHING_PHASE_LB_ENVSTR,
                                max_active_phases / 2);
        if (num_active_phases_lb < matching_interval)
        {
          num_active_phases_lb = matching_interval;
        }

        phase_task_counts = calloc(max_active_phases_mod, sizeof(*phase_task_counts));
        phase_task_weight = malloc(max_active_phases_mod*sizeof(*phase_task_weight));
      }
      matching_factor = dart__base__env__float(
                            DART_MATCHING_PHASE_INTERVAL_FACTOR_ENVSTR,
                            1);
    } else {
      DART_LOG_TRACE("Intermediate task matching disabled");
    }
  }
}

static inline void
wait_for_active_phases(int32_t num_tasks_prev_phase)
{
  if (phase_task_counts != NULL) {
    // contribute to task execution until we are free to create tasks again
    int entry = creation_phase%max_active_phases_mod;

    // default weight is 1
    phase_task_weight[entry] = 1;
    if (num_tasks_prev_phase == 0) {
      // carry over weight from previous phase
      int prev_phase_entry = (creation_phase - 1)%max_active_phases_mod;
      phase_task_weight[entry] += phase_task_weight[prev_phase_entry];
    }

    if (num_active_phases == max_active_phases
        || phase_task_counts[entry] > 0)
    {
      while (num_active_phases > num_active_phases_lb
          || phase_task_counts[entry] > 0) {
        /*
        DART_LOG_TRACE("Waiting for phase %d to become available "
                      "(entry: %d, active phases: %d, in phase %d: %d)",
                      creation_phase, entry, num_active_phases,
                      creation_phase, phase_task_counts[entry]);
        */
        dart__tasking__yield(0);
      }
    }
    DART_ASSERT_MSG(
      DART_FETCH32(&phase_task_counts[entry]) == 0,
      "Active tasks in new phase %d are %d, entry %d should be zero",
      creation_phase,
      DART_FETCH32(&phase_task_counts[entry]),
      entry);
  }
}

void
dart__tasking__phase_advance()
{
  if (matching_interval == INT_MIN) {
    do_init();
  }

  // no need to handle phases if there is only one unit
  if (num_units == 1) {
    return;
  }

  int32_t num_tasks_prev_phase = -1;
  if (creation_phase >= 0) {
    num_tasks_prev_phase = phase_task_counts[creation_phase % max_active_phases_mod];
    DART_ASSERT(num_tasks_prev_phase >= 0);
  }

  if (--phases_remaining == 0) {
    do_matching();
  }

  ++creation_phase;
  wait_for_active_phases(num_tasks_prev_phase);
  DART_FETCH_AND_INC32(&num_active_phases);
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
    }
  }
}

void
dart__tasking__phase_take_task(dart_taskphase_t phase)
{
  if (NULL != phase_task_counts && phase >= 0) {
    int entry = phase%max_active_phases_mod;
    int32_t val = DART_DEC_AND_FETCH32(&phase_task_counts[entry]);
    DART_LOG_TRACE("Phase %d has %d tasks active!", phase, val);
    if (val == 0) {
      DART_LOG_TRACE("Phase %d is ready for re-use!", phase);
      DART_SUB_AND_FETCH32(&num_active_phases, phase_task_weight[entry]);
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
