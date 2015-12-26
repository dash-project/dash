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
#include <unistd.h>

using std::cout;
using std::endl;
using std::vector;

typedef int     ElementType;
typedef int64_t index_t;
typedef dash::Array<
          ElementType,
          index_t,
          dash::CSRPattern<1, dash::ROW_MAJOR, index_t>
        > Array_t;
typedef dash::util::Timer<
          dash::util::TimeMeasure::Clock
        > Timer;

#define CPU_FREQ 2501

#define DASH_PRINT_MASTER(expr) \
  do { \
    if (dash::myid() == 0) { \
      std::cout << expr << std::endl; \
    } \
  } while(0)

double copy_all_local(size_t size, bool parallel);
double copy_partially_local(size_t size, bool parallel);
double copy_no_local(size_t size, bool parallel);

int main(int argc, char** argv)
{
  double kps_al;
  double kps_pl;
  double kps_nl;
  double mem_rank;
  double mem_glob;
  size_t size;

  dash::init(&argc, &argv);
  Timer::Calibrate(0);

  if (dash::myid() == 0) {
    cout << "Local copy benchmark"
         << endl;
    cout << "Timer: " << Timer::TimerName()
         << endl;
    cout << std::setw(8)  << "size";
    cout << std::setw(14) << "all local";
    cout << std::setw(14) << "part. loc.";
    cout << std::setw(14) << "no local";
    cout << std::setw(8)  << " ";
    cout << std::setw(14) << "mem/rank";
    cout << std::setw(14) << "mem/glob";
    cout << endl;
  }

  for (int size_exp = 3; size_exp < 10; ++size_exp)
  {
    size      = (size_t)std::pow((double)10, (double)size_exp);

    DASH_LOG_DEBUG("main", "START copy_all_local", "size: 10^", size_exp);
    kps_al    = copy_all_local(size, false);
    dash::barrier();
    sleep(1);
    DASH_LOG_DEBUG("main", "DONE  copy_all_local", "size: 10^", size_exp);

    DASH_LOG_DEBUG("main", "START copy_partially_local", "size: 10^", size_exp);
    kps_pl    = copy_partially_local(size, false);
    dash::barrier();
    sleep(1);
    DASH_LOG_DEBUG("main", "DONE  copy_partially_local", "size: 10^", size_exp);

    DASH_LOG_DEBUG("main", "START copy_no_local", "size: 10^", size_exp);
    kps_nl    = copy_no_local(size,false);
    dash::barrier();
    sleep(1);
    DASH_LOG_DEBUG("main", "DONE  copy_no_local", "size: 10^", size_exp);

    mem_glob  = ((sizeof(ElementType) * size) / 1024) / 1024;
    mem_rank  = mem_glob / dash::size();

    DASH_PRINT_MASTER("10^" << std::setw(5) << size_exp
                      << std::setprecision(5) << std::setw(14) << kps_al
                      << std::setprecision(5) << std::setw(14) << kps_pl
                      << std::setprecision(5) << std::setw(14) << kps_nl
                      << std::setw(8)  << "MKeys/s"
                      << std::setw(10) << mem_rank << " MiB"
                      << std::setw(10) << mem_glob << " MiB");
  }

  DASH_PRINT_MASTER("Benchmark finished");

  dash::finalize();
  return 0;
}

double copy_all_local(size_t size, bool parallel)
{
  Array_t global_array(size, dash::BLOCKED);
  ElementType *local_array = nullptr;

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
    local_array = (ElementType*) malloc(sizeof(ElementType)*local_size*2);
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

double copy_partially_local(size_t size, bool parallel)
{
  Array_t global_array(size, dash::BLOCKED);
  ElementType *local_array = nullptr;
  auto timer_start  = Timer::Now();
  double elapsed;  

  DASH_LOG_DEBUG("copy_partially_local()",
                 "size:", size);

  if(dash::myid() == 0 && !parallel){
    local_array = (ElementType*) malloc(sizeof(ElementType)*size*2);
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

double copy_no_local(size_t size, bool parallel)
{
  Array_t global_array(size, dash::BLOCKED);
  auto    l_start_idx    = global_array.pattern().lbegin();
  auto    l_end_idx      = global_array.pattern().lend();
  size_t  local_size     = l_end_idx - l_start_idx;

  size_t  num_copy_elem  = size - local_size;
  auto    timer_start    = Timer::Now();
  double  elapsed;

  DASH_LOG_DEBUG("copy_no_local()",
                 "size:",   size,
                 "l_idcs:", l_start_idx, "-", l_end_idx,
                 "l_size:", local_size,
                 "n_copy:", num_copy_elem);

  for (auto l = 0; l < global_array.lsize(); ++l) {
    global_array.local[l] = ((dash::myid() + 1) * 1000) + l;
  }
  dash::barrier();

  if(dash::myid() == 0 && !parallel) {
    // Allocate target array on heap as it might get too large for the stack:
    ElementType * local_array = new ElementType[num_copy_elem];
    // Pointer to first element in destination range:
    auto dest_first = local_array;
    // Pointer past last element in destination range:
    auto dest_last  = local_array;
    // Start timer:
    timer_start = Timer::Now();
    // Copy elements in front of local range:
    DASH_LOG_DEBUG(
      "copy_no_local",
      "Copying from global range",
      0, "-", l_start_idx);
    dest_first = dash::copy(
                   global_array.begin(),
                   global_array.begin() + l_start_idx,
                   dest_first);
    // Copy elements after local range:
    DASH_LOG_DEBUG(
      "copy_no_local",
      "Copying from global range",
      l_end_idx, "-", global_array.size());
    dest_last  = dash::copy(
                   global_array.begin() + l_end_idx,
                   global_array.end(),
                   dest_first);
    elapsed = Timer::ElapsedSince(timer_start);

    DASH_ASSERT_EQ(
      local_array + num_copy_elem,
      dest_last,
      "Unexpected output pointer from dash::copy");
    if (local_array != nullptr) {
      delete[] local_array;
    }
  }

  DASH_LOG_DEBUG(
      "copy_no_local",
      "Waiting for completion of copy operation");
  dash::barrier();

  return (1.0e-6 * (num_copy_elem / elapsed));
}
