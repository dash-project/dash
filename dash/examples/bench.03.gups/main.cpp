/**
 *
 * Random Access (GUPS) benchmark
 *
 * Based on the UPC++ version of the same bechmark
 *
 */

#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h> // for int64_t and uint64_t

#include <iostream>

#include <libdash.h>

#ifndef N
#define N (25)
#endif

#define TableSize (1ULL<<N)
#define NUPDATE   (4ULL * TableSize)

#define POLY      0x0000000000000007ULL
#define PERIOD    1317624576693539401LL

typedef dash::CSRPattern<1, dash::ROW_MAJOR, int64_t> pattern_t;
// typedef dash::TilePattern<1, dash::ROW_MAJOR, int64_t> pattern_t;

dash::Array<uint64_t, int64_t, pattern_t> Table;


template<typename T> 
void print(dash::Array<T>& arr)
{
  for (auto el: arr) {
    std::cout << el << " ";
  }
  std::cout << std::endl;
}

uint64_t starts(int64_t n)
{
  int i;
  uint64_t m2[64];
  uint64_t temp, ran;

  while (n < 0)       n += PERIOD;
  while (n > PERIOD)  n -= PERIOD;

  if (n == 0)           
    return 0x1;

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
  uint64_t ran = starts(NUPDATE / dash::size() * dash::myid());

  for (i = dash::myid(); i < NUPDATE; i += dash::size()) {
    ran           = (ran << 1) ^ (((int64_t) ran < 0) ? POLY : 0);
    int64_t g_idx = static_cast<int64_t>(ran & (TableSize-1));
    Table[g_idx]  = Table[g_idx] ^ ran;
  }
}

uint64_t RandomAccessVerify()
{
  uint64_t i, localerrors, errors;
  localerrors = 0;
  for (i = dash::myid(); i < TableSize; i += dash::size()) {
    if (Table[i] != i) {
      localerrors++;
    }
  }
  errors = localerrors;
  return errors;
}

int main(int argc, char **argv)
{
  dash::util::Timer::timestamp_t ts_start;
  double duration_us;
  double GUPs;
  double latency;

  DASH_LOG_DEBUG("bench.gups", "main()");

  dash::init(&argc, &argv);

  dash::util::Timer::Calibrate(
    dash::util::TimeMeasure::Clock, 0);

  DASH_LOG_DEBUG("bench.gups", "Table.allocate()");

  Table.allocate(TableSize, dash::BLOCKED);

  if(dash::myid() == 0) {
    printf("\nTable size = %g MB/unit, %g MB/total on %d units\n",
           (double)TableSize*8/1024/1024/dash::size(),
           (double)TableSize*8/1024/1024,
           dash::size());
    printf("\nExecuting random updates...\n\n");
  }

  if(dash::myid() == 0) {
    for (uint64_t i = dash::myid(); i < TableSize; ++i) {
      Table[i] = i;
    }
  }

  dash::barrier(); 

  ts_start    = dash::util::Timer::Now();
  RandomAccessUpdate();
  dash::barrier(); 
  duration_us = dash::util::Timer::ElapsedSince(ts_start);

#if 0
  if (dash::myid() == 0) {
    print(Table);
  }

  uint64_t * lt = (uint64_t *) &(*Table)[dash::myid()];
  for (uint64_t i = dash::myid(); i < TableSize; i += dash::size()) {
    *lt++ = i;
  }
#endif

  GUPs        = ((double)(NUPDATE) / 1000.0f) / duration_us;
  latency     = duration_us * dash::size() / NUPDATE;

  if(dash::myid() == 0) {
    printf("Number of updates: %llu\n", NUPDATE);
    printf("Real time used:    %.6f seconds\n", 1.0e-6 * duration_us);
    printf("Update latency:    %.6f usecs\n", latency);
    printf("GUP/s:             %.6f Billion upates / second\n", GUPs);
  }

#if 1
  // Verify
  if (dash::myid() == 0) printf ("\nVerifying...\n");
  RandomAccessUpdate();  // do it again
  dash::barrier(); 
  uint64_t errors = RandomAccessVerify();
  if (dash::myid() == 0) {
    if ((double)errors/NUPDATE < 0.01) {
      printf ("Verification: SUCCESS (%llu errors in %llu updates)\n", 
              errors, NUPDATE);
    } else {
      printf ("Verification FAILED, (%llu errors in %llu updates)\n", 
              errors, NUPDATE);
    }
  }
#endif
  
  dash::finalize();
  
  return 0;
}
