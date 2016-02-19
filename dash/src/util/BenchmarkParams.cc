#include <dash/util/BenchmarkParams.h>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <string>
#include <cstring>
#include <ctime>

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
  _config = params;

  // Collect process pinning information:
  _unit_pinning.allocate(dash::size(), dash::BLOCKCYCLIC(1));
  dash::barrier();

  int cpu       = dash::util::Locality::UnitCPU();
  int numa_node = dash::util::Locality::UnitNUMANode();

  unit_pinning_type my_pin_info;
  my_pin_info.rank      = dash::myid();
  my_pin_info.cpu       = cpu;
  my_pin_info.numa_node = numa_node;
  gethostname(my_pin_info.host, 100);

  _unit_pinning[dash::myid()] = my_pin_info;

  // Ensure pinning data is ready:
  dash::barrier();
}

void BenchmarkParams::parse_args(
  int argc, char * argv[])
{
#if 0
  std::string flags_str = argv[i+1];
  // Split string into vector of key-value pairs
  const char delim    = ':';
  string::size_type i = 0;
  string::size_type j = flags_str.find(delim);
  while (j != string::npos) {
    string flag_str = flags_str.substr(i, j-i);
    // Split into key and value:
    string::size_type fi = flag_str.find('=');
    string::size_type fj = flag_str.find('=', fi);
    if (fj == string::npos) {
      fj = flag_str.length();
    }
    string flag_name    = flag_str.substr(0,    fi);
    string flag_value   = flag_str.substr(fi+1, fj);
    params.env_config.push_back(std::make_pair(flag_name, flag_value));
    i = ++j;
    j = flags_str.find(delim, j);
  }
  if (j == string::npos) {
    // Split into key and value:
    string::size_type k = flags_str.find('=', i+1);
    string flag_name    = flags_str.substr(i, k-i);
    string flag_value   = flags_str.substr(k+1, flags_str.length());
    params.env_config.push_back(std::make_pair(flag_name, flag_value));
  }
#endif
}

void BenchmarkParams::print_header()
{
  if (_myid != 0) {
    return;
  }

  size_t box_width  = _header_width;
  std::string separator(box_width, '-');
  size_t numa_nodes = dash::util::Locality::NumNumaNodes();
  size_t local_cpus = dash::util::Locality::NumCPUs();

  std::time_t t_now = std::time(NULL);
  char date_cstr[100];
  std::strftime(date_cstr, sizeof(date_cstr), "%c", std::localtime(&t_now));

  print_section_end();
  print_section_start("Benchmark");
  print_param("identifier", _name);
  print_param("date",   date_cstr);
  print_section_end();

  print_section_start("Hardware Locality");
  print_param("CPUs/node",    local_cpus);
  print_param("NUMA domains", numa_nodes);
  print_section_end();

#ifdef MPI_IMPL_ID
  print_section_start("MPI Environment Flags");
  for (auto flag : _config.env_config) {
    int val_w  = box_width - flag.first.length() - 5;
    cout << "--   " << std::left   << flag.first
                    << setw(val_w) << std::right << flag.second
         << endl;
  }
  print_section_end();
#endif

  print_section_start("DASH Configuration");
#ifdef MPI_IMPL_ID
  print_param("MPI implementation", dash__toxstr(MPI_IMPL_ID));
#endif
  if (_config.env_mpi_shared_win) {
    print_param("MPI shared windows", "enabled");
  } else {
    print_param("MPI shared windows", "disabled");
  }
  if (_config.env_mkl) {
    print_param("Intel MKL", "enabled");
    if (_config.env_scalapack) {
      print_param("ScaLAPACK", "enabled");
    } else {
      print_param("ScaLAPACK", "disabled");
    }
  } else {
    print_param("Intel MKL", "disabled");
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
       << std::right
       << std::setw(10)     << "numa node"
       << std::setw(5)      << "cpu"
       << endl;
  for (size_t unit = 0; unit < _unit_pinning.size(); ++unit) {
    unit_pinning_type pin_info = _unit_pinning[unit];
    cout << std::left         << "--   "
         << std::setw(5)      << pin_info.rank
         << std::setw(host_w) << pin_info.host
         << std::right
         << std::setw(10)     << pin_info.numa_node
         << std::setw(5)      << pin_info.cpu
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

std::ostream & operator<<(
  std::ostream        & os,
  const typename BenchmarkParams::unit_pinning_type & upi)
{
  std::ostringstream ss;
  ss << "unit_pinning_type("
     << "rank:" << upi.rank << " "
     << "host:" << upi.host << " "
     << "cpu:"  << upi.cpu  << " "
     << "numa:" << upi.numa_node << ")";
  return operator<<(os, ss.str());
}

} // namespace util
} // namespace dash
