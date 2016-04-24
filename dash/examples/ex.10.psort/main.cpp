/*
 * psort/main.cpp
 *
 * author(s)/ Abhishek Pasari
 */
/* @DASH_HEADER@ */

/* Redirect log output to file:
 *
 *   mpirun -n 4 ./bin/ex.10.psort.mpi 100 10 > file.log 2>&
 *
 * Scan log file for strings 'foo' and 'bar:'
 *
 *   cat file.log | grep 'foo\|bar' | vim -
 *
 * Print output of every process to separate log file:
 *
 *   mpirun -n 4 -outfile-pattern summa.%r.log
 *
 */

#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <cstddef>
#include <ctime>
#include <vector>
#include <cstdlib>
#include <libdash.h>
#include <math.h>
#include <limits>

using std::cout;
using std::endl;

typedef dash::util::Timer <
  dash::util::TimeMeasure::Clock
> Timer;

#define CLASS_A__TOTAL_KEYS  (1 << 23)
#define CLASS_A__MAX_KEY     (1 << 19)

#define CLASS_B__TOTAL_KEYS  (1 << 25)
#define CLASS_B__MAX_KEY     (1 << 21)

#define CLASS_C__TOTAL_KEYS  (1 << 27)
#define CLASS_C__MAX_KEY     (1 << 23)

#define CLASS_D__TOTAL_KEYS  (1 << 29)
#define CLASS_D__MAX_KEY     (1 << 27)

#define ITERATION   1
#define INIT_REPEAT 10 // Initial Repeat Size
typedef int key_type;

