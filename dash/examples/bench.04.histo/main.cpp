#include <libdash.h>

#include "../bench.h"

#include <algorithm>
#include <iostream>
#include <iomanip>

using std::cout;
using std::cerr;
using std::endl;

using dash::util::Timer;
using dash::util::TimeMeasure;

// Benchmark specification:
//   NAS Parallel Benchmark, Kernel IS
//   https://www.nas.nasa.gov/assets/pdf/techreports/1994/rnr-94-007.pdf

// NOTE: 
//   In the NBP reference implementation, keys are first sorted to buckets to
//   determine their coarse distribution.
//   For example, for key range (0, 2^23) and buckset size s_b = 2^10, a
//   histogram with n_b = 2^(23-10) = 2^13 bins of size 2^10 = 1024 is created
//   so that bucket[b] holds the number of keys with values between (b * s_b)
//   and ((b+1) * s_b).

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
#  define TOTAL_KEYS_LOG_2    23
#  define MAX_KEY_LOG_2       21
#  define NUM_BUCKETS_LOG_2   3
#  define I_MAX               1
#  define SEED                314159265
#  define DBGOUT
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

  Timer::Calibrate(TimeMeasure::Clock);

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

  // global array of keys and histogram
  dash::Array<int> key_array(TOTAL_KEYS, dash::BLOCKED);
  dash::Array<int> key_histo(MAX_KEY,    dash::BLOCKED);
  
  // Global array of global pointers to local histogram buffer of all units:
  dash::Array< dash::GlobPtr<int> > local_buffers(num_units, dash::CYCLIC);
  // Allocate local histogram buffer for active unit:
  local_buffers[myid]          = dash::memalloc<int>(MAX_KEY);
  dash::GlobPtr<int> gptr      = local_buffers[myid];
  int *              local_buf = (int*)(gptr);
  
  // PROCEDURE STEP 1 --------------------------------------------------------
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
  for(int l = 0; l < key_array.lsize(); l++) {
    x  = randlc(&seed, &a);
    x += randlc(&seed, &a);
    x += randlc(&seed, &a);
    x += randlc(&seed, &a);  
    key_array.local[l] = k * x;
  }

  // PROCEDURE STEP 2 --------------------------------------------------------
  // "Using the appropriate memory mapping described above, load the N keys
  //  into the memory system."
  //

#ifdef DBGOUT
  if (myid == 0) {
    cout << "key_array (size: " << key_array.size() << "):"
         << endl;
    for(int i=0; i < std::min<size_t>(200, key_array.size()); i++ ) {
      cout << key_array[i] << " ";
    }
    cout << endl;
  }
#endif
  // Wait for initialization of input values:
  dash::barrier();

  // PROCEDURE STEP 3 --------------------------------------------------------
  // "Begin timing."
  //
  auto ts_start = Timer::Now();

  // PROCEDURE STEP 4 --------------------------------------------------------
  // "Do, for i = 1 to I_max"
  //
  // PROCEDURE STEP 4.a ------------------------------------------------------
  // "Modify the sequence of keys by making the following two changes:
  //   - K[i]          <- i
  //   - K[i + I_max]  <- B_max - i"
  //

  // PROCEDURE STEP 4.b ------------------------------------------------------
  // "Compute the rank of each key."
  //

#if 1
  // Original implementation

  // Compute the histogram for the values in local range:
  for (int i = 0; i < key_array.lsize(); i++) {
    int value = key_array.local[i];
    local_buf[value]++; 
  }
  auto & pat = key_histo.pattern();

  // Copy local histogram values to global range:

  // Global offset of local range in key_histo:
  auto goffs = pat.global(0);
  // Copy local histogram to local range of global histogram:
  for(int l = 0; l < key_histo.lsize(); l++) { 
    key_histo.local[l] = local_buf[goffs + l];
  }

  // Copy remote histogram values to local range:

  // NOTE:
  // This is inefficient because, for a histogram size of H = MAX_KEY,
  // every histogram entry (= key rank) is communicated in a single, blocking
  // MPI_Get call from every remote unit (=> H * (nunits-1)) instead of e.g.
  // MPI_Alltoallv.
  for(int unit = 1; unit < num_units; unit++) {
    auto remote_id                = (myid + unit) % num_units;
    dash::GlobPtr<int> remote_buf = local_buffers[remote_id];
    for(int i = 0; i < key_histo.lsize(); i++) {
      // NOTE: MPI_Get for every value
      key_histo.local[i] += remote_buf[goffs + i];
    }
  }
#endif

#if 0
  // NOTE:
  // More efficient implementation using dash::transform is disabled for now
  // as it does not support optimization to MPI_Alltoall, yet.

  // Local range of key_histo to global range:
  auto local_block_viewspec = key_histo.pattern().local_block(0);
  // Global offset of first element in local range:
  auto local_block_gbegin   = local_block_viewspec[0].offset;

  DASH_ASSERT_EQ(
      local_block_viewspec[0].extent,
      MAX_KEY,
      "Unexpected extent of local block");

  DASH_LOG_DEBUG("Call dash::transform ...");
  // key_histo[i] += local_buf[i]
  dash::transform<int>(
      // Input A begin
      static_cast<int*>(local_buf),
      // Input A end
      static_cast<int*>(local_buf + MAX_KEY),
      // Input B begin
      key_histo.begin(),
      // Output = ReduceOp(A,B)
      key_histo.begin(),
      // ReduceOp
      dash::plus<int>());
  DASH_LOG_DEBUG("Finished dash::transform");
#endif

  dash::barrier();
  
  // PROCEDURE STEP 5 --------------------------------------------------------
  // "End timing."
  //
  if (myid == 0) {
    auto time_elapsed_usec = Timer::ElapsedSince(ts_start);
    auto mkeys_per_sec     = static_cast<double>(TOTAL_KEYS) /
                               time_elapsed_usec;
    cout << "MKeys/sec: " << mkeys_per_sec
         << endl;
  }

#ifdef DBGOUT
  dash::barrier();
  if (myid == 0) {
    cout << "key_histo (size: " << key_histo.size() << "):"
         << endl;
    for(int i = 0; i < key_histo.size(); i++) {
      cout << std::setw(5) << i << ": " << key_histo[i]
           << endl;
    }
  }
#endif

  dash::finalize();

  return 0;
}

/**
 * Implementation from NAS Parallel Benchmark MPI implementation of Kernel IS.
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
 * Implementation from NAS Parallel Benchmark MPI implementation of Kernel IS.
 * 
 * See NBP3.3-MPI/IS/is.c
 *
 * Create a random number sequence of total length nn residing
 * on np number of processors.  Each processor will therefore have a 
 * subsequence of length nn/np.  This routine returns that random 
 * number which is the first random number for the subsequence belonging
 * to processor rank kn, and which is used as seed for proc kn ran # gen.
 */
double find_my_seed(int    kn,     /* my processor rank, 0<=kn<=num procs */
                    int    np,     /* np = num procs                      */
                    long   nn,     /* total num of ran numbers, all procs */
                    double s,      /* Ran num seed, for ex.: 314159265.00 */
                    double a)      /* Ran num gen mult, try 1220703125.00 */
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
  return(t1);
}

