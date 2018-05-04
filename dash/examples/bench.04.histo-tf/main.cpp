#include <libdash.h>

#include "../bench.h"

#include <algorithm>
#include <iostream>
#include <iomanip>

using std::cout;
using std::cerr;
using std::endl;

typedef dash::util::Timer<
          dash::util::TimeMeasure::Clock
        > Timer;

// Benchmark specification:
//   NAS Parallel Benchmark, Kernel IS
//   https://www.nas.nasa.gov/assets/pdf/techreports/1994/rnr-94-007.pdf

// NOTE:
//   In the NBP reference implementation, keys are first sorted to buckets
//   to determine their coarse distribution.
//   For example, for key range (0, 2^23) and buckset size s_b = 2^10, a
//   histogram with n_b = 2^(23-10) = 2^13 bins of size 2^10 = 1024 is
//   created so that bucket[b] holds the number of keys with values
//   between (b * s_b) and ((b+1) * s_b).

#if   defined(DASH__BENCH__HISTO__CLASS_A)
#  define TOTAL_KEYS_LOG_2    23
#  define MAX_KEY_LOG_2       19
#  define NUM_BUCKETS_LOG_2   10
#  define I_MAX               10
#  define SEED                314159265
#elif defined(DASH__BENCH__HISTO__CLASS_B)
#  define TOTAL_KEYS_LOG_2    25
#  define MAX_KEY_LOG_2       21
#  define NUM_BUCKETS_LOG_2   10
#  define I_MAX               10
#  define SEED                314159265
#else
// Debug configuration
#  define TOTAL_KEYS_LOG_2    29
#  define MAX_KEY_LOG_2       22
#  define NUM_BUCKETS_LOG_2   3
#  define I_MAX               1
#  define SEED                314159265
#endif

#define TOTAL_KEYS            (1 << TOTAL_KEYS_LOG_2)
#define MAX_KEY               (1 << MAX_KEY_LOG_2)
#define NUM_BUCKETS           (1 << NUM_BUCKETS_LOG_2)

#if __NOTE
// Defined at run time as value of NUM_PROCS is dynamic
#define  NUM_KEYS             (TOTAL_KEYS / NUM_PROCS * MIN_PROCS)
#endif

double randlc(double *, double *);
double find_my_seed(int, int, long, double, double);

int main(int argc, char **argv)
{
  dash::init(&argc, &argv);

  Timer::Calibrate(0);

  int    myid       = dash::myid();
  size_t num_units  = dash::size();
  // Maximum number of keys per unit:
  size_t NUM_KEYS   = dash::math::div_ceil(TOTAL_KEYS, num_units);

  if (TOTAL_KEYS <= (num_units-1) * NUM_KEYS) {
    // One unit would have no key range assigned, exit.
    if (myid == 0) {
      cerr << "Invalid number of units"
           << endl;
    }
    return -1;
  }

  // input, global array of keys
  dash::Array<int> key_array(TOTAL_KEYS, dash::BLOCKED);
  // result histograms, one per unit
  dash::Array<int> key_histo(MAX_KEY * num_units, dash::BLOCKED);

  // {{{
  //
  // PROCEDURE STEP 1 ----------------------------------------------------
  // "In a scalar sequential manner and using the key generation algorithm
  //  described above, generate the sequence of N keys."
  //

  // Random number generator multiplier
  double a    = 1220703125.00;
  // Random number generator seed
  double seed = find_my_seed(
                  dash::myid(),
                  dash::size(),
                  4 * static_cast<long>(TOTAL_KEYS),
                  SEED,
                  a);
  int    k    = MAX_KEY / 4;
  double x;
  for(size_t l = 0; l < key_array.lsize(); l++) {
    x  = randlc(&seed, &a);
    x += randlc(&seed, &a);
    x += randlc(&seed, &a);
    x += randlc(&seed, &a);
    key_array.local[l] = k * x;
  }

  // PROCEDURE STEP 2 ----------------------------------------------------
  // "Using the appropriate memory mapping described above, load the
  //  N keys into the memory system."
  //

  if (myid == 0) {
    cout << std::setw(25) << "Number of keys: "
         << std::setw(18) << key_array.size()
         << endl
         << std::setw(25) << "Max key: "
         << std::setw(18) << MAX_KEY
         << endl;
  }

  // }}}

  // Wait for initialization of input values:
  dash::barrier();

  // PROCEDURE STEP 3 ----------------------------------------------------
  // "Begin timing."
  //
  auto ts_start = Timer::Now();

  // PROCEDURE STEP 4 ----------------------------------------------------
  // "Do, for i = 1 to I_max"
  //
  // PROCEDURE STEP 4.a --------------------------------------------------
  // "Modify the sequence of keys by making the following two changes:
  //   - K[i]          <- i
  //   - K[i + I_max]  <- B_max - i"
  //

  // PROCEDURE STEP 4.b --------------------------------------------------
  // "Compute the rank of each key."
  //

  // Compute the histogram for the values in local range:
  for (size_t i = 0; i < key_array.lsize(); i++) {
    int value = key_array.local[i];
    ++key_histo.local[value];
  }

  if (dash::myid() != 0) {
    // Add local histogram values to result histogram at unit 0:
    dash::transform(key_histo.lbegin(), // A begin
                         key_histo.lend(),   // A end
                         key_histo.begin(),  // B
                         key_histo.begin(),  // C = plus(A,B)
                         dash::plus<int>());
  }
  // Wait for all units to accumulate their local results to local
  // histogram of unit 0:
  dash::barrier();

  if (dash::myid() != 0) {
    // Overwrite local histogram result with result histogram from unit 0:
    dash::copy(key_histo.begin(),           // Begin of block at unit 0
               key_histo.begin() + MAX_KEY, // End of block at unit 0
               key_histo.lbegin());
  }

  // Wait for all units to obtain the result histogram:
  dash::barrier();

  // PROCEDURE STEP 5 ----------------------------------------------------
  // "End timing."
  //

  auto time_elapsed_usec = Timer::ElapsedSince(ts_start);
  auto mkeys_per_sec     = static_cast<double>(TOTAL_KEYS) /
                             time_elapsed_usec;

  if (myid == 0) {
    cout << "-------------------------------------------"
         << endl
         << std::setw(25) << "MKeys/sec: "
         << std::setw(18) << mkeys_per_sec
         << endl;
  }

  dash::finalize();

  return 0;
}

