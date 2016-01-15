#include <stdio.h>

#define PAL_DASH
#include "../../include/dash/omp/PAL.h"

int main(int argc, char *argv[]) {
  PAL_INIT;
  
  PAL_SHARED_ARR_DECL(arr, int, 100);
  PAL_PARALLEL_BEGIN();
  int i;

  PAL_FOR_NOWAIT_BEGIN(0, 99, 1, i, int);
    {
      PAL_SHARED_ARR_SET(arr, i, i*i);
    }
  PAL_FOR_NOWAIT_END;
    
  PAL_PARALLEL_END;
  
  PAL_SEQUENTIAL_BEGIN;
  printf("Hello world only printed once\n");
  PAL_SEQUENTIAL_END;
  
  PAL_FINALIZE;
}
