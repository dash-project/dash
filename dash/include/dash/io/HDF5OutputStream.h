#ifndef DASH__IO__HDF5_OUTPUT_STREAM_H__
#define DASH__IO__HDF5_OUTPUT_STREAM_H__

#include <string>
#include <dash/Matrix.h>

namespace dash {
namespace io {

class HDF5Table {
public:
  std::string _table;
public:
  HDF5Table(std::string table) {
    _table = table;
  }
};

class HDF5Options
{
public:
  typedef struct file_options_t {
    bool          overwrite_file;
    bool          overwrite_table;  // TODO
    bool          store_pattern;
    bool          restore_pattern;
    std::string   pattern_metadata_key;
  } file_options;

  file_options _foptions;
  std::string  _test;

public:
  HDF5Options() {
  }

  HDF5Options(file_options opts) {
    _foptions = opts;
  }

  static inline file_options getDefaults() {
    file_options fopt;
    fopt.overwrite_file       = true;
    fopt.overwrite_table      = false;
    fopt.store_pattern        = true;
    fopt.restore_pattern      = true;
    fopt.pattern_metadata_key = "DASH_PATTERN";
    return fopt;
  }
};


class HDF5OutputStream
{
private:
  std::string                _filename;
  std::string                _table;
  HDF5Options::file_options  _foptions;

public:
  HDF5OutputStream(std::string filename) {
    _filename = filename;
  }

  friend HDF5OutputStream & operator<< (
    HDF5OutputStream & os,
    const HDF5Table & tbl);

  friend HDF5OutputStream & operator<< (
    HDF5OutputStream & os,
    HDF5Options opts);

  template <
    typename value_t,
    dim_t    ndim,
    typename index_t,
    class    pattern_t >
  friend HDF5OutputStream & operator<< (
    HDF5OutputStream & os,
    dash::Matrix < value_t,
    ndim,
    index_t,
    pattern_t > matrix);
};

HDF5OutputStream & operator<< (
  HDF5OutputStream & os,
  const HDF5Table & tbl)
{
  os._table = tbl._table;
  return os;
}

HDF5OutputStream & operator<< (
  HDF5OutputStream & os,
  HDF5Options opts)
{
  os._foptions = opts._foptions;
  return os;
}

} // namespace io
} // namespace dash

#include <dash/io/internal/HDF5OutputStream-inl.h>

#endif // DASH__IO__HDF5_OUTPUT_STREAM_H__
