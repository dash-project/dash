
#include "DARTTaskingTest.h"
#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_globmem.h>

typedef struct testdata {
  int  expected;
  int  assign;
  int *valptr;
} testdata_t;

static void testfn_assign(void *data) {
  testdata_t *td = (testdata_t*)data;
  int *valptr = td->valptr;
  ASSERT_EQ(td->expected, *valptr);
  LOG_MESSAGE("[Task %p] testfn: incrementing valptr %p from %i",
    dart_task_current_task(), valptr, *valptr);
  *valptr = td->assign;
}

static void testfn_inc(void *data) {
  testdata_t *td = (testdata_t*)data;
  int *valptr = td->valptr;
//  ASSERT_EQ(td->expected, *valptr);
  LOG_MESSAGE("[Task %p] testfn: incrementing valptr %p from %i",
    dart_task_current_task(), valptr, *valptr);
  __sync_add_and_fetch(valptr, 1);
}

static void testfn_inc_yield(void *data) {
  testdata_t *td = (testdata_t*)data;
  volatile int *valptr = td->valptr;
//  ASSERT_EQ(td->expected, *valptr);
  LOG_MESSAGE("[Task %p] testfn: pre-yield increment of valptr %p from %i",
    dart_task_current_task(), valptr, *valptr);
  __sync_add_and_fetch(valptr, 1);
  // the last 20 tasks will be re-enqueued at the end
  dart_task_yield(20);
  LOG_MESSAGE("[Task %p] testfn: post-yield increment of valptr %p from %i",
    dart_task_current_task(), valptr, *valptr);
  __sync_add_and_fetch(valptr, 1);
}

static void testfn_nested_deps(void *data) {
  int  i;
  int  val = 0;
  int *valptr = &val; // dummy pointer used for synchronization, never accessed

  for (i = 0; i < 100; i++) {
    testdata_t td;
    td.valptr   = &val;
    td.expected = i;
    td.assign   = i + 1;

    dart_task_dep_t dep[2];
    dep[0].type = DART_DEP_INOUT;
    dep[0].gptr = DART_GPTR_NULL;
    dep[0].gptr.unitid = dash::myid();
    dep[0].gptr.teamid = dash::Team::All().dart_id();
    dep[0].gptr.addr_or_offs.addr = valptr;
    ASSERT_EQ(
      DART_OK,
      dart_task_create(
        &testfn_assign,      // action to call
        &td,                 // argument to pass
        sizeof(td),          // size of the tasks's data (if to be copied)
      dep,                   // dependency
      1,                     // number of dependencies
      DART_PRIO_LOW)
    );
  }

  dart_task_complete();

  ASSERT_EQ(i, val);
}

TEST_F(DARTTaskingTest, BulkAtomicIncrement)
{
  if (!dash::is_multithreaded()) {
    SKIP_TEST_MSG("Thread-support required");
  }

  int i;
  int val = 0;

  for (i = 0; i < 100; i++) {
    testdata_t td;
    td.valptr   = &val;
    ASSERT_EQ(
      DART_OK,
      dart_task_create(
        &testfn_inc,             // action to call
        &td,                 // argument to pass
        sizeof(td),          // size of the tasks's data (if to be copied)
        NULL,                // dependency
        0,                    // number of dependencies
        DART_PRIO_LOW)
    );
  }

  dart_task_complete();

  ASSERT_EQ(i, val);
}

TEST_F(DARTTaskingTest, Yield)
{
  if (!dash::is_multithreaded()) {
    SKIP_TEST_MSG("Thread-support required");
  }

  int i;
  int val = 0;

  for (i = 0; i < 100; i++) {
    testdata_t td;
    td.valptr   = &val;
    ASSERT_EQ(
      DART_OK,
      dart_task_create(
        &testfn_inc_yield,   // action to call
        &td,                 // argument to pass
        sizeof(td),          // size of the tasks's data (if to be copied)
        NULL,                // dependency
        0,                    // number of dependencies
        DART_PRIO_LOW)
    );
  }

  dart_task_complete();

  ASSERT_EQ(2*i, val);
}


TEST_F(DARTTaskingTest, LocalDirectDependency)
{
  if (!dash::is_multithreaded()) {
    SKIP_TEST_MSG("Thread-support required");
  }
  int i;
  int val = 0;

  dart_taskref_t prev_task = DART_TASK_NULL;

  for (i = 0; i < 100; i++) {
    testdata_t td;
    td.valptr   = &val;
    td.expected = i;
    td.assign   = i + 1;
    dart_task_dep_t dep;
    dep.type = DART_DEP_DIRECT;
    dep.task = prev_task;
    dart_taskref_t task;
    ASSERT_EQ(
      DART_OK,
      dart_task_create_handle(
        &testfn_assign,             // action to call
        &td,                 // argument to pass
        sizeof(td),          // size of the tasks's data (if to be copied)
        &dep,                // dependency
        1,                   // number of dependencies
        DART_PRIO_LOW,
        &task           // handle to be returned
        )
    );
    dart_task_freeref(&prev_task);
    prev_task = task;
  }
  dart_task_freeref(&prev_task);

  dart_task_complete();

  ASSERT_EQ(i, val);
}


