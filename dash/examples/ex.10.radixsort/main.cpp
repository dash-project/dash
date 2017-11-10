/**
 * \example ex.10.radixsort/main.cpp
 * Example demonstrating radixsort on 
 * DASH global data structures.
 */
#include <libdash.h>

#include <vector>
#include <map>
#include <limits>
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <cstddef>
#include <ctime>
#include <cstdlib>
#include <math.h>

#include <mpi.h>


using std::cout;
using std::cerr;
using std::endl;

#define MAX_KEY       524288
#define ARRAY_SIZE    8388608
#define ITERATION     1
#define INIT_REPEAT   1

// number of bits for integer
#define bits_init     32
// group of bits for each scan
#define group_oneword 8
// number of passes
#define NUM_PASSES    (bits_init / group_oneword)
#define NUM_BUCKETS   (1 << group_oneword)

typedef long long index_t;
typedef int key_type;
typedef dash::util::Timer <
dash::util::TimeMeasure::Clock
> Timer;

/**
 * Compute j bits which appear k bits from the right in x Ex.
 * to obtain rightmost bit of x call bits(x, 0, 1)
 */
unsigned bits(unsigned x, int k, int j)
{
  return (x >> k) & ~(~0 << j);
}

typedef struct radix_sort_result_t {
  key_type * array;
  int        size;
} radix_sort_result;

