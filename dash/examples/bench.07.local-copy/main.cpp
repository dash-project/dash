/**
 * Local copy benchmark for various containers
 *
 * author(s): Felix Moessbauer, LMU Munich */
/* @DASH_HEADER@ */

// #define DASH__ALGORITHM__COPY__USE_WAIT

#include <libdash.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <cstring>

using std::cout;
using std::endl;

// Environment variables as array of strings, terminated by null pointer.
extern char ** environ;

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

typedef std::vector< std::pair< std::string, std::string > >
  env_flags;

typedef struct benchmark_params_t {
  env_flags   env_config;
  bool        env_mkl;
  bool        env_scalapack;
  bool        env_mpi_shared_win;
} benchmark_params;

typedef struct {
  int  rank;
  char host[100];
  int  cpu;
  int  numa_node;
} unit_pin_info;

std::ostream & operator<<(
  std::ostream        & os,
  const unit_pin_info & upi)
{
  std::ostringstream ss;
  ss << "unit_pin_info("
     << "rank:" << upi.rank << " "
     << "host:" << upi.host << " "
     << "cpu:"  << upi.cpu  << " "
     << "numa:" << upi.numa_node << ")";
  return operator<<(os, ss.str());
}

#define DASH_PRINT_MASTER(expr) \
  do { \
    if (dash::myid() == 0) { \
      std::cout << "" << expr << std::endl; \
    } \
  } while(0)

#ifndef DASH__ALGORITHM__COPY__USE_WAIT
const std::string dash_copy_variant = "flush";
#else
const std::string dash_copy_variant = "wait";
#endif

double copy_block_to_local(size_t size, int num_repeats, index_t block_index);

benchmark_params parse_args(int argc, char * argv[]);

void print_params(const benchmark_params & params);
void print_measurement_header();
void print_measurement_record(
  const std::string      & scenario,
  const benchmark_params & params,
  int                      unit_src,
  size_t                   size,
  int                      num_repeats,
  double                   time_s,
  double                   kps);

int main(int argc, char** argv)
{
  dash::init(&argc, &argv);
  Timer::Calibrate(0);

  double kps;
  double time_s;
  size_t size;
  auto   params          = parse_args(argc, argv);
  size_t num_iterations  = 11;
  int    num_repeats     = 50 * std::pow(2, 10);
  auto   ts_start        = Timer::Now();
  size_t num_numa_nodes  = dash::util::Locality::NumNumaNodes();
  size_t num_local_cpus  = dash::util::Locality::NumCPUs();
  // Number of physical cores in a single NUMA domain (7 on SuperMUC):
  size_t numa_node_cores = num_local_cpus / num_numa_nodes;
  // Number of physical cores on a single socket (14 on SuperMUC):
  size_t socket_cores    = numa_node_cores * 2;

  // Collect process pinning information:
  //
  dash::Array<unit_pin_info> unit_pinning(dash::size());

  int cpu        = dash::util::Locality::UnitCPU();
  int numa_node  = dash::util::Locality::UnitNUMANode();

  unit_pin_info my_pin_info;
  my_pin_info.rank      = dash::myid();
  my_pin_info.cpu       = cpu;
  my_pin_info.numa_node = numa_node;
  gethostname(my_pin_info.host, 100);

  unit_pinning[dash::myid()] = my_pin_info;

  dash::barrier();

  if (dash::myid() == 0) {
    print_params(params);

    cout << std::left     << "-- "
         << std::setw(5)  << "unit"
         << std::setw(32) << "host"
         << std::setw(10) << "numa node"
         << std::setw(5)  << "cpu"
         << endl;
    for (size_t unit = 0; unit < dash::size(); ++unit) {
      unit_pin_info pin_info = unit_pinning[unit];
      cout << std::left     << "-- "
           << std::setw(5)  << pin_info.rank
           << std::setw(32) << pin_info.host
           << std::setw(10) << pin_info.numa_node
           << std::setw(5)  << pin_info.cpu
           << endl;
    }
  }

  dash::barrier();

  print_measurement_header();

  /// Increments of 64 MB of elements in total:
  size_t size_inc = 1 * (static_cast<size_t>(std::pow(2, 30)) /
                         sizeof(ElementType)) / 16;
  size_t size_min = size_inc;

  dart_unit_t unit_src;
  for (size_t iteration = 0; iteration < num_iterations; ++iteration) {
    size     = size_min + (std::pow(2,iteration) * size_inc);

    // Copy first block in array, assigned to unit 0:
    unit_src = 0;
    ts_start = Timer::Now();
    kps      = copy_block_to_local(size, num_repeats, unit_src);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("local", params, unit_src, size, num_repeats,
                             time_s, kps);

    // Copy last block in the master's NUMA domain:
    unit_src = (numa_node_cores-1) % dash::size();
    ts_start = Timer::Now();
    kps      = copy_block_to_local(size, num_repeats, unit_src);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("uma", params, unit_src, size, num_repeats,
                             time_s, kps);

    // Copy block in the master's neighbor NUMA domain:
    unit_src = (numa_node_cores + (numa_node_cores / 2)) % dash::size();
    ts_start = Timer::Now();
    kps      = copy_block_to_local(size, num_repeats, unit_src);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("numa", params, unit_src, size, num_repeats,
                             time_s, kps);

    // Copy first block in next socket on the master's node:
    unit_src = (socket_cores + (numa_node_cores / 2)) % dash::size();
    ts_start = Timer::Now();
    kps      = copy_block_to_local(size, num_repeats, unit_src);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("socket", params, unit_src, size, num_repeats,
                             time_s, kps);

    // Copy block preceeding last block as it is guaranteed to be located on
    // a remote unit and completely filled:
    unit_src = dash::size() - 2;
    ts_start = Timer::Now();
    kps      = copy_block_to_local(size, num_repeats, unit_src);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("remote", params, unit_src, size, num_repeats,
                             time_s, kps);

    num_repeats /= 2;
  }

  DASH_PRINT_MASTER("Benchmark finished");

  dash::finalize();
  return 0;
}

