/**
 *
 * Random Access (GUPS) benchmark
 *
 * Based on the UPC++ version of the same bechmark
 *
 */

#ifdef DASH_ENABLE_IPM
#include <mpi.h>
#endif

#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#include <iostream>

#include <libdash.h>

#ifndef N
#define N (25)
#endif

#define TableSize (1ULL<<N)
#define NUPDATE   (4ULL * TableSize)

#define POLY      0x0000000000000007ULL
#define PERIOD    1317624576693539401LL

typedef uint64_t
  value_t;
typedef int64_t
  index_t;
typedef dash::CSRPattern<1, dash::ROW_MAJOR, index_t>
  pattern_t;
typedef typename pattern_t::size_type
  extent_t;
typedef dash::util::Timer<dash::util::TimeMeasure::Clock>
  Timer;

typedef struct benchmark_params_t {
  size_t size_base;
  size_t num_updates;
  size_t rep_base;
  bool   verify;
} benchmark_params;

using std::cout;
using std::endl;
using std::setw;

dash::Array<value_t, index_t, pattern_t> Table;


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

void RandomAccessUpdate(const benchmark_params & params)
{
  uint64_t i;
  uint64_t ran = starts(params.num_updates / dash::size() * dash::myid());
  auto     table_size = params.size_base;

  for (i = dash::myid(); i < params.num_updates; i += dash::size()) {
    ran           = (ran << 1) ^ (((int64_t) ran < 0) ? POLY : 0);
    int64_t g_idx = static_cast<int64_t>(ran & (table_size-1));
    Table[g_idx] ^= ran;
  }
}

uint64_t RandomAccessVerify(const benchmark_params & params)
{
  uint64_t i, localerrors, errors;
  auto     table_size = params.size_base;
  localerrors = 0;
  for (i = dash::myid(); i < table_size; i += dash::size()) {
    if (Table[i] != i) {
      localerrors++;
    }
  }
  errors = localerrors;
  return errors;
}

void perform_test(const benchmark_params & params);

benchmark_params parse_args(int argc, char * argv[]);

void print_params(
  const dash::util::BenchmarkParams & bench_cfg,
  const benchmark_params            & params);


int main(int argc, char **argv)
{
  DASH_LOG_DEBUG("bench.gups", "main()");

  dash::init(&argc, &argv);
#ifdef DASH_ENABLE_IPM
  MPI_Pcontrol(0, "off");
#endif

  Timer::Calibrate(0);

  dash::util::BenchmarkParams bench_cfg("bench.03.gups");

  bench_cfg.set_output_width(72);
  bench_cfg.print_header();
  bench_cfg.print_pinning();

  benchmark_params params = parse_args(argc, argv);
  print_params(bench_cfg, params);

  perform_test(params);

  dash::finalize();

  return 0;
}

