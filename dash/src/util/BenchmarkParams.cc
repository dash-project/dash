#include <dash/util/BenchmarkParams.h>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <string>
#include <cstring>
#include <ctime>

#include <dash/util/Config.h>
#include <dash/util/Locality.h>
#include <dash/Array.h>

// Environment variables as array of strings, terminated by null pointer.
extern char ** environ;


namespace dash {
namespace util {

using std::cout;
using std::endl;
using std::setw;
using std::setprecision;

BenchmarkParams::BenchmarkParams(
  const std::string & benchmark_name)
: _name(benchmark_name)
{
  _myid = dash::myid();

  config_params_type params;
  params.env_mpi_shared_win = true;
  params.env_hwloc          = false;
  params.env_mkl            = false;
  params.env_blas           = false;
  params.env_lapack         = false;
  params.env_scalapack      = false;
  params.env_plasma         = false;
#ifdef DART_MPI_DISABLE_SHARED_WINDOWS
  params.env_mpi_shared_win = false;
#endif
#ifdef DASH_ENABLE_HWLOC
  params.env_hwloc          = true;
#endif
#ifdef DASH_ENABLE_MKL
  params.env_mkl            = true;
#endif
#ifdef DASH_ENABLE_BLAS
  params.env_blas           = true;
#endif
#ifdef DASH_ENABLE_LAPACK
  params.env_lapack         = true;
#endif
#ifdef DASH_ENABLE_SCALAPACK
  params.env_scalapack      = true;
#endif
#ifdef DASH_ENABLE_PLASMA
  params.env_plasma         = true;
#endif
  // Add environment variables to params.env_mpi_config and
  // params.env_dash_config:
  int    i          = 1;
  char * env_var_kv = *environ;
  for (; env_var_kv != 0; ++i) {
    // Split into key and value:
    char * flag_name_cstr  = env_var_kv;
    char * flag_value_cstr = strstr(env_var_kv, "=");
    int    flag_name_len   = flag_value_cstr - flag_name_cstr;
    std::string flag_name(flag_name_cstr, flag_name_cstr + flag_name_len);
    std::string flag_value(flag_value_cstr+1);
    if (strstr(env_var_kv, "I_MPI_") == env_var_kv ||
        strstr(env_var_kv, "MV2_")   == env_var_kv ||
        strstr(env_var_kv, "MPICH")  == env_var_kv ||
        strstr(env_var_kv, "OMPI_")  == env_var_kv ||
        strstr(env_var_kv, "MP_")    == env_var_kv)
    {
      params.env_mpi_config.push_back(std::make_pair(flag_name, flag_value));
    }
    env_var_kv = *(environ + i);
  }
  _config = params;
}

void BenchmarkParams::parse_args(
  int argc, char * argv[])
{
  // TODO
}

void BenchmarkParams::print_header()
{
  if (_myid != 0) {
    return;
  }

  size_t box_width        = _header_width;
  size_t numa_nodes       = dash::util::Locality::NumNUMANodes();
  size_t num_nodes        = dash::util::Locality::NumNodes();
  size_t local_cpus       = dash::util::Locality::NumCPUs();
  int    cpu_max_mhz      = dash::util::Locality::CPUMaxMhz();
  int    cpu_min_mhz      = dash::util::Locality::CPUMinMhz();
  auto   cache_sizes      = dash::util::Locality::CacheSizes();
  auto   cache_line_sizes = dash::util::Locality::CacheLineSizes();
  std::string separator(box_width, '-');

  std::time_t t_now = std::time(NULL);
  char date_cstr[100];
  std::strftime(date_cstr, sizeof(date_cstr), "%c", std::localtime(&t_now));

  print_section_end();
  print_section_start("Benchmark");
  print_param("identifier", _name);
  print_param("date",   date_cstr);
  print_section_end();

  print_section_start("Hardware Locality");
  print_param("Processing nodes",  num_nodes);
  print_param("CPUs/node",         local_cpus);
  print_param("NUMA domains/node", numa_nodes);
  print_param("CPU max MHz",       cpu_max_mhz);
  print_param("CPU min MHz",       cpu_min_mhz);
  for (size_t level = 0; level < cache_sizes.size(); ++level) {
    auto cache_kb     = cache_sizes[level] / 1024;
    auto cache_line_b = cache_line_sizes[level];
    std::ostringstream cn;
    cn << "L" << (level+1) << "d cache";
    std::ostringstream cs;
    cs << std::right << std::setw(5) << cache_kb << " KB, "
       << std::right << std::setw(2) << cache_line_b << " B/line";
    print_param(cn.str(), cs.str());
  }
  print_section_end();

#ifdef MPI_IMPL_ID
  print_section_start("MPI Environment Flags");
  for (auto flag : _config.env_mpi_config) {
    int val_w  = box_width - flag.first.length() - 6;
    cout << "--   " << std::left   << flag.first << " "
                    << setw(val_w) << std::right << flag.second
         << endl;
  }
  print_section_end();
#endif

  print_section_start("DASH Environment Flags");
  for (auto flag = dash::util::Config::begin();
       flag != dash::util::Config::end(); ++flag)
  {
    int val_w  = box_width - flag->first.length() - 5;
    cout << "--   " << std::left   << flag->first
                    << setw(val_w) << std::right << flag->second
         << endl;
  }
  print_section_end();

  print_section_start("DASH Configuration");
#ifdef MPI_IMPL_ID
  print_param("MPI implementation", dash__toxstr(MPI_IMPL_ID));
#endif
#ifdef DASH_ENV_HOST_SYSTEM_ID
  print_param("Host system identifier", dash__toxstr(DASH_ENV_HOST_SYSTEM_ID));
#endif
  if (_config.env_hwloc) {
    print_param("hwloc",              "enabled");
  } else {
    print_param("hwloc",              "disabled");
  }
  if (_config.env_mpi_shared_win) {
    print_param("MPI shared windows", "enabled");
  } else {
    print_param("MPI shared windows", "disabled");
  }
  if (_config.env_blas) {
    print_param("BLAS",               "enabled");
  } else {
    print_param("BLAS",               "disabled");
  }
  if (_config.env_lapack) {
    print_param("LAPACK",             "enabled");
  } else {
    print_param("LAPACK",             "disabled");
  }
  if (_config.env_mkl) {
    print_param("Intel MKL",          "enabled");
  } else {
    print_param("Intel MKL",          "disabled");
  }
  if (_config.env_scalapack) {
    print_param("ScaLAPACK",          "enabled");
  } else {
    print_param("ScaLAPACK",          "disabled");
  }
  print_section_end();
}

void BenchmarkParams::print_pinning()
{
  if (_myid != 0) {
    return;
  }
  auto line_w = _header_width;
  auto host_w = line_w - 5 - 5 - 10 - 5;
  print_section_start("Process Pinning");
  cout << std::left         << "--   "
       << std::setw(5)      << "unit"
       << std::setw(host_w) << "host"
       << std::setw(10)     << "domain"
       << std::right
       << std::setw(10)     << "numa node"
       << std::setw(5)      << "core"
       << endl;
  for (size_t unit = 0; unit < dash::size(); ++unit) {
    unit_pinning_type pin_info = Locality::Pinning(unit);
    cout << std::left         << "--   "
         << std::setw(5)      << pin_info.unit
         << std::setw(host_w) << pin_info.host
         << std::setw(10)     << pin_info.domain
         << std::right
         << std::setw(10)     << pin_info.numa_id
         << std::setw(5)      << pin_info.core_id
         << endl;
  }
  print_section_end();
}

void BenchmarkParams::print_section_start(
  const std::string & section_name) const
{
  if (_myid != 0) {
    return;
  }
  cout << std::left << "-- " << section_name << endl;
}

void BenchmarkParams::print_section_end() const
{
  if (_myid != 0) {
    return;
  }
  auto line_w = _header_width;
  std::string separator(line_w, '-');
  cout << separator << endl;
}

} // namespace util
} // namespace dash
