/**
 * Local copy benchmark for various containers
 *
 * author(s): Felix Moessbauer, LMU Munich */
/* @DASH_HEADER@ */

#include <libdash.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <math.h>

using std::cout;
using std::endl;
using std::vector;

typedef int64_t index_t;

typedef dash::util::Timer<
          dash::util::TimeMeasure::Clock
        > Timer;

#ifndef TYPE
#define TYPE int
#endif

#define CPU_FREQ 2501

#define DASH_PRINT_MASTER(expr) \
  do { \
    if (dash::myid() == 0) { \
      std::cout << expr << std::endl; \
    } \
  } while(0)

double copy_all_local(unsigned long size, bool parallel);
double copy_partially_local(unsigned long size, bool parallel);
double copy_no_local(unsigned long size, bool parallel);

int main(int argc, char** argv)
{
  double kps_al;
  double kps_pl;
  double kps_nl;
  double mem_rank;
  double mem_glob;
  long   size;

  dash::init(&argc, &argv);
  Timer::Calibrate(0);

  if (dash::myid() == 0) {
    cout << "Timer: " << Timer::TimerName()
         << endl;
    cout << "Local copy benchmark"
         << endl
         << "size\t all local\t part. loc.\t no local\t "
            "unit\t mem/rank\t mem/glob"
         << endl;
  }

  for (int i=3; i<10; i++)
  {
    size      = (unsigned long) std::pow((double) 10,(double) i);

    DASH_LOG_DEBUG("==== START: i:", i, "copy_all_local");
    kps_al    = copy_all_local(size,false);
    dash::barrier();
    DASH_LOG_DEBUG("==== FINISHED: i:", i, "copy_all_local");

    DASH_LOG_DEBUG("==== START: i:", i, "copy_partially_local");
    kps_pl    = copy_partially_local(size,false);
    dash::barrier();
    DASH_LOG_DEBUG("==== FINISHED: i:", i, "copy_partially_local");

    DASH_LOG_DEBUG("==== START: i:", i, "copy_partially_local");
    kps_nl    = copy_no_local(size,false);
    dash::barrier();
    DASH_LOG_DEBUG("==== FINISHED: i:", i, "copy_partially_local");

    mem_glob  = ((sizeof(TYPE) * size) / 1024) / 1024;
    mem_rank  = mem_glob / dash::size();

    DASH_PRINT_MASTER("10^" << i << "\t"
                      << std::setprecision(5) << std::setw(5)
                      << kps_al << "\t\t"
                      << std::setprecision(5) << std::setw(5)
                      << kps_pl << "\t\t"
                      << std::setprecision(5) << std::setw(5)
                      << kps_nl << "\t\t"
                      << "MKeys/s\t"
                      << std::setw(5)
                      << mem_rank << " MiB\t"
                      << std::setw(5)
                      << mem_glob << " MiB\t");
  }

  DASH_PRINT_MASTER("Benchmark finished");

  dash::finalize();
  return 0;
}

double copy_all_local(unsigned long size, bool parallel)
{
  dash::Array<TYPE, index_t> global_array(size, dash::BLOCKED);
  TYPE *local_array = nullptr;

  auto l_start_idx  = global_array.pattern().lbegin();
  auto l_end_idx    = global_array.pattern().lend();
  size_t local_size = l_end_idx - l_start_idx;

  auto timer_start  = Timer::Now();
  double elapsed;  

  DASH_LOG_DEBUG("copy_all_local()",
                 "size:",   size,
                 "l_idcs:", l_start_idx, "-", l_end_idx,
                 "l_size:", local_size);

  if(dash::myid() == 0 && !parallel){
    local_array = (TYPE*) malloc(sizeof(TYPE)*local_size*2);
  }

  dash::barrier();

  if(dash::myid() == 0 && !parallel){
    timer_start = Timer::Now();
    dash::copy(global_array.begin() + l_start_idx,
               global_array.begin() + l_end_idx,
               local_array);
    elapsed = Timer::ElapsedSince(timer_start);
    if (local_array != nullptr) {
      free(local_array);
    }
  }

  dash::barrier();
  return (1.0e-6 * (local_size / elapsed));
}

double copy_partially_local(unsigned long size, bool parallel)
{
  dash::Array<TYPE, index_t> global_array(size, dash::BLOCKED);
  TYPE *local_array = nullptr;
  auto timer_start  = Timer::Now();
  double elapsed;  

  DASH_LOG_DEBUG("copy_partially_local()",
                 "size:", size);

  if(dash::myid() == 0 && !parallel){
    local_array = (TYPE*) malloc(sizeof(TYPE)*size*2);
  }

  dash::barrier();

  if(dash::myid() == 0 && !parallel){
    timer_start = Timer::Now();
    dash::copy(global_array.begin(),
               global_array.end(),
               local_array);
    elapsed = Timer::ElapsedSince(timer_start);
    if (local_array != nullptr) {
      free(local_array);
    }
  }

  dash::barrier();
  return (1.0e-6 * (size / elapsed));
}

double copy_no_local(unsigned long size, bool parallel)
{
  dash::Array<TYPE, index_t> global_array(size, dash::BLOCKED);
  TYPE *local_array     = nullptr;
  auto l_start_idx      = global_array.pattern().lbegin();
  auto l_end_idx        = global_array.pattern().lend();
  size_t local_size     = l_end_idx - l_start_idx;

  size_t num_copy_elem  = size - local_size;
  auto timer_start      = Timer::Now();
  double elapsed;

  DASH_LOG_DEBUG("copy_no_local()",
                 "size:",   size,
                 "l_idcs:", l_start_idx, "-", l_end_idx,
                 "l_size:", local_size,
                 "n_copy:", num_copy_elem);

  if(dash::myid() == 0 && !parallel){
    local_array = (TYPE*) malloc(sizeof(TYPE)*local_size*2);
  }

  dash::barrier();

  if(dash::myid() == 0 && !parallel){
    timer_start = Timer::Now();
    // Does not work
    dash::copy(global_array.begin() + l_end_idx,
               global_array.end(),
               local_array);
    elapsed = Timer::ElapsedSince(timer_start);

    if (local_array != nullptr) {
      free(local_array);
    }
  }

  dash::barrier();
  return (1.0e-6 * (num_copy_elem / elapsed));
}
