#ifndef WTIME_H_INCLUDED
#define WTIME_H_INCLUDED

void wtime(double *t);
double elapsed_time(void);
void timer_clear(int n);
void timer_start(int n);
void timer_stop(int n);
double timer_read(int n);

#endif
