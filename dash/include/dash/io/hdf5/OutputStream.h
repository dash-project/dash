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

class OutputStream
: public ::dash::io::IOSBase<StreamMode> {

  typedef OutputStream                   self_t;
  typedef dash::io::IOSBase<StreamMode>  base_t;
  typedef StreamMode                     mode_t;

private:
  std::string                _filename;
  std::string                _dataset;
  hdf5_options               _foptions;
  type_converter             _converter;
  bool           _use_cust_conv = false;

public:
    OutputStream(
        std::string filename,
        mode_t open_mode = DeviceMode::no_flags)
        : _filename(filename),
          _dataset("data"),
          _foptions(StoreHDF::get_default_options())
    {
      if((open_mode & DeviceMode::app)){
          _foptions.overwrite_file = false;
        }
    }

    // IO Manipulators

    friend OutputStream & operator<<(
        OutputStream  & os,
        const dataset   tbl)
    {
        os._dataset = tbl._dataset;
        return os;
    }

    friend OutputStream & operator<<(
        OutputStream         & os,
        const setpattern_key   pk)
    {
        os._foptions.pattern_metadata_key = pk._key;
        return os;
    }

    friend OutputStream & operator<<(
        OutputStream         & os,
        const store_pattern    sp) {
        os._foptions.store_pattern = sp._store;
        return os;
    }

    friend OutputStream & operator<<(
        OutputStream         & os,
        const modify_dataset   md)
    {
        os._foptions.modify_dataset = md._modify;
        return os;
    }

    friend OutputStream & operator<< (
      OutputStream         & os,
      const type_converter   conv) {
      os._converter = conv;
      os._use_cust_conv = true;
      return os;
    }

    template < typename Container_t >
    friend OutputStream & operator<<(
        OutputStream & os,
        Container_t  & container);

};

} // namespace hdf5
} // namespace io
} // namespace dash

#include <dash/io/hdf5/internal/OutputStream-inl.h>

#endif // DASH_ENABLE_HDF5

#endif // DASH__IO__HDF5__OUTPUT_STREAM_H__