double copy_block_to_local(size_t size, int num_repeats, index_t block_index)
{
  Array_t global_array(size, dash::BLOCKED);

  auto    block_size       = global_array.pattern().local_size();
  // Index of block to copy. Use block of succeeding neighbor
  // which is expected to be in same NUMA domain for unit 0:
  auto    source_block     = global_array.pattern().block(block_index);
  index_t copy_start_idx   = source_block.offset(0);
  index_t copy_end_idx     = copy_start_idx + block_size;
  auto    source_unit_id   = global_array.pattern().unit_at(copy_start_idx);
  double  elapsed          = 1;

  DASH_LOG_DEBUG("copy_block_to_local()",
                 "size:",             size,
                 "block index:",      block_index,
                 "block size:",       block_size,
                 "copy index range:", copy_start_idx, "-", copy_end_idx);

  if (source_unit_id != block_index) {
    DASH_THROW(dash::exception::RuntimeError,
               "copy_block_to_local: Invalid distribution of global array");
    return 0;
  }

  for (size_t l = 0; l < global_array.lsize(); ++l) {
    global_array.local[l] = ((dash::myid() + 1) * 1000) + l;
  }
  dash::barrier();

  if(dash::myid() == 0) {
    ElementType * local_array = new ElementType[block_size];

    // Perform measurement:
    auto timer_start = Timer::Now();
    for (int r = 0; r < num_repeats; ++r) {
      ElementType * copy_lend = dash::copy(
                                  global_array.begin() + copy_start_idx ,
                                  global_array.begin() + copy_end_idx,
                                  local_array);
      DASH_ASSERT_EQ(local_array + block_size, copy_lend,
                     "Unexpected end of copied range");
#ifndef DASH_ENABLE_ASSERTIONS
      dash__unused(copy_lend);
#endif
    }
    elapsed = Timer::ElapsedSince(timer_start);

    // Validate values:
    for (int l = 0; l < static_cast<int>(block_size); ++l) {
      if (local_array[l] != ((source_unit_id + 1) * 1000) + l) {
        DASH_THROW(dash::exception::RuntimeError,
                   "copy_block_to_local: Validation failed");
        return 0;
      }
    }

    delete[] local_array;
  }

  DASH_LOG_DEBUG(
      "copy_block_to_local",
      "Waiting for completion of copy operation");
  dash::barrier();
  return (static_cast<double>(block_size * num_repeats) / elapsed);
}

void print_measurement_header()
{
  if (dash::myid() == 0) {
    cout << std::right
         << std::setw(5)  << "units"      << ","
         << std::setw(10) << "mpi.impl"   << ","
         << std::setw(10) << "scenario"   << ","
         << std::setw(10) << "copy type"  << ","
         << std::setw(9)  << "src.unit"   << ","
         << std::setw(9)  << "repeats"    << ","
         << std::setw(12) << "blocksize"  << ","
         << std::setw(9)  << "glob.mb"    << ","
         << std::setw(9)  << "mb/rank"    << ","
         << std::setw(9)  << "time.s"     << ","
         << std::setw(12) << "elem.m/s"
         << endl;
  }
}

