#ifndef DASH__IO__HDF5__OUTPUT_STREAM_H__
#define DASH__IO__HDF5__OUTPUT_STREAM_H__

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

  
using HDF5DeviceMode = dash::io::IOSBaseMode;
using HDF5StreamMode = dash::io::IOStreamMode<HDF5DeviceMode>;

class OutputStream
: public ::dash::io::IOSBase<HDF5StreamMode> {

  typedef OutputStream                       self_t;
  typedef dash::io::IOSBase<HDF5StreamMode>  base_t;
  typedef HDF5StreamMode                     mode_t;

  private:

  std::string                _filename;
  std::string                _dataset;
  hdf5_options               _foptions;

  public:
    OutputStream(
        std::string filename,
        mode_t open_mode = HDF5DeviceMode::no_flags)
        : _filename(filename),
          _dataset("data"),
          _foptions(StoreHDF::get_default_options())
    {
      if((open_mode & HDF5DeviceMode::app)){
          _foptions.overwrite_file = false;
        }
    }

    // IO Manipulators

    friend OutputStream & operator<<(
        OutputStream & os,
        const dataset    & tbl)
    {
        os._dataset = tbl._dataset;
        return os;
    }

    friend OutputStream & operator<<(
        OutputStream & os,
        setpattern_key     pk)
    {
        os._foptions.pattern_metadata_key = pk._key;
        return os;
    }

    friend OutputStream & operator<<(
        OutputStream & os,
        store_pattern      sp) {
        os._foptions.store_pattern = sp._store;
        return os;
    }

    friend OutputStream & operator<<(
        OutputStream & os,
        modify_dataset     md)
    {
        os._foptions.modify_dataset = md._modify;
        return os;
    }


    // Array Implementation
    template <
        typename value_t,
        typename index_t,
        class    pattern_t >
    friend OutputStream & operator<<(
        OutputStream         & os,
        dash::Array< value_t,
                     index_t,
                     pattern_t > & array);

    // Matrix Implementation
    template <
        typename value_t,
        dim_t    ndim,
        typename index_t,
        class    pattern_t >
    friend OutputStream & operator<<(
        OutputStream          & os,
        dash::Matrix< value_t,
                      ndim,
                      index_t,
                      pattern_t > & matrix);

};

} // namespace hdf5
} // namespace io
} // namespace dash

#include <dash/io/hdf5/internal/OutputStream-inl.h>

#endif // DASH_ENABLE_HDF5

#endif // DASH__IO__HDF5__OUTPUT_STREAM_H__