void perform_test(const benchmark_params & params)
{
  Timer::timestamp_t ts_start;
  double duration_us;
  auto array_size    = params.size_base;
  auto num_updates   = params.num_updates;
  auto ts_init_start = Timer::Now();

  DASH_LOG_DEBUG("bench.gups", "Table.allocate()");
  Table.allocate(params.size_base, dash::BLOCKED);

  if(dash::myid() == 0) {
    auto   nunits        = dash::size();
    double bytes_total   = (double)(array_size * sizeof(value_t));
    double mb_total      = bytes_total / 1024 / 1024;
    double mb_unit       = mb_total / nunits;
    double updates_m     = (double)(num_updates * 1.0e-9);
#ifdef DASH_MPI_IMPL_ID
    std::string mpi_impl = dash__toxstr(DASH_MPI_IMPL_ID);
#else
    std::string mpi_impl = "-";
#endif
    cout << setw(6)  << "units"     << ","
         << setw(12) << "size"      << ","
         << setw(9)  << "mpi.impl"  << ","
         << setw(12) << "mb.total"  << ","
         << setw(12) << "mb.unit"   << ","
         << setw(12) << "updates.m" << ","
         << setw(9)  << "init.s"    << ","
         << setw(9)  << "time.s"    << ","
         << setw(9)  << "lat.us"    << ","
         << setw(9)  << "gups"      << ","
         << setw(9)  << "verified"  << ","
         << setw(9)  << "errors"
         << endl;
    cout << setw(6)  << nunits           << ","
         << setw(12) << params.size_base << ","
         << setw(9)  << mpi_impl         << ","
         << setw(12) << std::fixed << std::setprecision(2) << mb_total  << ","
         << setw(12) << std::fixed << std::setprecision(2) << mb_unit   << ","
         << setw(12) << std::fixed << std::setprecision(2) << updates_m << ","
         << std::flush;
  }

  // Initialize array values:
#if 0
  if(dash::myid() == 0) {
    for (uint64_t i = dash::myid(); i < TableSize; ++i) {
      Table[i] = i;
    }
  }
#else
  // Global offset of local array region:
  auto l_begin_glob_offset = Table.pattern().global(0);
  for (uint64_t i = 0; i < Table.local.size(); ++i) {
    Table.local[i] = l_begin_glob_offset + i;
  }
#endif
  dash::barrier();

  if(dash::myid() == 0) {
    double time_init_s = Timer::ElapsedSince(ts_init_start) * 1.0e-6;

    cout << setw(9) << std::fixed << std::setprecision(4) << time_init_s << ","
         << std::flush;
  }

  // Perform random access test:
#ifdef DASH_ENABLE_IPM
  MPI_Pcontrol(0, "on");
  MPI_Pcontrol(0, "clear");
#endif
  ts_start    = Timer::Now();
  RandomAccessUpdate(params);
  dash::barrier();
  duration_us = Timer::ElapsedSince(ts_start);
#ifdef DASH_ENABLE_IPM
  MPI_Pcontrol(0, "off");
#endif

  if(dash::myid() == 0) {
    double gups       = (static_cast<double>(params.num_updates) / 1000.0f)
                        / duration_us;
    double latency    = duration_us * dash::size() / num_updates;
    double duration_s = (duration_us * 1.0e-6);
    cout << setw(9) << std::fixed << std::setprecision(4) << duration_s << ","
         << setw(9) << std::fixed << std::setprecision(4) << latency    << ","
         << setw(9) << std::fixed << std::setprecision(5) << gups       << ","
         << std::flush;
  }

  // Verification:
  if (params.verify) {
    // do it again
    RandomAccessUpdate(params);
    dash::barrier();
    uint64_t errors = RandomAccessVerify(params);
    if (dash::myid() == 0) {
      if ((double)errors/num_updates < 0.01) {
        cout << setw(9) << "passed" << ","
             << setw(9) << errors
             << endl;
      } else {
        cout << setw(9) << "failed" << ","
             << setw(9) << errors
             << endl;
      }
    }
  } else {
    if (dash::myid() == 0) {
      cout << setw(9) << "no" << ","
           << setw(9) << 0
           << endl;
    }
  }
}

benchmark_params parse_args(int argc, char * argv[])
{
  benchmark_params params;
  params.size_base   = TableSize;
  params.num_updates = NUPDATE;
  params.rep_base    = 1;
  params.verify      = false;

  for (auto i = 1; i < argc; i += 2) {
    std::string flag = argv[i];
    if (flag == "-sb") {
      params.size_base = atoi(argv[i+1]);
    } else if (flag == "-rb") {
      params.rep_base  = atoi(argv[i+1]);
    } else if (flag == "-verify") {
      params.verify    = true;
      --i;
    }
  }
  return params;
}

void print_params(
  const dash::util::BenchmarkParams & bench_cfg,
  const benchmark_params            & params)
{
  if (dash::myid() != 0) {
    return;
  }

  bench_cfg.print_section_start("Runtime arguments");
  bench_cfg.print_param("-sb",     "size base",    params.size_base);
  bench_cfg.print_param("-rb",     "rep. base",    params.rep_base);
  bench_cfg.print_param("-verify", "verification", params.verify);
  bench_cfg.print_section_end();
}

