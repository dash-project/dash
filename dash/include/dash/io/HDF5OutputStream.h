#ifndef DASH__IO__HDF5_OUTPUT_STREAM_H__
#define DASH__IO__HDF5_OUTPUT_STREAM_H__

#ifdef DASH_ENABLE_HDF5

#include <string>
#include <dash/Matrix.h>

namespace dash {
namespace io {

/**
 * DASH stream API to store an dash::Array or dash::Matrix
 * in an HDF5 file using parallel IO.
 *
 * All operations are collective.
 */

class HDF5OutputStream {
  private:
    std::string                _filename;
    std::string                _dataset;
    hdf5_file_options					 _foptions;

  public:
    HDF5OutputStream(std::string filename) {
        _filename = filename;
        _foptions = dash::io::StoreHDF::get_default_options();
    }

    friend HDF5OutputStream & operator<< (
        HDF5OutputStream & os,
        const HDF5Dataset & tbl) {
        os._dataset = tbl._dataset;
        return os;
    }

    friend HDF5OutputStream & operator<< (
        HDF5OutputStream & os,
        hdf5_file_options opts) {
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
};



} // namespace io
} // namespace dash

#include <dash/io/internal/HDF5OutputStream-inl.h>

#endif // DASH_ENABLE_HDF5

#endif // DASH__IO__HDF5_OUTPUT_STREAM_H__
