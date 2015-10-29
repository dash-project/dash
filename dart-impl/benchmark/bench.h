#ifndef BENCH_H
#define BENCH_H
//~ #include <time.h>
#include <sys/time.h>

static double get_wtime()
{
    struct timeval tp;
    gettimeofday( &tp, 0 );
    return tp.tv_sec + ( tp.tv_usec * 1.0e-6 );
}


#endif /* BENCH_H */
