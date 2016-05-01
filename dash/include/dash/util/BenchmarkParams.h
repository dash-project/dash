#ifndef DASH__UTIL__BENCHMARK_PARAMS_H__
#define DASH__UTIL__BENCHMARK_PARAMS_H__

#include <dash/util/Locality.h>
#include <dash/Array.h>

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>


namespace dash {
namespace util {

struct CommandLineParamSpec {
  std::string flag;
  std::string description;
  std::string value_type;
  bool        required;
};

class BenchmarkParams
{
public:
  typedef std::vector< std::pair< std::string, std::string > >
    env_flags_type;

  typedef struct dash_config_params_t {
    env_flags_type env_mpi_config;
    env_flags_type env_dash_config;
    bool           env_hwloc;
    bool           env_mpi_shared_win;
    bool           env_mkl;
    bool           env_blas;
    bool           env_lapack;
    bool           env_scalapack;
    bool           env_plasma;
  } config_params_type;

  typedef dash::util::Locality::UnitPinning
    unit_pinning_type;

public:
  BenchmarkParams(const std::string & benchmark_name);

  inline void set_output_width(int width)
  {
    _header_width = width;
  }

  void parse_args(int argc, char * argv[]);

  const config_params_type & config() const {
    return _config;
  }

  int output_width() const {
    return _header_width;
  }

  void print_header();

  void print_pinning();

  void print_section_start(const std::string & section_name) const;
  void print_section_end() const;

  template <typename T>
  void print_param(
    const std::string & name,
    T value) const
  {
    if (dash::myid() != 0) {
      return;
    }
    int value_w = _header_width - 6 - name.length();
    std::cout << "--   "
              << std::left  << name << " "
              << std::right << std::setw(value_w) << value
              << std::endl;
  }

  template <typename T>
  void print_param(
    const std::string & flag,
    const std::string & description,
    T value) const
  {
    if (dash::myid() != 0) {
      return;
    }
    int flag_w  =  7;
    int value_w = 10;
    int desc_w  = _header_width - value_w - flag_w - 6;
    std::cout << "--   "
              << std::left  << std::setw(flag_w)  << flag << " "
              << std::right << std::setw(value_w) << value
              << std::right << std::setw(desc_w)  << description
              << std::endl;
  }

private:
  bool               _myid;
  int                _header_width = 72;
  config_params_type _config;
  std::string        _name;
};

} // namespae util
} // namespace dash

#endif // DASH__UTIL__BENCHMARK_PARAMS_H__
