#ifndef DASH__IO__HDF5__INPUT_STREAM_H__
#define DASH__IO__HDF5__INPUT_STREAM_H__

#ifdef DASH_ENABLE_HDF5

#include <string>

#include <dash/Matrix.h>
#include <dash/Array.h>


namespace dash {
namespace io {
namespace hdf5 {

/**
 * DASH stream API to store an dash::Array or dash::Matrix
 * in an HDF5 file using parallel IO.
 *
 * All operations are collective.
 */

using IOStreamMode = dash::io::IOStreamMode<dash::io::IOSBaseMode::openmode_type>;

class InputStream
: public ::dash::io::IOSBase<IOStreamMode> {
  private:
    std::string                _filename;
    std::string                _dataset;
    hdf5_options               _foptions;

  public:
    InputStream(std::string filename)
        : _filename(filename),
          _dataset("data"),
          _foptions(StoreHDF::get_default_options())
    { }

    // IO Manipulators

    friend InputStream & operator>> (
        InputStream & is,
        const dataset   & tbl) {
        is._dataset = tbl._dataset;
        return is;
    }

    friend InputStream & operator>> (
      InputStream & is,
      setpattern_key    pk) {
      is._foptions.pattern_metadata_key = pk._key;
      return is;
    }

    friend InputStream & operator>> (
      InputStream & is,
      restore_pattern   rs) {
      is._foptions.restore_pattern = rs._restore;
      return is;
    }


    // Array Implementation
    template <
        typename value_t,
        typename index_t,
        class    pattern_t >
    friend InputStream & operator>> (
        InputStream           & is,
        dash::Array < value_t,
                      index_t,
                      pattern_t > & array);

    // Matrix Implementation
    template <
        typename value_t,
        dim_t    ndim,
        typename index_t,
        class    pattern_t >
    friend InputStream & operator>> (
        InputStream           & is,
        dash::Matrix< value_t,
                      ndim,
                      index_t,
                      pattern_t > & matrix);
};

} // namespace hfd5
} // namespace io
} // namespace dash

#include <dash/io/hdf5/internal/InputStream-inl.h>

#endif // DASH_ENABLE_HDF5

#endif // DASH__IO__HDF5__INPUT_STREAM_H__