void print_measurement_record(
  const std::string      & scenario,
  const benchmark_params & params,
  int                      unit_src,
  size_t                   size,
  int                      num_repeats,
  double                   secs,
  double                   kps)
{
  if (dash::myid() == 0) {
    std::string mpi_impl = dash__toxstr(MPI_IMPL_ID);
    double mem_g = ((static_cast<double>(size) *
                     sizeof(ElementType)) / 1024) / 1024;
    double mem_l = mem_g / dash::size();
    cout << std::right
         << std::setw(5)  << dash::size()        << ","
         << std::setw(10) << mpi_impl            << ","
         << std::setw(10) << scenario            << ","
         << std::setw(10) << dash_copy_variant   << ","
         << std::setw(9)  << unit_src            << ","
         << std::setw(9)  << num_repeats         << ","
         << std::setw(12) << size / dash::size() << ","
         << std::fixed << std::setprecision(2) << std::setw(9)  << mem_g << ","
         << std::fixed << std::setprecision(2) << std::setw(9)  << mem_l << ","
         << std::fixed << std::setprecision(2) << std::setw(9)  << secs  << ","
         << std::fixed << std::setprecision(2) << std::setw(12) << kps
         << endl;
  }
}

benchmark_params parse_args(int argc, char * argv[])
{
  benchmark_params params;
  params.env_mpi_shared_win = true;
  params.env_mkl            = false;
  params.env_scalapack      = false;
#ifdef DASH_ENABLE_MKL
  params.env_mkl            = true;
#endif
#ifdef DASH_ENABLE_SCALAPACK
  params.env_scalapack      = true;
#endif
#ifdef DART_MPI_DISABLE_SHARED_WINDOWS
  params.env_mpi_shared_win = false;
#endif
  // Add environment variables starting with 'I_MPI_' or 'MP_' to
  // params.env_config:
  int    i          = 1;
  char * env_var_kv = *environ;
  for (; env_var_kv != 0; ++i) {
    if (strstr(env_var_kv, "I_MPI_") == env_var_kv ||
        strstr(env_var_kv, "MV2_")   == env_var_kv ||
        strstr(env_var_kv, "OMPI_")  == env_var_kv ||
        strstr(env_var_kv, "MP_")    == env_var_kv)
    {
      // Split into key and value:
      char * flag_name_cstr  = env_var_kv;
      char * flag_value_cstr = strstr(env_var_kv, "=");
      int    flag_name_len   = flag_value_cstr - flag_name_cstr;
      std::string flag_name(flag_name_cstr, flag_name_cstr + flag_name_len);
      std::string flag_value(flag_value_cstr+1);
      params.env_config.push_back(std::make_pair(flag_name, flag_value));
    }
    env_var_kv = *(environ + i);
  }
  return params;
}

void print_params(const benchmark_params & params)
{
  size_t box_width = 53;
  std::string separator(box_width, '-');
  size_t numa_nodes = dash::util::Locality::NumNumaNodes();
  size_t local_cpus = dash::util::Locality::NumCPUs();
  cout << separator << endl
       << "-- bench.07.local-copy" << endl
       << "-- environment:"        << endl
       << "--   NUMA nodes:" << std::setw(box_width-16) << numa_nodes << endl
       << "--   Local CPUs:" << std::setw(box_width-16) << local_cpus << endl
       << "--   Flags:"
       << endl;
  for (auto flag : params.env_config) {
    cout << "--     " << std::setw(box_width-22) << std::left  << flag.first
                      << std::setw(15)           << std::right << flag.second
         << endl;
  }
#ifdef MPI_IMPL_ID
  cout << "--   MPI implementation:"
       << std::setw(box_width-24) << dash__toxstr(MPI_IMPL_ID) << endl;
#endif
  cout << "--   MPI shared windows:";
  if (params.env_mpi_shared_win) {
    cout << std::setw(box_width-24) <<  "enabled" << endl;
  } else {
    cout << std::setw(box_width-24) << "disabled" << endl;
  }
  cout << "--   Intel MKL:";
  if (params.env_mkl) {
    cout << std::setw(box_width-15) << "enabled" << endl;
    cout << "--   ScaLAPACK:";
    if (params.env_scalapack) {
      cout << std::setw(box_width-15) <<  "enabled" << endl;
    } else {
      cout << std::setw(box_width-15) << "disabled" << endl;
    }
  } else {
    cout << std::setw(box_width-15) << "disabled" << endl;
  }
  cout << separator
       << endl;
}