radix_sort_result radix_sort(
  int  * local_a,
  std::map<int, std::vector<key_type> > & buckets,
  int    nunits,
  int    myid,
  int    arr_lsize)
{
  radix_sort_result result;
  result.array = nullptr;
  result.size  = 0;

//  cout << "unit " << dash::myid() << " radix_sort--1" << endl;

  try {

    auto rows = NUM_BUCKETS;
    auto cols = nunits;

    // mpirun -n 1 ./bin/ex.06.pattern-block-visualizer.mpi
    //              -n 100 32 -u 32 1 -t 0 1 -s block
    typedef dash::Pattern<2> pattern_t;
    dash::Matrix<int, 2, pattern_t::index_type, pattern_t> count(
      dash::SizeSpec<2>(rows, cols),
      dash::DistributionSpec<2>(dash::NONE, dash::CYCLIC));
    int l_B = NUM_BUCKETS / nunits;

    std::vector< std::vector<key_type> > pre_sum;

    for (int lb = 0; lb < l_B; ++lb) {
      std::vector<key_type> lb_pref_sum;
      for (int i = 0; i < nunits; ++i) {
        lb_pref_sum.push_back(0);
      }
      pre_sum.push_back(lb_pref_sum);
    }

//  cout << "unit " << dash::myid() << " radix_sort--2" << endl;

    MPI_Request req;
    MPI_Status  stat;

    for (auto pass = 0; pass < NUM_PASSES; pass++) {
//    cout << "unit " << dash::myid() << " radix_sort--pass--" << pass << "-"
//         << "begin" << endl;

      int * lit  = count.lbegin();
      int * lend = count.lend();

      dash::barrier();

//    cout << "unit " << dash::myid() << " radix_sort--pass--" << pass << "-"
//         << "step 1" << endl;

      for (size_t it = 0; lit != lend; ++it, ++lit) {
        if (it < count.local_size()) {
          *lit = 0;
        }
      }
//    cout << "unit " << dash::myid() << " radix_sort--pass--" << pass << "-"
//         << "step 2 -- arr_lsize = " << arr_lsize << endl;

      for (auto i = 0; i < arr_lsize; i++) {
        unsigned int idx  = bits(local_a[i],
                                 pass * group_oneword,
                                 group_oneword);
        count[idx][myid] += 1;
#if __COMMENT
        cout << " Rank: " << std::setw(5) << myid
             << " Pass: " << std::setw(5) << pass
             << " idx: "  << std::setw(5) << idx
             << " Array Local[i]: " << std::setw(5) << local_a[i]
             << " Count-idx-rank: " << std::setw(5) << (int)count[idx][myid]
             << " \n";
#endif
//      cout << "unit " << dash::myid() << " radix_sort--pass-" << pass << "-"
//           << "add_item(idx:" << idx << " item:" << local_a[i] << ")"
//           << endl;
        buckets[idx].push_back(local_a[i]);
      }

//    cout << "unit " << dash::myid() << " radix_sort--pass--" << pass << "-"
//         << "step 3 pre-barrier" << endl;

      dash::barrier();

//    cout << "unit " << dash::myid() << " radix_sort--pass--" << pass << "-"
//         << "step 3 post-barrier" << endl;

      key_type new_size = 0;
      for (auto j = 0; j < l_B; j++) {
        key_type idx = j + (myid * l_B);
        for (auto p = 0; p < nunits; p++) {
          pre_sum[j][p] = new_size;
          new_size     += count[idx][p];
        }
      }

#if __ORIGINAL__IMPL
      if (new_size > arr_lsize) {
        int * temp = new int[new_size];
        for (int la = 0; la < arr_lsize; ++la) {
          temp[la] = local_a[la];
        }
        delete[] local_a;
        local_a = temp;
      }
#endif

      dash::barrier();

//    cout << "unit " << dash::myid() << " radix_sort--pass--" << pass << "-"
//         << "step 4" << endl;

      for (auto j = 0; j < NUM_BUCKETS; j++) {
        // determine which process this buckets belongs to
        key_type p   = j / l_B;
        // transpose to that process local bucket index
        key_type p_j = j % l_B;

        auto buckets_j = buckets[j];

        if (p != myid && buckets_j.size() > 0) {
          cout << "unit " << dash::myid()
               << " radix_sort--MPI_Send: send_count=" << buckets_j.size()
               << endl;
          MPI_Isend(
            &buckets_j[0],
            buckets_j.size(),
            MPI_INT,
            p,
            p_j,
            MPI_COMM_WORLD,
            &req);
        }
      }

//    cout << "unit " << dash::myid() << " radix_sort--pass--" << pass << "-"
//         << "step 5" << endl;

      for (auto j = 0; j < l_B; j++) {
        key_type idx = j + myid * l_B;

        for (auto p = 0; p < nunits; p++) {
          key_type b_count = count[idx][p];

          if (b_count > 0) {
            key_type * dest = &local_a[pre_sum[j][p]];

            if (myid != p) {
              cout << "unit " << dash::myid()
                   << " radix_sort--MPI_Recv: recv_count=" << b_count
                   << endl;
              MPI_Recv(
                dest,
                b_count,
                MPI_INT,
                p,
                j,
                MPI_COMM_WORLD,
                &stat);
            }	else {
              std::copy(buckets[idx].begin(),
                        buckets[idx].end(),
                        dest);
            }
          }
        }
      }
      arr_lsize = new_size;

      dash::barrier();
//    cout << "unit " << dash::myid() << " radix_sort--pass--" << pass << "-"
//         << "end -- arr_lsize = " << arr_lsize << endl;
    } // pass ended

    dash::barrier();

//  cout << "unit " << dash::myid() << " radix_sort--3" << endl;

    result.array = local_a;
    result.size  = arr_lsize;
    return result;
  }
  catch (std::exception & excep)
  {
    cerr << "Exception: " << excep.what()
         << endl;
    dash__print_stacktrace();
    return result;
  }
}


