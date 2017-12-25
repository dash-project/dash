/**
 * IO benchmark for parallel HDF5 storage
 * for optimal performance run benchmark
 * on a parallel file system like GPFS
 */

#include <benchmark/benchmark.h>

#include <libdash.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <functional>

#include "../util/gbench_mpi_tweaks.h"

#ifdef DASH_ENABLE_HDF5

using std::cout;
using std::endl;
using std::setw;
using std::setprecision;

using dash::io::hdf5::InputStream;
using dash::io::hdf5::OutputStream;

using pattern_t = dash::TilePattern<2>;
using matrix_t  = dash::Matrix<double, 2, dash::default_index_t, pattern_t>;

typedef dash::util::Timer<
          dash::util::TimeMeasure::Clock
        > Timer;

typedef struct benchmark_params_t {
  long   size_base;
  int    num_it;
  bool   verify;
  std::string path;
} benchmark_params;

benchmark_params parse_args(int argc, char * argv[]);

void print_params(
  const dash::util::BenchmarkParams & bench_cfg,
  const benchmark_params            & params);

void store_matrix(benchmark::State & state, benchmark_params params);
void restore_matrix(benchmark::State & state, benchmark_params params);
void verify_data(const matrix_t & mat);

int main(int argc, char** argv)
{
  dash::init(&argc, &argv);

  // 0: real, 1: virt
  Timer::Calibrate(0);

  dash::util::BenchmarkParams bench_params("bench.11.hdf-io");
  bench_params.print_header();
  bench_params.print_pinning();

  benchmark_params params = parse_args(argc, argv);
  print_params(bench_params, params);

  // Register Benchmarks
  std::vector<benchmark::internal::Benchmark *> benchmarks;
  benchmarks.push_back(benchmark::RegisterBenchmark(
        "StoreMatrix", &store_matrix, params));
  benchmarks.push_back(benchmark::RegisterBenchmark(
        "RestoreMatrix", &restore_matrix, params));

  for(auto b : benchmarks){
    b->RangeMultiplier(2)->Range(params.size_base, params.size_base*params.num_it);
  }

  benchmark::Initialize(&argc, argv);
  dash::util::RunSpecifiedBenchmarks();

  dash::finalize();
  return 0;
}

matrix_t create_matrix(benchmark::State & state, benchmark_params params){
  typedef dash::default_size_t  extent_t;

  long size                   = state.range(0);
  extent_t    extent_cols     = size;
  extent_t    extent_rows     = size;

  auto num_elems     = extent_cols * extent_rows;
  auto mb_global     = (sizeof(double) * num_elems) / (1024 * 1024);
  auto mb_per_unit   = mb_global / dash::size();

  // check if unit has sufficient memory to store matrix
  dash::util::UnitLocality uloc;
  long sys_mem_bytes = uloc.hwinfo().system_memory_bytes;
  long unit_mem_bytes = sys_mem_bytes / (uloc.num_cores());

  if(mb_per_unit > ((unit_mem_bytes / 10) * 9)){
    state.SkipWithError("memory limit reached");
  }

  // Create Matrix
  dash::SizeSpec<2> size_spec(extent_cols, extent_rows);
  dash::DistributionSpec<2> dist_spec;
  dash::TeamSpec<2> team_spec;
  team_spec.balance_extents();
  pattern_t pattern(size_spec, dist_spec, team_spec);
  matrix_t matrix(pattern);
  return matrix;
}

void cleanup(benchmark_params params){
  if(dash::myid() == 0){
    remove(params.path.c_str());
  }
}

void verify_data(const matrix_t & mat){
  auto myid   = static_cast<matrix_t::value_type>(dash::myid());
  for(auto i=mat.lbegin(); i<mat.lend(); i++) {
    if(*i != myid){
      DASH_THROW(dash::exception::RuntimeError,
              "HDF5 data is corrupted");
    }
  }
}

void store_matrix(benchmark::State & state, benchmark_params params)
{
  using element_t = matrix_t::value_type;

  auto myid   = static_cast<element_t>(dash::myid());
  auto matrix = std::move(create_matrix(state, params));

  // Fill local block with id of unit
  std::fill(matrix.lbegin(), matrix.lend(), myid);
  dash::barrier();

  // Store Matrix
  for(auto _ : state){
    {
      OutputStream os(params.path);
      os << matrix;
      os.flush();
    }
    dash::barrier();
    state.PauseTiming();
    cleanup(params);
    state.ResumeTiming();
  }

  long long byte_total            = matrix.size() * sizeof(element_t);
  long long byte_unit             = byte_total / dash::size();

  state.SetBytesProcessed(state.iterations() * byte_total);

  state.counters["units"]         = dash::size();
  state.counters["byte.total"]    = byte_total;
  state.counters["byte.unit"]     = byte_unit;

  cleanup(params);
}

void restore_matrix(benchmark::State & state, benchmark_params params)
{
  using element_t = matrix_t::value_type;

  auto myid   = static_cast<element_t>(dash::myid());
  auto matrix = std::move(create_matrix(state, params));

  // Fill local block with id of unit
  std::fill(matrix.lbegin(), matrix.lend(), myid);
  dash::barrier();

  // Store Matrix once
  {
    OutputStream os(params.path);
    os << matrix;
  }
  dash::barrier();

  // Clear Data
  std::fill(matrix.lbegin(), matrix.lend(), -1);
  dash::barrier(); 

  // Restore Matrix
  for(auto _ : state){
    {
      InputStream is(params.path);
      is >> matrix;
      is.flush();
      state.PauseTiming();
      if(params.verify) verify_data(matrix);
      std::fill(matrix.lbegin(), matrix.lend(), -1);
      state.ResumeTiming();
    }
    dash::barrier();
  }

  long long byte_total            = matrix.size() * sizeof(element_t);
  long long byte_unit             = byte_total / dash::size();

  state.SetBytesProcessed(state.iterations() * byte_total);

  state.counters["units"]         = dash::size();
  state.counters["byte.total"]    = byte_total;
  state.counters["byte.unit"]     = byte_unit;

  cleanup(params);
}



benchmark_params parse_args(int argc, char * argv[])
{
  benchmark_params params;
  params.size_base      = 28 * 512;
  params.num_it         = 1;
  params.path           = "testfile.hdf5";
  params.verify         = false;

  for (auto i = 1; i < argc; i += 2) {
    std::string flag = argv[i];
    if (flag == "-sb") {
      params.size_base      = atoi(argv[i+1]);
    } else if(flag == "-it") {
      params.num_it         = atoi(argv[i+1]);
    } else if(flag == "-path") {
      params.path = argv[i+1];
    } else if (flag == "-verify") {
      params.verify         = true;
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
  bench_cfg.print_param("-sb",    "initial matrix size", params.size_base);
  bench_cfg.print_param("-it",    "number of iterations", params.num_it);
  bench_cfg.print_param("-path",  "path including filename", params.path);
  bench_cfg.print_param("-verify","verification",        params.verify);
  bench_cfg.print_section_end();
}

#else // DASH_ENABLE_HDF5

int main(int argc, char** argv)
{
  std::cerr << "Example requires HDF5 support" << std::endl;

  return EXIT_FAILURE;
}

#endif // DASH_ENABLE_HDF5
