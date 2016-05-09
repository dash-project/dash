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

/**
 * DASH stream API to store an dash::Array or dash::Matrix
 * in an HDF5 file using parallel IO.
 *
 * All operations are collective.
 */

class HDF5OutputStream {
  private:
    std::string                _filename;
    std::string                _table;
    hdf5_file_options					 _foptions;
    bool											 _flushed;

  public:
    HDF5OutputStream(std::string filename) {
        _filename = filename;
        _flushed  = false;
        _foptions = dash::io::StoreHDF::get_default_options();
    }

    friend HDF5OutputStream & operator<< (
        HDF5OutputStream & os,
        const HDF5Table & tbl) {
        os._assert_flush();
        os._table = tbl._table;
        return os;
    }

    friend HDF5OutputStream & operator<< (
        HDF5OutputStream & os,
        hdf5_file_options opts) {
        os._assert_flush();
        os._foptions = opts;
        return os;
    }

    // Array Implementation
    template <
        typename value_t,
        typename index_t,
        class    pattern_t >
    friend HDF5OutputStream & operator<< (
        HDF5OutputStream & os,
        dash::Array < value_t,
        index_t,
        pattern_t > &array);

    template <
        typename value_t,
        typename index_t,
        class    pattern_t >
    friend HDF5OutputStream & operator>> (
        HDF5OutputStream & os,
        dash::Array < value_t,
        index_t,
        pattern_t > &array);

    // Matrix Implementation
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

    template <
        typename value_t,
        dim_t    ndim,
        typename index_t,
        class    pattern_t >
    friend HDF5OutputStream & operator>> (
        HDF5OutputStream & os,
        dash::Matrix < value_t,
        ndim,
        index_t,
        pattern_t > &matrix);

  private:
    void _assert_flush() {
        if(_flushed) {
            dash::exception::AssertionFailed(
                "cannot modify HDF5 parameters because matrix is already written");
        }
    }
};



} // namespace io
} // namespace dash

#include <dash/io/internal/HDF5OutputStream-inl.h>

#endif // DASH_ENABLE_HDF5

#endif // DASH__IO__HDF5_OUTPUT_STREAM_H__