/**
 * Implementation from NAS Parallel Benchmark MPI implementation of
 * Kernel IS.
 *
 * See NBP3.3-MPI/IS/is.c
 */
double randlc(double * X, double * A)
{
  static int    KS = 0;
  static double  R23, R46, T23, T46;
  double    T1, T2, T3, T4;
  double    A1;
  double    A2;
  double    X1;
  double    X2;
  double    Z;
  int       i, j;
  if (KS == 0) {
    R23 = 1.0;
    R46 = 1.0;
    T23 = 1.0;
    T46 = 1.0;
    for (i = 1; i <= 23; i++) {
      R23 = 0.50 * R23;
      T23 = 2.0  * T23;
    }
    for (i = 1; i <= 46; i++) {
      R46 = 0.50 * R46;
      T46 = 2.0  * T46;
    }
    KS = 1;
  }
  /*
      Break A into two parts such that A = 2^23 * A1 + A2 and set X = N.
   */
  T1 = R23 * *A;
  j  = T1;
  A1 = j;
  A2 = *A - T23 * A1;
  /*
      Break X into two parts such that X = 2^23 * X1 + X2, compute
      Z = A1 * X2 + A2 * X1  (mod 2^23), and then
      X = 2^23 * Z + A2 * X2  (mod 2^46).
  */
  T1 = R23 * *X;
  j  = T1;
  X1 = j;
  X2 = *X - T23 * X1;
  T1 = A1 * X2 + A2 * X1;
  j  = R23 * T1;
  T2 = j;
  Z  = T1 - T23 * T2;
  T3 = T23 * Z + A2 * X2;
  j  = R46 * T3;
  T4 = j;
  *X = T3 - T46 * T4;
  return(R46 * *X);
}

/**
 * Implementation from NAS Parallel Benchmark MPI implementation of
 * Kernel IS.
 *
 * See NBP3.3-MPI/IS/is.c
 *
 * Create a random number sequence of total length nn residing
 * on np number of processors.  Each processor will therefore have a
 * subsequence of length nn/np.  This routine returns that random
 * number which is the first random number for the subsequence belonging
 * to processor rank kn, and which is used as seed for proc kn ran # gen.
 */
double find_my_seed(int    kn,   /* my processor rank, 0<=kn<=num procs */
                    int    np,   /* np = num procs                      */
                    long   nn,   /* total num of ran numbers, all procs */
                    double s,    /* Ran num seed, for ex.: 314159265.00 */
                    double a)    /* Ran num gen mult, try 1220703125.00 */
{
  long   i;
  double t1,t2,t3,an;
  long   mq,nq,kk,ik;

  nq = nn / np;

  for (mq = 0; nq > 1; mq++, nq /= 2);

  t1 = a;
  for (i = 1; i <= mq; i++) {
    t2 = randlc( &t1, &t1 );
  }
  an = t1;
  kk = kn;
  t1 = s;
  t2 = an;
  for (i = 1; i <= 100; i++) {
    ik = kk / 2;
    if (2 * ik != kk) {
      t3 = randlc(&t1, &t2);
    }
    if (ik == 0) {
      break;
    }
    t3 = randlc(&t2, &t2);
    kk = ik;
  }
  // To avoid compiler warning of unused variable:
  (void)(t3);

  return(t1);
}

