
#include "DARTTaskingTest.h"
#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_globmem.h>
#include <dash/dart/if/dart_tasking.h>

#include <dash/tasks/Tasks.h>

#define TASK_CANCEL_CUTOFF 10

typedef struct testdata {
  int  expected;
  int  assign;
  int *valptr;
} testdata_t;

typedef struct globaltestdata {
  dart_gptr_t src;
  dart_gptr_t dst;
  int         expected;
} globaltestdata_t;

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


static void testfn_assign_cancel_global(void *data) {
  testdata_t *td = (testdata_t*)data;
  int *valptr = td->valptr;
  ASSERT_EQ(td->expected, *valptr);
  LOG_MESSAGE("[Task %p] testfn: incrementing valptr %p from %i",
    dart_task_current_task(), valptr, *valptr);
  *valptr = td->assign;

  if (td->assign == TASK_CANCEL_CUTOFF) {
    printf("Cancelling task %p\n", dart_task_current_task());
    dart_task_cancel_global();
    // this should never be executed
    *valptr = 0;
  }
}

static void testfn_assign_cancel_bcast_barrier(void *data) {
  testdata_t *td = (testdata_t*)data;
  int *valptr = td->valptr;
  ASSERT_EQ(td->expected, *valptr);
  LOG_MESSAGE("[Task %p] testfn: incrementing valptr %p from %i",
    dart_task_current_task(), valptr, *valptr);
  *valptr = td->assign;

  dash::barrier();

  // unit 0 broadcasts the abort to all other units
  if (td->assign == TASK_CANCEL_CUTOFF) {
    if (dash::myid() == 0) {
      printf("Cancelling task %p\n", dart_task_current_task());
      dart_task_cancel_bcast();
      // this should never be executed
      *valptr = 0;
    } else {
      while (1) {
        // wait for the signal to arrive
        dart_task_yield(1);
      }
    }
  }
}

static void testfn_assign_cancel_bcast(void *data) {
  globaltestdata_t *td = (globaltestdata_t*)data;
  int val;
  dart_get_blocking(&val, td->src, 1, DART_TYPE_INT);
  ASSERT_EQ_U(td->expected, val);
  int newval = val+1;
  dart_put_blocking(td->dst, &newval, 1, DART_TYPE_INT);
  LOG_MESSAGE("[Task %p] testfn: incremented value from %i to %i (t:%d,s:%d,o:%p,u:%d)",
    dart_task_current_task(), val, newval, td->src.teamid, td->dst.segid, td->dst.addr_or_offs.addr, td->dst.unitid);

  // unit 0 broadcasts the abort to all other units
  if (td->expected == TASK_CANCEL_CUTOFF) {
    if (dash::myid() == 0) {
      dart_get_blocking(&val, td->dst, 1, DART_TYPE_INT);
      printf("Cancelling task %p with dst=%d\n", dart_task_current_task(), val);
      dart_task_cancel_bcast();
      // this should never be executed
      int zero = 0;
      dart_put_blocking(td->dst, &zero, 1, DART_TYPE_INT);
    }
  }
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
  for (int i = 0; i < dart_task_num_threads()*10; ++i) {
    ASSERT_EQ(
      DART_OK,
      dart_task_create(&testfn_nested_deps, NULL, 0, NULL, 0, DART_PRIO_HIGH)
    );
  }

  dart_task_complete();
}


TEST_F(DARTTaskingTest, CancelLocal)
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
        &testfn_assign_cancel_global,              // action to call
        &td,                 // argument to pass
        sizeof(td),          // size of the tasks's data (if to be copied)
        &dep,                // dependency
        1,                    // number of dependencies
        DART_PRIO_LOW)
    );
  }

  dart_task_complete();

  ASSERT_EQ(TASK_CANCEL_CUTOFF, val);

}

TEST_F(DARTTaskingTest, CancelBcast)
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
        &testfn_assign_cancel_bcast_barrier,              // action to call
        &td,                 // argument to pass
        sizeof(td),          // size of the tasks's data (if to be copied)
        &dep,                // dependency
        1,                    // number of dependencies
        DART_PRIO_LOW)
    );
  }

  dart_task_complete();

  ASSERT_EQ(TASK_CANCEL_CUTOFF, val);
}


TEST_F(DARTTaskingTest, CancelBcastGlobalInDep)
{
  if (!dash::is_multithreaded()) {
    SKIP_TEST_MSG("Thread-support required");
  }

  int i;
  int val = 0;
  dart_gptr_t gptr1, gptr2;

  dart_team_memalloc_aligned(DART_TEAM_ALL, 1, DART_TYPE_INT, &gptr1);
  dart_put_blocking(gptr1, &val, 1, DART_TYPE_INT);
  dart_team_memalloc_aligned(DART_TEAM_ALL, 1, DART_TYPE_INT, &gptr2);
  dart_put_blocking(gptr2, &val, 1, DART_TYPE_INT);

  // create a bunch of tasks, one of them will abort
  for (i = 1; i <= 100; i++) {
    globaltestdata_t td;
    td.expected = i-1;
    // alternate allocations to avoid circular WAR dependencies
    dart_gptr_t in_gptr  = (i % 2) ? gptr1 : gptr2;
    in_gptr.unitid = ((dash::myid() + 1) % dash::size());
    dart_gptr_t out_gptr = (i % 2) ? gptr2 : gptr1;
    out_gptr.unitid = dash::myid();

    td.src      = in_gptr;
    td.dst      = out_gptr;

    // force serialization through an output chain
    // local output dependency
    dart_task_dep_t dep[3];
    dep[0].gptr  = out_gptr;
    dep[0].epoch = i;
    dep[0].type  = DART_DEP_OUT;
    // remote input dependency
    dep[1].gptr  = in_gptr;
    dep[1].epoch = i-1;
    dep[1].type  = DART_DEP_IN;
    // serialize iterations locally, otherwise only one result would be valid
    dep[2].gptr  = in_gptr;
    dep[2].gptr.unitid = dash::myid();
    dep[2].epoch = i-1;
    dep[2].type  = DART_DEP_IN;
    ASSERT_EQ(
      DART_OK,
      dart_task_create(
        &testfn_assign_cancel_bcast,              // action to call
        &td,                 // argument to pass
        sizeof(td),          // size of the tasks's data (if to be copied)
        dep,                 // dependency
        3,                   // number of dependencies
        DART_PRIO_LOW)
    );
  }

  dart_task_complete();

  // fetch result
  dart_get_blocking(&val, gptr1, 1, DART_TYPE_INT);
  // we will have (TASK_CANCEL_CUTOFF) increments on the first value
  ASSERT_EQ(TASK_CANCEL_CUTOFF, val);

  // fetch result
  dart_get_blocking(&val, gptr2, 1, DART_TYPE_INT);
  // we will have (TASK_CANCEL_CUTOFF + 1) increments on the second value
  ASSERT_EQ(TASK_CANCEL_CUTOFF + 1, val);

  dart_team_memfree(gptr1);
  dart_team_memfree(gptr2);

}

TEST_F(DARTTaskingTest, Abort)
{
  static int value = 0;
  // test the abortion mechanism of the C++ abstraction
  struct DtorIncrement {
    DtorIncrement() { }
    ~DtorIncrement() {
      ++value;
    }
  };

  dash::tasks::create([](){
    DtorIncrement dt;
    dash::tasks::abort_task();
    // this should not be executed
    value = 10;
  });

  dash::tasks::complete();

  ASSERT_EQ_U(1, value);
}
