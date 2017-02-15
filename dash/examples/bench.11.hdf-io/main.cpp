/**
 * IO benchmark for parallel HDF5 storage
 * for optimal performance run benchmark
 * on a parallel file system like GPFS
 */

#include <libdash.h>
#include <iostream>
#include <iomanip>
#include <string>

#ifdef DASH_ENABLE_HDF5

using std::cout;
using std::endl;
using std::setw;
using std::setprecision;

using dash::io::hdf5::InputStream;
using dash::io::hdf5::OutputStream;


typedef dash::util::Timer<
          dash::util::TimeMeasure::Clock
        > Timer;

typedef typename dash::util::BenchmarkParams::config_params_type
  bench_cfg_params;

typedef struct benchmark_params_t {
  long   size_base;
  int    num_it;
  bool   verify;
  std::string path;
} benchmark_params;

typedef struct measurement_t {
  double mb_per_unit;
  double mb_global;
  double time_init_s;
  double time_write_s;
  double time_read_s;
  double time_total_s;
  double mb_per_s_read;
  double mb_per_s_write;
} measurement;

void print_measurement_header();
void print_measurement_record(
  const bench_cfg_params & cfg_params,
  measurement              measurement,
  const benchmark_params & params);

benchmark_params parse_args(int argc, char * argv[]);

void print_params(
  const dash::util::BenchmarkParams & bench_cfg,
  const benchmark_params            & params);


measurement store_matrix(
              long size,
              benchmark_params params);

int main(int argc, char** argv)
{
  dash::init(&argc, &argv);

  // 0: real, 1: virt
  Timer::Calibrate(0);

  measurement res;

  dash::util::BenchmarkParams bench_params("bench.11.hdf-io");
  bench_params.print_header();
  bench_params.print_pinning();

  benchmark_params params = parse_args(argc, argv);
  auto bench_cfg = bench_params.config();

  print_params(bench_params, params);
  print_measurement_header();

  for(int i=0;i<params.num_it;++i){
    res = store_matrix(params.size_base*(i+1), params);
    print_measurement_record(bench_cfg, res, params);
  }

  if( dash::myid()==0 ) {
    cout << "Benchmark finished" << endl;
  }

  dash::finalize();
  return 0;
}

measurement store_matrix(long size, benchmark_params params)
{
#ifdef DASH_ENABLE_HDF5
  typedef dash::default_index_t index_t;
  typedef dash::default_size_t  extent_t;

  measurement mes;
  auto        myid            = dash::myid();
  extent_t    extent_cols     = size;
  extent_t    extent_rows     = size;
  auto        ts_start_total  = Timer::Now();
  auto        ts_start_create = Timer::Now();

  auto num_elems     = extent_cols * extent_rows;
  mes.mb_global      = (sizeof(double) * num_elems) / (1024 * 1024);
  mes.mb_per_unit    = mes.mb_global / dash::size();

  // check if unit has sufficient memory to store matrix
  dash::util::UnitLocality uloc;
  long sys_mem_bytes = uloc.hwinfo().system_memory_bytes;
  long unit_mem_bytes = sys_mem_bytes / (uloc.num_cores());
 
  if(mes.mb_per_unit > ((unit_mem_bytes / 10) * 9)){
    mes.time_total_s   = -1;
    mes.mb_per_s_read  = -1;
    mes.mb_per_s_write = -1;
    return mes;
  }

  // Create Matrix
  dash::SizeSpec<2> size_spec(extent_cols, extent_rows);
  dash::TeamSpec<2> team_spec;
  team_spec.balance_extents();
  auto pattern = dash::make_pattern <
                   dash::summa_pattern_partitioning_constraints,
                   dash::summa_pattern_mapping_constraints,
                   dash::summa_pattern_layout_constraints >(
                       size_spec,
                       team_spec);

  typedef decltype(pattern)                           pattern_t;
  typedef dash::Matrix<double, 2, index_t, pattern_t> matrix_t;

  matrix_t matrix_a(pattern);
  // Fill local block with id of unit
  std::fill(matrix_a.lbegin(), matrix_a.lend(), myid);
  dash::barrier();

  mes.time_init_s = 1e-6 * Timer::ElapsedSince(ts_start_create);

  // Store Matrix
  auto ts_start_write    = Timer::Now();

	OutputStream os(params.path);
	os << matrix_a;

  dash::barrier();
  mes.time_write_s = 1e-6 * Timer::ElapsedSince(ts_start_write);

  // Deallocate
  matrix_a.deallocate();

  auto ts_start_read    = Timer::Now();
  // Read Matrix
  matrix_t matrix_b;

	InputStream is(params.path);
	is >> matrix_b;

  dash::barrier();

  mes.time_read_s  = 1e-6 * Timer::ElapsedSince(ts_start_read);

  // Verify
  if(params.verify){
    for(auto i=matrix_b.lbegin(); i<matrix_b.lend(); i++) {
      if(*i != myid){
        DASH_THROW(dash::exception::RuntimeError,
                "HDF5 data is corrupted"
                );
      }
    }
  }
  matrix_b.deallocate();

  // Remove hdf5 file
  if(myid == 0){
    remove(params.path.c_str());
  }

  mes.time_total_s   = 1e-6 * Timer::ElapsedSince(ts_start_total);
  mes.mb_per_s_read  = mes.mb_global / mes.time_read_s;
  mes.mb_per_s_write = mes.mb_global / mes.time_write_s;
  return mes;

#else
  DASH_THROW(dash::exception::RuntimeError,
          "HDF5 module not enabled");
#endif
}

void print_measurement_header()
{
  if (dash::myid() == 0) {
    cout << std::right
         << std::setw(5)  << "units"       << ","
         << std::setw(9)  << "mpi.impl"    << ","
         << std::setw(12) << "mb.unit"     << ","
         << std::setw(12) << "mb.global"   << ","
         << std::setw(12) << "init.s"      << ","
         << std::setw(12) << "write.s"     << ","
         << std::setw(12) << "read.s"      << ","
         << std::setw(12) << "write.mb/s"  << ","
         << std::setw(12) << "read.mb/s"   << ","
         << std::setw(12) << "time.s"
         << endl;
  }
}

void print_measurement_record(
  const bench_cfg_params & cfg_params,
  measurement              measurement,
  const benchmark_params & params)
{
  if (dash::myid() == 0) {
    std::string mpi_impl = dash__toxstr(MPI_IMPL_ID);
    auto mes = measurement;
        cout << std::right
         << std::setw(5) << dash::size() << ","
         << std::setw(9) << mpi_impl     << ","
         << std::fixed << setprecision(2) << setw(12) << mes.mb_per_unit    << ","
         << std::fixed << setprecision(2) << setw(12) << mes.mb_global      << ","
         << std::fixed << setprecision(2) << setw(12) << mes.time_init_s    << ","
         << std::fixed << setprecision(2) << setw(12) << mes.time_write_s   << ","
         << std::fixed << setprecision(2) << setw(12) << mes.time_read_s    << ","
         << std::fixed << setprecision(2) << setw(12) << mes.mb_per_s_write << ","
         << std::fixed << setprecision(2) << setw(12) << mes.mb_per_s_read  << ","
         << std::fixed << setprecision(2) << setw(12) << mes.time_total_s
         << endl;
  }
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
