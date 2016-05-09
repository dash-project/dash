#ifndef DASH__IO__HDF5_OUTPUT_STREAM_H__
#define DASH__IO__HDF5_OUTPUT_STREAM_H__

#ifdef DASH_ENABLE_HDF5

#include <string>
#include <dash/Matrix.h>

namespace dash {
namespace io {

typedef dash::io::StoreHDF::hdf5_file_options hdf5_file_options;

class HDF5Table {
  public:
    std::string _table;
  public:
    HDF5Table(std::string table) {
        _table = table;
    }
};

class HDF5Options {
  public:
    hdf5_file_options _foptions;
    std::string  _test;

  public:
    HDF5Options() {
    }

    HDF5Options(hdf5_file_options opts) {
        _foptions = opts;
    }
};


class HDF5OutputStream {
  private:
    std::string                _filename;
    std::string                _table;
    hdf5_file_options					 _foptions;

  public:
    HDF5OutputStream(std::string filename) {
        _filename = filename;
        _foptions = dash::io::StoreHDF::get_default_options();
    }

    friend HDF5OutputStream & operator<< (
        HDF5OutputStream & os,
        const HDF5Table & tbl) {
        os._table = tbl._table;
        return os;
    }

    friend HDF5OutputStream & operator<< (
        HDF5OutputStream & os,
        hdf5_file_options opts) {
        os._foptions = opts;
        return os;
    }


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
        pattern_t > &matrix);
};



} // namespace io
} // namespace dash

#include <dash/io/internal/HDF5OutputStream-inl.h>

#endif // DASH_ENABLE_HDF5

#endif // DASH__IO__HDF5_OUTPUT_STREAM_H__