int main(int argc, char * argv[])
{
#if 0
  typedef dash::Array<
            key_type, int,
            dash::CSRPattern<1, dash::ROW_MAJOR, int> >
    array_t;
#else
  typedef dash::Array<key_type> array_t;
#endif

  size_t array_size = CLASS_A__TOTAL_KEYS;
  size_t max_key    = CLASS_A__MAX_KEY;
  size_t iteration  = ITERATION;
  size_t repeat     = INIT_REPEAT;
  int    head       = 0;

  double duration_avg_s;
  double duration_min_s = std::numeric_limits<double>::max();
  double duration_max_s = std::numeric_limits<double>::min();

  dash::init(&argc, &argv);

  Timer::Calibrate(0);

  //  For TimeMeasure::Counter:
  //  Timer::Calibrate(<CPU_BASE_FREQ_MHZ>);

  auto myid   = dash::myid();
  auto nunits = dash::size();

  if (argc == 2) {
    char class_name = argv[1][0];
    if (dash::myid() == 0) {
      cout << "class:           " << class_name << endl;
    }
    if (class_name == 'A') {
      array_size = CLASS_A__TOTAL_KEYS;
      max_key    = CLASS_A__MAX_KEY;
    } else if (class_name == 'B') {
      array_size = CLASS_B__TOTAL_KEYS;
      max_key    = CLASS_B__MAX_KEY;
    } else if (class_name == 'C') {
      array_size = CLASS_C__TOTAL_KEYS;
      max_key    = CLASS_C__MAX_KEY;
    } else if (class_name == 'D') {
      array_size = CLASS_D__TOTAL_KEYS;
      max_key    = CLASS_D__MAX_KEY;
    }
  } else if (argc > 2) {
    array_size = atoi(argv[1]);
    max_key    = atoi(argv[2]);
    if (argc >= 4) {
      repeat    = atoi(argv[3]);
    }
    if (argc >= 5) {
      iteration = atoi(argv[4]);
    }
  }

  if (dash::myid() == 0) {
    cout << "min. array size: " << array_size << endl;
    cout << "max. key value:  " << max_key    << endl;
    cout << "num repeats:     " << repeat     << endl;
    cout << "num iterations:  " << iteration  << endl;
  }

  for (size_t iter = 0; iter < iteration ; iter++) {
    duration_min_s = std::numeric_limits<double>::max();
    duration_max_s = std::numeric_limits<double>::min();

    double duration_it_s = 0;

    array_t arr(array_size);
    array_t key_histo(max_key * nunits, dash::BLOCKED);
    array_t pre_sum(key_histo.size() / nunits);

    for (size_t rep = 0; rep < repeat; rep++)
    {
      srand(time(0)+myid);
      // Initialization:
      // Writing the data into the array.
      key_type a = myid * key_histo.size() / nunits;
      for (auto lit = arr.lbegin(); lit != arr.lend(); ++lit) {
        *lit = rand() % max_key;
        a++;
      }
      for (auto lit = key_histo.lbegin(); lit != key_histo.lend(); ++lit) {
        *lit = 0;
      }
      // Wait till all process have written the data to array.
      arr.barrier();
      // Initialization done.

      auto ts_rep_start = Timer::Now();

      // Create Local Histograms.
      // Compute the histogram for the values in local range:
      for (size_t i = 0; i < arr.lsize(); i++) {
        key_type value = arr.local[i];
        ++key_histo.local[value];
      }

      if (myid != 0) {
        // Add local histogram values to result histogram at unit 0:
        // C[i] = A[i] + B[i]
        dash::transform<key_type>(key_histo.lbegin(), // A begin
                                  key_histo.lend(),   // A end
                                  key_histo.begin(),  // B begin
                                  key_histo.begin(),  // C begin
                                  dash::plus<key_type>());
      }

      dash::barrier();

      if (myid != 0) {
        // Overwrite local histogram result with result histogram from unit 0:
        dash::copy(
          key_histo.begin(),           // Begin of block at unit 0
          key_histo.begin() + max_key, // End of block at unit 0
          key_histo.lbegin());
      }

      // Wait for all units to obtain the result histogram:
      dash::barrier();

#if 0
      if (myid == 0) {
        key_type value = key_histo[0];
        pre_sum[0] = value;
        for (size_t i = 0; i < pre_sum.size(); i++) {
          pre_sum[i + 1] = pre_sum[i] + key_histo[i + 1];
        }
      }
      dash::barrier();

      /*
        As the prefix sum offset is generated in the prefix_sum array what we
        need to do is declare a new array which will be available to all the
        units and each unit will construct the array parallely into its local
        array.
        This will be done by looking at the prefix sum offset which will
        enable each unit to fill the numbers upto a particular point. Now each
        unit has to lookup through its global index and see the range into
        which it is falling and print that number.
      */

      // global start index for this unit
      auto gstart = arr.pattern().global(0);

      // find first bucket for this unit
      key_type bucket;
      for (bucket = 0; bucket < pre_sum.size(); bucket++) {
        if (pre_sum[bucket] > gstart) {
          break;
        }
      }

      // find out how many items to take out of this bucket
      key_type fill = pre_sum[bucket] - gstart;

      // fill local part of result array
      for (size_t i = 0; i < arr.lsize();) {
        for (key_type j = 0; j < fill; j++, i++ ) {
          arr.local[i] = bucket;
        }
        // move to next bucket and find out new fill size
        bucket++;
        fill = std::min<key_type>(
                 arr.lsize() - i,
                 pre_sum[bucket] - pre_sum[bucket - 1]);
      }

      dash::barrier();
#endif

      double duration_rep_s = Timer::ElapsedSince(ts_rep_start) * 1.0e-6;
      if (duration_rep_s < duration_min_s) {
        duration_min_s = duration_rep_s;
      }
      if (duration_rep_s > duration_max_s) {
        duration_max_s = duration_rep_s;
      }
      duration_it_s += duration_rep_s;
    }

    duration_avg_s = duration_it_s / repeat;

    if (myid == 0) {
      if (head != 1) {
        cout << std::setw(12) << "nunits"
             << std::setw(12) << "n"
             << std::setw(12) << "repeats"
             << std::setw(12) << "min.s"
             << std::setw(12) << "avg.s"
             << std::setw(12) << "max.s"
             << std::setw(12) << "m.op/s"
             << std::setw(12) << "m.op/s/p"
             << std::endl;
        head = 1;
      }
      double mop_total_per_s = (array_size / duration_avg_s) * 1.0e-6;
      double mop_proc_per_s  = mop_total_per_s / nunits;
      cout << std::setw(12) << nunits
           << std::setw(12) << array_size
           << std::setw(12) << repeat
           << std::setw(12) << std::fixed << std::setprecision(2) << duration_min_s
           << std::setw(12) << std::fixed << std::setprecision(2) << duration_avg_s
           << std::setw(12) << std::fixed << std::setprecision(2) << duration_max_s
           << std::setw(12) << std::fixed << std::setprecision(2) << mop_total_per_s
           << std::setw(12) << std::fixed << std::setprecision(2) << mop_proc_per_s
           << std::endl;
    }

    repeat     /= 2;
    array_size *= 10;

    if (repeat <= 0) {
      repeat = 1;
    }
  }
  dash::finalize();
}