int main(int argc, char * argv[])
{
  size_t array_size = ARRAY_SIZE;
  size_t max_key    = MAX_KEY;
  size_t iteration  = ITERATION;
  size_t repeat     = INIT_REPEAT;
  int    head       = 0;

  double duration_avg_s;
  double duration_min_s = std::numeric_limits<double>::max();
  double duration_max_s = std::numeric_limits<double>::min();

  typedef dash::Pattern<2> pattern_t;

  dash::init(&argc, &argv);
  Timer::Calibrate(0);

  auto myid    = dash::myid();
  auto size    = dash::size();

  if (argc >= 2) {
    array_size = atoi(argv[1]);
  }
  if (argc >= 3) {
    max_key    = atoi(argv[2]);
  }
  if (argc >= 4) {
    repeat     = atoi(argv[3]);
  }
  if (argc >= 5) {
    iteration  = atoi(argv[4]);
  }

  if (dash::myid() == 0) {
    cout << "min. array size: " << array_size << endl;
    cout << "max. key value:  " << max_key    << endl;
    cout << "num repeats:     " << repeat     << endl;
    cout << "num iterations:  " << iteration  << endl;
  }

  if ((array_size % size) != 0) {
    if (myid == 0) {
      cout << "Please enter an array size which is divisible ny number of "
           << "units.\n";
    }
    dash::finalize();
    return 0;
  }

  if ((size % 2) != 0) {
    if (myid == 0) {
      cout << "Please enter an even size of processes \n";
    }
    dash::finalize();
    return 0;
  }

  key_type rem_buckets = NUM_BUCKETS % size;

  if (rem_buckets > 0) {
    if (myid == 0) {
      cout << "Number of buckets and array size must be divisible by number "
           << "of processors"
           << std::endl;
    }
    dash::finalize();
    return 0;
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

    dash::Array<key_type> arr(array_size);
    dash::barrier();

    for (size_t rep = 0; rep < repeat; rep++) {
      // Initialization:
      // Writing the data into the array.
      srand(time(0) + myid);

      int v = 0;
      for (auto lit = arr.lbegin(); lit != arr.lend(); ++lit) {
//      *lit = (rand() % max_key) + 1;
        *lit = (dash::myid() * 100 + v + (v % 2) * 512) % max_key;
        ++v;
      }

      arr.barrier();

      cout << "unit " << dash::myid() << " "
           << "local array size: " << arr.lend() - arr.lbegin() << " "
           << "local pattern size: " << arr.pattern().local_size()
           << endl;

#if __PRINT_ARRAY_VALUES__
      for (auto unit_id = 0; unit_id < dash::size(); ++unit_id) {
        if (unit_id == dash::myid()) {
          int l = 0;
          for (auto lit = arr.lbegin(); lit != arr.lend(); ++lit) {
            cout << "unit " << unit_id << ": array[" << l++
                 << "] = " << *lit << endl;
          }
          sleep(1);
        }
        dash::barrier();
      }
#endif

      key_type arr_lsize = arr.lsize();

#if __ORIGINAL__IMPL
      key_type * local_arr = new key_type[arr_lsize];
#else
      key_type * local_arr = new key_type[array_size];
#endif

      for (size_t lit = 0; lit < arr.lsize(); lit++) {
        local_arr[lit] = arr.local[lit];
      }

      std::map<int, std::vector<key_type> > buckets;

      dash::barrier();

      auto ts_rep_start = Timer::Now();
      auto result = radix_sort(local_arr, buckets, size, myid, arr_lsize);
      local_arr   = result.array;
      arr_lsize   = result.size;
      double duration_rep_s = Timer::ElapsedSince(ts_rep_start) * 1.0e-6;
/*
      //TODO: Fill all the values in global array
      //      and check if it is sorted or not.
      proc_n[myid] = arr_lsize;
      if(myid == 0){
        auto iter_p = 0;
        for(auto j=0; j<proc_n[iter_p];)
      }
*/
      dash::barrier();

      if (dash::myid() == 0) {
        if (arr_lsize <= 0) {
          DASH_THROW(dash::exception::RuntimeError,
                     "local result array at unit 0 has size 0");
        }
        for (auto j = 0; j < arr_lsize; j++) {
          cout << std::setw(5) << myid << std::setw(5) << local_arr[j]
               << endl;
        }
      }
      dash::barrier();

      if (duration_rep_s < duration_min_s) {
        duration_min_s = duration_rep_s;
      }

      if (duration_rep_s > duration_max_s) {
        duration_max_s = duration_rep_s;
      }
      duration_it_s += duration_rep_s;

      dash::barrier();

      if (local_arr != nullptr) {
        delete[] local_arr;
      }
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
             << std::endl;
        head = 1;
      }
      cout << std::setw(12) << size
           << std::setw(12) << array_size
           << std::setw(12) << repeat
           << std::setw(12) << std::setprecision(3) << duration_min_s
           << std::setw(12) << std::setprecision(3) << duration_avg_s
           << std::setw(12) << std::setprecision(3) << duration_max_s
           << std::endl;
    }

    repeat     /= 4;
    array_size *= 4;

    if (repeat <= 0) {
      repeat = 1;
    }

  }
  dash::finalize();

  return EXIT_SUCCESS;
}