TEST_F(DARTTaskingTest, LocalOutDependency)
{
  if (!dash::is_multithreaded()) {
    SKIP_TEST_MSG("Thread-support required");
  }
  int i;
  int val = 0;

  for (i = 0; i < 100; i++) {
    testdata_t td;
    td.valptr   = &val;
    td.expected = i;
    td.assign   = i + 1;

    // force serialization through an output chain
    dart_task_dep_t dep;
    dep.type = DART_DEP_OUT;
    dep.gptr = DART_GPTR_NULL;
    dep.gptr.unitid = dash::myid();
    dep.gptr.teamid = dash::Team::All().dart_id();
    dep.gptr.addr_or_offs.addr = &val;
    ASSERT_EQ(
      DART_OK,
      dart_task_create(
        &testfn_assign,              // action to call
        &td,                 // argument to pass
        sizeof(td),          // size of the tasks's data (if to be copied)
        &dep,                // dependency
        1,                    // number of dependencies
        DART_PRIO_LOW)
    );
  }

  dart_task_complete();

  ASSERT_EQ(i, val);
}

TEST_F(DARTTaskingTest, LocalInOutDependencies)
{
  if (!dash::is_multithreaded()) {
    SKIP_TEST_MSG("Thread-support required");
  }
  int  i;
  int  val = 0;
  int *valptr = &val;

  for (i = 0; i < 100; i++) {
    testdata_t td;
    td.valptr   = &val;
    td.expected = i;
    td.assign   = i + 1;

    dart_task_dep_t dep[2];
    dep[0].type = DART_DEP_IN;
    dep[0].gptr = DART_GPTR_NULL;
    dep[0].gptr.unitid = dash::myid();
    dep[0].gptr.teamid = dash::Team::All().dart_id();
    dep[0].gptr.addr_or_offs.addr = valptr;
    valptr++;
    dep[1].type = DART_DEP_OUT;
    dep[1].gptr = DART_GPTR_NULL;
    dep[1].gptr.unitid = dash::myid();
    dep[1].gptr.teamid = dash::Team::All().dart_id();
    dep[1].gptr.addr_or_offs.addr = valptr;
    ASSERT_EQ(
      DART_OK,
      dart_task_create(
        &testfn_assign,              // action to call
        &td,                 // argument to pass
        sizeof(td),          // size of the tasks's data (if to be copied)
        dep,                // dependency
        2,                    // number of dependencies
        DART_PRIO_LOW)
    );
  }

  dart_task_complete();

  ASSERT_EQ(i, val);
}

TEST_F(DARTTaskingTest, SameLocalInOutDependency)
{
  if (!dash::is_multithreaded()) {
    SKIP_TEST_MSG("Thread-support required");
  }
  int  i;
  int  val = 0;
  int *valptr = &val;

  for (i = 0; i < 100; i++) {
    testdata_t td;
    td.valptr   = &val;
    td.expected = i;
    td.assign   = i + 1;

    dart_task_dep_t dep[2];
    dep[0].type = DART_DEP_IN;
    dep[0].gptr = DART_GPTR_NULL;
    dep[0].gptr.unitid = dash::myid();
    dep[0].gptr.teamid = dash::Team::All().dart_id();
    dep[0].gptr.addr_or_offs.addr = valptr;
    dep[1].type = DART_DEP_OUT;
    dep[1].gptr = DART_GPTR_NULL;
    dep[1].gptr.unitid = dash::myid();
    dep[1].gptr.teamid = dash::Team::All().dart_id();
    dep[1].gptr.addr_or_offs.addr = valptr;
    ASSERT_EQ(
      DART_OK,
      dart_task_create(
        &testfn_assign,              // action to call
        &td,                 // argument to pass
        sizeof(td),          // size of the tasks's data (if to be copied)
        dep,                // dependency
        2,                    // number of dependencies
        DART_PRIO_LOW)
    );
  }

  dart_task_complete();

  ASSERT_EQ(i, val);
}

TEST_F(DARTTaskingTest, InOutDependency)
{
  if (!dash::is_multithreaded()) {
    SKIP_TEST_MSG("Thread-support required");
  }
  int  i;
  int  val = 0;
  int *valptr = &val; // dummy pointer used for synchronization, never accessed

  for (i = 0; i < 100; i++) {
    testdata_t td;
    td.valptr   = &val;
    td.expected = i;
    td.assign   = i + 1;

    dart_task_dep_t dep[2];
    dep[0].type = DART_DEP_INOUT;
    dep[0].gptr = DART_GPTR_NULL;
    dep[0].gptr.unitid = dash::myid();
    dep[0].gptr.teamid = dash::Team::All().dart_id();
    dep[0].gptr.addr_or_offs.addr = valptr;
    ASSERT_EQ(
      DART_OK,
      dart_task_create(
        &testfn_assign,              // action to call
        &td,                 // argument to pass
        sizeof(td),          // size of the tasks's data (if to be copied)
        dep,                // dependency
        1,                    // number of dependencies
        DART_PRIO_LOW)
    );
  }

  dart_task_complete();

  ASSERT_EQ(i, val);
}


TEST_F(DARTTaskingTest, NestedTaskDeps)
{
  if (!dash::is_multithreaded()) {
    SKIP_TEST_MSG("Thread-support required");
  }

  // create tasks that will nest
  for (int i = 0; i < dart_task_num_threads(); ++i) {
    ASSERT_EQ(
      DART_OK,
      dart_task_create(&testfn_nested_deps, NULL, 0, NULL, 0, DART_PRIO_HIGH)
    );
  }

  dart_task_complete();
}
