#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <libdash.h>

#include "common.h"

#define ARR_SIZE 6561

dart_lock_t lock;
dash::Array<double> arr;

void refer() {
  int j;
  for (j = 0; j < innerreps; j++) {
    delay(delaylength);
  }
}

#if 0
void testfor() {
  int j;
  for (j = 0; j < innerreps; j++) {
    dash::omp::for_loop(nthreads, dash::BLOCKED, [&](int i){
      delay(delaylength);
    });
  }
}
#endif 

void testbar() {
  int j;
  for (j = 0; j < innerreps; j++) {
    delay(delaylength);
    dash::barrier();
  }
}

void testmaster() {
  int j;
  for (j = 0; j < innerreps; j++) {
    dash::omp::master([]{
      delay(delaylength);
    });
  }
}

void testsing() {
  int j;
  for (j = 0; j < innerreps; j++) {
    dash::omp::master([]{
      delay(delaylength);
    });
  }
}

void testcrit() {
  int j;
  for (j = 0; j < innerreps / nthreads; j++) {
    dash::omp::critical("testcrit", []{
      delay(delaylength);
    });
  }
}

void testlock() {
  int j;
  dash::omp::Mutex m;
  for (j = 0; j < innerreps / nthreads; j++) {
    m.lock();
    delay(delaylength);
    m.unlock();
  }
}

void testdlock() {
  int j;
  for (j = 0; j < innerreps / nthreads; j++) {
    dart_lock_acquire(lock);
    delay(delaylength);
    dart_lock_release(lock);
  }
}

void referred() {
  int j;
  int aaaa = 0;
  for (j = 0; j < innerreps; j++) {
    delay(delaylength);
    aaaa += 1;
  }
}

#if 0
void testred() {
  int j;
  int aaaa = 0;
  dash::omp::Reduction<int, dash::omp::Op::Plus<int> > red(aaaa);
  for (j = 0; j < innerreps; j++) {
    delay(delaylength);
    aaaa += 1;
  }
  red.reduce(aaaa);
}
#endif 

void refarr() {
  int j;
  double a[1];
  for (j = 0; j < innerreps; j++) {
    array_delay(delaylength, a);
  }
}

void testarr1() {
  int j;
  for (j = 0; j < innerreps; j++) {
    array_delay_dash(delaylength, arr);
  }
}

void testarr2() {
  int j;
  for (j = 0; j < innerreps; j++) {
    array_delay_dash_6500(delaylength, arr);
  }
}

void testarr3() {
  int j;
  for (j = 0; j < innerreps; j++) {
    array_delay_dash_local(delaylength, arr);
  }
}

int main(int argc, char **argv) {
  char name[32];
  dash::init(&argc, &argv);
  init(argc, argv);
  
  // init locks, mutexes, arrays
  //dash::omp::mutex_init("testcrit");
  //dash::omp::mutex_init("testlock");
  //dart_team_lock_init(DART_TEAM_ALL, &lock);
  //arr.allocate(ARR_SIZE, dash::BLOCKED);
  
  // test block 1
  //reference("reference time 1", &refer);
  //benchmark("FOR", &testfor);
  benchmark("BARRIER", &testbar);
  //benchmark("SINGLE", &testsing);
  //benchmark("MASTER", &testmaster);
  //benchmark("CRITICAL", &testcrit);
  //benchmark("LOCK/UNLOCK (MUTEX)", &testlock);
  //benchmark("LOCK/UNLOCK (DART)", &testdlock);
  
  // test block 2
  //reference("reference time 2", &referred);
  //benchmark("REDUCTION", &testred);
  
  // test block 3
  //reference("reference time 3", &refarr);
  //sprintf(name, "ARRAY[%d] element 0 regular", ARR_SIZE);
  //benchmark(name, &testarr1);
  //sprintf(name, "ARRAY[%d] element 6500 regular", ARR_SIZE);
  //benchmark(name, &testarr2);
  //sprintf(name, "ARRAY[%d] element 0 local", ARR_SIZE);
  //benchmark(name, &testarr3);
  
  finalise();
  dash::finalize();
  return EXIT_SUCCESS;
}
