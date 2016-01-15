#include "wtime.h"
#include <sys/time.h>
#ifdef _OPENMP
#include <omp.h>
#endif

void wtime(double *t)
{
#if defined(_OPENMP) && (_OPENMP > 200010)
   /* Use omp_get_wtime() if we can */
   *t = omp_get_wtime();

#else
   /* Use a generic timer */
   static int sec = -1;
   struct timeval tv;
   gettimeofday(&tv, NULL);
   if (sec < 0) sec = tv.tv_sec;
   *t = (tv.tv_sec - sec) + 1.0e-6*tv.tv_usec;
#endif
}

    
