/**
 *
 * Random Access (GUPS) benchmark
 *
 * Sequential version based on UPC version
 *
 */

#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <stdint.h> // for int64_t and uint64_t

#ifndef N
#define N (23)
#endif

#define ranks()   1
#define myrank()  0

#define TableSize (1ULL<<N)
#define NUPDATE   (4ULL * TableSize)

#define POLY      0x0000000000000007ULL
#define PERIOD    1317624576693539401LL

uint64_t Table[TableSize];

double get_time()
{
  struct timeval tv;
  gettimeofday(&tv, 0);
  return tv.tv_sec + ((double) tv.tv_usec / 1000000);
}

uint64_t starts(int64_t n)
{
  int i;
  uint64_t m2[64];
  uint64_t temp, ran;

  while (n < 0)         n += PERIOD;
  while (n > PERIOD)    n -= PERIOD;

  if (n == 0)           return 0x1;

  temp = 0x1;
  for (i=0; i<64; i++) {
    m2[i] = temp;
    temp = (temp << 1) ^ ((int64_t) temp < 0 ? POLY : 0);
    temp = (temp << 1) ^ ((int64_t) temp < 0 ? POLY : 0);
  }

  for (i=62; i>=0; i--) if ((n >> i) & 1) break;

  ran = 0x2;
  while (i > 0) {
    temp = 0;
    for (int j=0; j<64; j++) if ((ran >> j) & 1) temp ^= m2[j];
    ran = temp;
    i -= 1;
    if ((n >> i) & 1)  ran = (ran << 1) ^ ((int64_t) ran < 0 ? POLY : 0);
  }
  
  return ran;
}

void RandomAccessUpdate()
{
  uint64_t i;
  uint64_t ran = starts(NUPDATE / ranks() * myrank());

  for (i = myrank(); i < NUPDATE; i += ranks()) {
    ran = (ran << 1) ^ (((int64_t) ran < 0) ? POLY : 0);
    // Table[ran & (TableSize-1)] = Table[ran & (TableSize-1)] ^ ran;
    Table[ran & (TableSize-1)] ^= ran;
  }
}

uint64_t RandomAccessVerify()
{
  uint64_t i, localerrors, errors;
  localerrors = 0;
  for (i = myrank(); i < TableSize; i += ranks()) {
    if (Table[i] != i) {
      localerrors++;
    }
  }
  //upcxx_reduce(&localerrors, &errors, 1, 0, UPCXX_SUM, UPCXX_ULONG_LONG);
  errors=localerrors;
  return errors;
}

int main(int argc, char **argv)
{
  double time;
  double GUPs;
  double latency;

  //upcxx::init(&argc, &argv);

  //Table.init();

  if(myrank() == 0) {
    printf("\nTable size = %g MBytes/CPU, %g MB/total on %d threads\n",
           (double)TableSize*8/1024/1024/ranks(),
           (double)TableSize*8/1024/1024,
           ranks());
    printf("\nExecuting randome updates...\n\n");
  }

  // barrier(); // upc_barrier;

  time = get_time();
  uint64_t * lt = (uint64_t *) &Table[myrank()];
  for (uint64_t i = myrank(); i < TableSize; i += ranks()) {
    *lt++ = i;
  }

  // barrier(); 
  RandomAccessUpdate();
  // barrier(); 

  time = get_time() - time;
  GUPs = (double)NUPDATE * 1e-9 / time;
  latency = time * ranks() / NUPDATE * 1e6;

  if(myrank() == 0) {
    printf("Number of updates = %llu\n", NUPDATE);
    printf("Real time used = %.6f seconds\n", time );
    printf("%.9f Billion(10^9) Updates per second [GUP/s]\n", GUPs);
    printf("Update latency = %6.2f usecs\n", latency);
  }

#if 1 // VERIFY  
  if (myrank() == 0) printf ("\nVerifying...\n");
  RandomAccessUpdate();  // do it again
  // barrier(); 
  uint64_t errors = RandomAccessVerify();
  if (myrank() == 0) {
    if ((double)errors/NUPDATE < 0.01) {
      printf ("Verification: SUCCESS (%llu errors in %llu updates)\n", 
              errors, NUPDATE);
    } else {
      printf ("Verification FAILED, (%llu errors in %llu updates)\n", 
              errors, NUPDATE);
    }
  }
#endif
  
  // barrier();
  // upcxx::finalize();

  return 0;
}
