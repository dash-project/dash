#ifndef BENCH_H
#define BENCH_H
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>

static FILE * get_file_handle(const char * path)
{
    char buffer[256];
    char host[100];
    gethostname(host, 99);
    char * jobid = "123";//getenv("SLURM_JOBID");
    int n = sprintf(buffer, "%s/%s-%s-%lu.txt", path, jobid, host, getpid());
    buffer[n] = '\0';
    return fopen(buffer, "w");
}

static double get_wtime()
{
    struct timeval tp;
    gettimeofday( &tp, 0 );
    return tp.tv_sec + ( tp.tv_usec * 1.0e-6 );
}


#endif /* BENCH_H */
